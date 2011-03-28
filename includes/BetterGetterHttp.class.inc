<?php

/**
 * Extension of Better Getter class for handling HTTP connections.
 *
 * An aspect of handling HTTP which is unique is HTTP headers, which are sent
 * by both the client and the server as part of a connection. This class
 * features methods for specifying outgoing HTTP headers for a connection, as
 * well as parsing out incoming HTTP headers from the server response.
 *
 * @todo: Authentication, compression, HTTPS/SSL certificate handling, cookie
 * handling, et al.
 */
class BetterGetterHttp extends BetterGetter {

  /**
   * Error constants.
   */

  /**
   * Please use the setPostField() or setRawPostData() methods to set POST data.
   */
  const ERR_USE_SETPOSTFIELD = 1;
  
  /**
   * Please use the setRequestHeader() method to set request headers.
   */
  const ERR_USE_SETREQUESTHEADER = 2;

  /**
   * HTTP headers couldn't be parsed.
   */
  const ERR_HTTP_HEADERS_NOT_EXTRACTIBLE = 900;


  /**
   * An associative array of headers to be sent as part of the server request,
   * keyed by header field name.
   *
   * Each value is a flat array of values. Why an array instead of a string?
   * Because it's theoretically possible to send headers repeated with different
   * values, though I'm not sure if this is in common use.
   *
   * Note that when a request happens, this param is nulled out, then rebuilt
   * using the actual headers sent to the server. Why? Because some cURL options
   * may define outgoing HTTP headers which wouldn't be explicitly set by using
   * the setRequestHeader() method, and therefore not be in this array, even
   * though they were set to the server. For example, the CURLOPT_USERAGENT
   * option will cause a User-Agent header to be sent to the server which won't
   * normally be in this array. However, it is possible to get the "real"
   * request headers that cURL sent as a string after execution, and then parse
   * them into an array in the format we're using here. However, it only really
   * makes sense to do so if the user actually tries to get the request headers.
   * So we check in getRequestHeaders() and rebuild if necessary in there.
   */
  protected $httpRequestHeaders = array();

  /**
   * The raw, unparsed HTTP request header string.
   */  
  protected $httpRawRequestHeaders;
 
  /**
   * POST parameters, if any, as an array for easy getting and setting.
   *
   * If this array (or $httpRawPostData) is not empty when a query is made,
   * the query will be a POST query made with this data (unless overriden with
   * the CURLOPT_CUSTOMREQUEST cURL option).
   *
   * Note that if both $httpPostFields and an $httpRawPostData are set at query
   * time, $httpRawPostData will take precedence and this data will be ignored.
   *
   * @see BetterGetterHttp::$httpRawPostData
   */
  protected $httpPostFields = array();

  /**
   * Raw data to be sent as POST query data.
   *
   * If set, the query will be a POST query, unless overridden with the
   * CURLOPT_CUSTOMREQUEST cURL option.
   *
   * Note that if both $httpPostFields and an $httpRawPostData are set at query
   * time, $httpRawPostData will take precedence and $httpPostFields will be
   * ignored.
   *
   * @see BetterGetterHttp::$httpPostFields
   */
  protected $httpRawPostData;

  /**
   * An associative array of headers sent from the server as part of the server
   * response, keyed by header field name.
   *
   * Like with BetterGetterHttp::httpRequestHeaders, each value is a flat array
   * of values, since the same field with different browsers may be sent by
   * servers. Unlike with outgoing headers, this practice *is* in common use,
   * particularly when it comes to cookies.
   * 
   * Note that this is not populated immediately after a response is returned;
   * it's populated by the parsehttpResponseHeaders() method, which will be
   * called if the getResponseHeaders() or getResponseHeader() methods are
   * called and the headers haven't been parsed yet. This makes it possible to
   * quickly get an HTTP response without the overhead of parsing the headers if
   * they're not needed.
   *
   * @see http://en.wikipedia.org/wiki/HTTP_cookie#Setting_a_cookie
   */
  protected $httpResponseHeaders;

  /**
   * The raw, unparsed HTTP response header string.
   */
  protected $httpRawResponseHeaders;

  /**
   * A string containing the HTTP response, minus headers.
   */
  protected $httpResponseBody;

