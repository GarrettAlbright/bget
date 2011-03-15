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
   * An associative array of headers to be sent as part of the server query,
   * keyed by header field name.
   *
   * Each value is a flat array of values. Why an array instead of a string?
   * Because it's theoretically possible to send headers repeated with different
   * values, though I'm not sure if this is in common use.
   */
  protected $httpHeadersOut = array();

  /**
   * An associative array of headers sent from the server as part of the server
   * response, keyed by header field name.
   *
   * Like with BetterGetterHttp::httpHeadersOut, each value is a flat array of
   * values, since the same field with different browsers may be sent by
   * servers. Unlike with outgoing headers, this practice *is* in common use,
   * particularly when it comes to cookies.
   *
   * @see http://en.wikipedia.org/wiki/HTTP_cookie#Setting_a_cookie
   */
  protected $httpHeadersIn = array();

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
      throw new BetterGetterConfigException('Please use the setHttpHeader() or setHttpHeaders() methods to set HTTP headers.');
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
  public function getHttpHeader($header) {
    return isset($this->httpHeadersOut[$header]) ? $this->httpHeadersOut[$header] : NULL;
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
   * @see BetterGetterHttp::httpHeadersOut
   */
  public function setHttpHeader($header, $value) {
    if (is_array($value)) {
      $this->httpHeadersOut[$header] = $value;
    }
    else {
      $this->httpHeadersOut[$header] = array($value);
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
  public function getHttpHeaders() {
    return $this->httpHeadersOut;
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
   * @see BetterGetterHttp::setHttpHeader()
   */
  public function setHttpHeaders($headers) {
    foreach ($headers as $header => $value) {
      $this->setHttpHeader($header, $value);
    }
    return $this;
  }

  /**
   * Get the respose body of a successful HTTP query.
   *
   * @return
   *   A string containing the response body of the query (or NULL if the cURL
   *   handle hasn't been executed yet).
   */
  public function getResponseBody() {
    return $this->httpResponseBody;
  }

  /**
   * Get all headers returned from a successful HTTP query.
   *
   * @return
   *   An array keyed by header, with the value being a one-dimensional array
   *   of values returned for that header.
   *
   * @see BetterGetterHttp::httpHeadersIn
   */
  public function getResponseHeaders() {
    return $this->httpHeadersIn;
  }

  /**
   * Get all values for a particular HTTP header from an HTTP query response.
   *
   * @param $header
   *   The header to return values for.
   * @return
   *   A one-dimensional array of values the server sent for that header; or
   *   NULL if none.
   *
   * @see BetterGetterHttp::httpHeadersIn
   */
  public function getResponseHeader($header) {
    return isset($this->httpHeadersOut[$header]) ? $this->httpHeadersOut[$header] : NULL;
  }

  /**
   * Get all or part of the HTTP status of the query response.
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
   * expecting and assign them as a cURL option.
   *
   * This method should probably be called right before the cURL handle is
   * executed.
   *
   * @return
   *   $this, for chaining.
   */
  public function prepareHttpHeaders() {
    $headers = array();
    foreach ($this->httpHeadersOut as $header => $values) {
      foreach ($values as $value) {
        $headers[] = $header . ': ' . $value;
      }
    }
    $this->curlOpts[CURLOPT_HTTPHEADER] = $headers;
    $this->curlOpts[CURLOPT_HEADER] = TRUE;
    // Get the outgoing query string so we can verify what our outgoing headers
    // were.

    return $this;
  }

  /**
   * Prepare and execute the cURL handle and parse the response.
   *
   * @return
   *   $this, for chaining.
   */
  public function execute() {
    return $this->prepareHttpHeaders()->prepareCurlHandle()->executeCurlHandle()->parseResponseHeaders();
  }

  /**
   * Split the HTTP response into body and header parts and handle both
   * appropriately.
   *
   * @return
   *  $this, for chaining.
   */
  public function parseResponseHeaders() {
    // cURL may return several sets of headers when doing HTTP authentication.
    // We'll use the last headers sent as the ones to parse.
    $chunks = preg_split('/\n\n|\r\n\r\n/', $this->rawResponse, 0, PREG_SPLIT_OFFSET_CAPTURE);
    // Find the last chunk that looks like a chunk of HTTP headers.
    foreach ($chunks as $chunk_key => $chunk) {
      // Look for a HTTP response's first line, which will look something like
      // HTTP/1.0 200 OK
      if (preg_match('/^(HTTP\/\d+\.\d+) (\d{3}) (.+)$/m', $chunk[0], $status_matches)) {
        // A failed preg_match() nulls out the variable passed to gather
        // matches, so keep track of our last success.
        $good_status_matches = $status_matches;
      }
      else {
        // Okay, if we didn't find it, then consider this chunk the first piece
        // of the body, and the previous chunk the headers to use.
        $unparsed_headers = $chunks[$chunk_key - 1][0];
        $this->httpResponseBody = substr($this->rawResponse, $chunk[1]);
        break;
      }
    }
    if (!isset($unparsed_headers)) {
      throw new BetterGetterHttpException('Plausible HTTP headers not found.');
    }
    $this->httpStatus = array(
      'raw' => $good_status_matches[0],
      'version' => $good_status_matches[1],
      'code' => $good_status_matches[2],
      'status' => $good_status_matches[3],
    );

    $header_lines = preg_split('/\n|\r\n/', $unparsed_headers);
    // The next() trick below is a silly trick to let us skip the first item
    // in the array, which will be the HTTP status line.
    while ($header_line = next($header_lines)) {
      $header_parts = explode(': ', $header_line, 2);
      if (isset($this->httpHeadersIn[$header_parts[0]])) {
        $this->httpHeadersIn[$header_parts[0]][] = $header_parts[1];
      }
      else {
        $this->httpHeadersIn[$header_parts[0]] = array($header_parts[1]);
      }
    }
    return $this;
  }
}

class BetterGetterHttpException extends Exception {}