  /**
   * An array of HTTP status information.
   *
   * The associative array will have the following keys:
   * - raw: The raw status header, eg "HTTP/1.1 404 Not Found"
   * - version: The HTTP version, eg "HTTP/1.1"
   * - code: The numeric status code, eg "404"
   * - status: The textual status description, eg "Not Found"
   */
  protected $httpStatus;

  /**
   * Override the setCurlOpt method to stop clever people from using it to set
   * HTTP headers
   *
   * @throws BetterGetterConfigException
   */
  public function setCurlOpt($option, $value) {
    if ($option === CURLOPT_HTTPHEADER) {
      throw new BetterGetterConfigException('Please use the setRequestHeader() method to set HTTP headers.', self::ERR_USE_SETREQUESTHEADER);
    }
    elseif ($option === CURLOPT_POSTFIELDS) {
      throw new BetterGetterConfigException('Please use the setPostField() or setRawPostData() methods to set POST data.', self::ERR_USE_SETPOSTFIELD);
    }
    return parent::setCurlOpt($option, $value);
  }

  /**
   * Get an outgoing HTTP header's value.
   *
   * @param $header
   *   The outgoing header.
   * @return
   *   The values currently being stored for the header, in a one-dimensional
   *   array; or NULL if none have been defined yet.
   */
  public function getRequestHeader($header) {
    $headers = $this->getRequestHeaders();
    return isset($headers[$header]) ? $headers[$header] : NULL;
  }

  /**
   * Set an outgoing HTTP header.
   *
   * @param $header
   *   The header to set a value for.
   * @param $value
   *   The value to be set for the given header. For convenience, this may be
   *   either a single value or an array; the former for simplicity's sake, and
   *   the latter when it is occasionally necessary.
   * @return
   *   $this, for chaining purposes
   *
   * @see BetterGetterHttp::httpRequestHeaders
   */
  public function setRequestHeader($header, $value) {
    if (is_array($value)) {
      $this->httpRequestHeaders[$header] = $value;
    }
    else {
      $this->httpRequestHeaders[$header] = array($value);
    }
    return $this;
  }

  /**
   * Get all outgoing HTTP headers.
   *
   * @return
   *   An associative array keyed by HTTP headers, with values being
   *   one-dimensional arrays of values for that header.
   */
  public function getRequestHeaders() {
    if ($this->hasExecuted() && empty($this->httpRequestHeaders)) {
      $this->parseRequestHeaders();
    }
    return $this->httpRequestHeaders;
  }

  /**
   * Set multiple outgoing HTTP headers.
   *
   * @param $headers
   *   An associative array keyed by HTTP header, with each value being either a
   *   single value or a one-dimensional array of values for the header.
   * @return
   *   $this, for chaining.
   *
   * @see BetterGetterHttp::setRequestHeader()
   */
  public function setRequestHeaders($headers) {
    foreach ($headers as $header => $value) {
      $this->setRequestHeader($header, $value);
    }
    return $this;
  }

  /**
   * Get a set HTTP post field value.
   *
   * @param $field
   *   The field to return the value for.
   * @return
   *   The set value for the field, or NULL if not set.
   */
  public function getPostField($field) {
    return isset($this->httpPostFields[$field]) ? $this->httpPostFields[$field] : NULL;
  }

  /**
   * Set an HTTP post field value.
   *
   * @param $field
   *   The field to set a value for; or NULL to "unset" a field.
   * @param $value
   *   The value to assign to the field.
   * @return
   *   $this, for chaining.
   */
  public function setPostField($field, $value) {
    if ($value === NULL) {
      unset($this->httpPostFields[$field]);
    }
    else {
      $this->httpPostFields[$field] = $value;
    }
    return $this;
  }

  /**
   * Get all HTTP post fields.
   *
   * @return
   *   An array of all set post fields.
   */
  public function getPostFields() {
    return $this->httpPostFields;
  }

  /**
   * Set multiple post fields at once.
   *
   * @param $fields 
   *   An array keyed by post field, with corresponding values.
   * @return
   *   $this, for chaining.
   *
   * @see BetterGetterHttp::setPostField
   */
  public function setPostFields($fields) {
    foreach ($fields as $field => $value) {
      $this->setPostField($field, $value);
    }
    return $this;
  }

  /**
   * Get the raw post data.
   *
   * Note that this may be NULL even if there was post data as definied by the
   * httpPostFields property (data in key => value format, instead of just a
   * big blob o' data as intended to be used with the httpRawPostData parameter)
   * both before and after a successful query. Care should be taken to check
   * both parameters if necessary when checking for post data.
   *
   * @return
   *   The raw post data as a string, or NULL if none.
   */
   public function getRawPostData() {
    return $this->httpRawPostData;
  }

  /**
   * Set the raw post data.
   *
   * @param $data
   *   The data, as a string (or NULL to "empty out" previous data).
   * @return
   *   $this, for chaining.
   */
  public function setRawPostData($data = NULL) {
    $this->httpRawPostData = $data;
    return $this;
  }

  /**
   * Get the respose body of a successful HTTP response.
   *
   * @return
   *   A string containing the body of the response (or NULL if the cURL handle
   *   hasn't been executed yet).
   */
  public function getResponseBody() {
    return $this->httpResponseBody;
  }

  /**
   * Get all headers returned from a successful HTTP response.
   *
   * @return
   *   An array keyed by header, with the value being a one-dimensional array
   *   of values returned for that header.
   *
   * @see BetterGetterHttp::$httpResponseHeaders
   */
  public function getResponseHeaders() {
    // If we have a response, but the response headers haven't been parsed yet…
    if ($this->rawResponse !== NULL && $this->httpResponseHeaders === NULL) {
      $this->parseResponseHeaders();
    }
    return $this->httpResponseHeaders;
  }

  /**
   * Get all values for a particular HTTP header from an HTTP response.
   *
   * @param $header
   *   The header to return values for.
   * @return
   *   A one-dimensional array of values the server sent for that header; or
   *   NULL if none.
   *
   * @see BetterGetterHttp::httpResponseHeaders
   */
  public function getResponseHeader($header) {
    $headers = $this->getResponseHeaders();
    return isset($headers[$header]) ? $headers[$header] : NULL;
  }

  /**
   * Get all or part of the HTTP status of the response.
   *
   * @param $part
   *   The part of the response to return, which is one of:
   *   - raw: The raw status header, eg "HTTP/1.1 404 Not Found"
   *   - version: The HTTP version, eg "HTTP/1.1"
   *   - code: The numeric status code, eg "404"
   *   - status: The textual status description, eg "Not Found"
   *   - NULL: An associative array with keys and values as above.
   * @return
   *   Either a string containing the part of the HTTP status as selected by the
   *   $part parameter, or an array containing all parts if $part is NULL.
   *
   * @see BetterGetterHttp::httpStatus
   */
  public function getResponseStatus($part = NULL) {
    return empty($part) ? $this->httpStatus : $this->httpStatus[$part];
  }

  /**
   * Format the outgoing HTTP header array into a format the cURL library is
   * expecting and assign them as a cURL option. Also assign POST data, if
   * applicable.
   *
   * This method should probably be called right before the cURL handle is
   * executed.
   *
   * @return
   *   $this, for chaining.
   */
  public function prepareRequest() {
    // Prepare outgoing HTTP headers.
    $headers = array();
    foreach ($this->httpRequestHeaders as $header => $values) {
      foreach ($values as $value) {
        $headers[] = $header . ': ' . $value;
      }
    }
    $this->curlOpts[CURLOPT_HTTPHEADER] = $headers;
    $this->curlOpts[CURLOPT_HEADER] = TRUE;
    // Get the outgoing request string so we can verify what our outgoing headers
    // were.
    $this->curlOpts[CURLINFO_HEADER_OUT] = TRUE;
    // Prepare POST data
    if (!empty($this->httpRawPostData)) {
      $this->curlOpts[CURLOPT_POSTFIELDS] = $this->httpRawPostData;
    }
    elseif (!empty($this->httpPostFields)) {
      $this->curlOpts[CURLOPT_POSTFIELDS] = $this->httpPostFields;
    }
    return $this;
  }

  /**
   * Prepare and execute the cURL handle and parse the response.
   *
   * @return
   *   $this, for chaining.
   */
  public function execute() {
    $this->prepareRequest()
      ->prepareCurlHandle()
      ->executeCurlHandle();
    $this->httpRawRequestHeaders = curl_getinfo($this->curlHandle, CURLINFO_HEADER_OUT);
    $this->httpRequestHeaders = NULL;
    $this->closeCurlHandle()
      ->splitResponse();
  }

  /**
   * Split the HTTP response into body and header parts and handle both
   * appropriately.
   *
   * @return
   *   $this, for chaining.
   *
   * @throws BetterGetterHttpException
   */
  public function splitResponse() {
    // cURL may return several sets of headers when doing HTTP authentication.
    // We'll use the last headers sent as the ones to parse. Note we use
    // preg_split() here instead of explode() so that we can capture the offset
    // of each chunk using PREG_SPLIT_OFFSET_CAPTURE.
    $chunks = preg_split('/\r\n\r\n/', $this->rawResponse, 0, PREG_SPLIT_OFFSET_CAPTURE);
    if (count($chunks) < 2) {
      // No way that was a valid HTTP response. Just give up.
      throw new BetterGetterHttpException('Plausible HTTP headers not found.', self::ERR_HTTP_HEADERS_NOT_EXTRACTIBLE);
    }
    // Find the last chunk that looks like a chunk of HTTP headers.
    foreach ($chunks as $chunk_key => $chunk) {
      // Look for a HTTP response's first line, which will look something like
      // HTTP/1.0 200 OK
      if (preg_match('/^(HTTP\/\d+\.\d+) (\d{3}) (.+)$/m', $chunk[0], $status_matches)) {
        // A failed preg_match() nulls out the variable passed to gather
        // matches, so keep track of our last success.
        $good_status_matches = $status_matches;
      }
      elseif ($chunk_key == 0) {
        // Wait. The first chunk didn't look like a header? Might as well bail
        // out now.
        throw new BetterGetterHttpException('Plausible HTTP headers not found.', self::ERR_HTTP_HEADERS_NOT_EXTRACTIBLE);
      }
      else {
        // Okay, if we didn't find it, then consider this chunk the first piece
        // of the body, and the previous chunk the headers to use.
        $this->httpRawResponseHeaders = $chunks[$chunk_key - 1][0];
        $this->httpResponseBody = substr($this->rawResponse, $chunk[1]);
        break;
      }
    }
    // Did we not find any plausible headers in the response?
    if (empty($this->httpRawResponseHeaders)) {
      throw new BetterGetterHttpException('Plausible HTTP headers not found.', self::ERR_HTTP_HEADERS_NOT_EXTRACTIBLE);
    }
    $this->httpStatus = array(
      // $good_status_matches[0] will include the line break characters, so…
      'raw' => trim($good_status_matches[0]),
      'version' => $good_status_matches[1],
      'code' => $good_status_matches[2],
      'status' => trim($good_status_matches[3]),
    );
    return $this;
  }

  /**
   * Parse the headers of a HTTP response into an easily-accessible array.
   *
   * @return
   *   $this, for chaining.
   *
   * @see BetterGetterHttp::httpResponseHeaders
   */
  public function parseResponseHeaders() {
    $this->httpResponseHeaders = $this->parseHeaderString($this->httpRawResponseHeaders);
    return $this;
  }

  /**
   * Parse the HTTP request headers after a request.
   *
   * @return
   *   $this, for chaining.
   *
   * @see BetterGetterHttp::httpRequestHeaders
   */
  public function parseRequestHeaders() {
    $this->httpRequestHeaders = $this->parseHeaderString($this->httpRawRequestHeaders);
    return $this;
  }

  /**
   * Generically parse HTTP headers into an array.
   *
   * @param $header_string
   *   The string of headers.
   * @return
   *   An associative array of headers, keyed by header and with values of one-
   *   dimensional arrays of values.
   */
  public function parseHeaderString($header_string) {
    $header_lines = explode("\r\n", $header_string);
    $headers = array();
    // The next() trick below is a silly trick to let us skip the first item
    // in the array, which will be the HTTP status line.
    while ($header_line = next($header_lines)) {
      $header_parts = explode(': ', $header_line, 2);
      if (isset($headers[$header_parts[0]])) {
        $headers[$header_parts[0]][] = $header_parts[1];
      }
      else {
        $headers[$header_parts[0]] = array($header_parts[1]);
      }
    }
    return $headers;
  }
}

class BetterGetterHttpException extends BetterGetterException {}
