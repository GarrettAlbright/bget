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
  
  protected $httpResponseBody;

  protected $httpStatus;

  public function setCurlOpt($option, $value) {
    if ($option === CURLOPT_HTTPHEADER) {
      throw new BetterGetterConfigException('Please use the setHttpHeader() or setHttpHeaders() methods to set HTTP headers.');
    }
    return parent::setCurlOpt($option, $value);
  }

  public function setCurlOpts($options) {
    if (isset($options[CURLOPT_HTTPHEADER])) {
      throw new BetterGetterConfigException('Please use the setHttpHeader() or setHttpHeaders() methods to set HTTP headers.');
    }
    return parent::setCurlOpts($options);
  }
  
  function getHttpHeader($header) {
    return isset($this->httpHeadersOut[$header]) ? $this->httpHeadersOut[$header] : NULL;
  }
  
  function setHttpHeader($header, $value) {
    if (is_array($value)) {
      $this->httpHeadersOut[$header] = $value;
    }
    else {
      $this->httpHeadersOut[$header] = array($value);
    }
    return $this;
  }
  
  function getHttpHeaders() {
    return $this->httpHeadersOut;
  }
  
  function setHttpHeaders($headers) {
    foreach ($headers as $header => $value) {
      $this->setHttpHeader($header, $value);
    }
    return $this;
  }

  function getResponseBody() {
    return $this->httpResponseBody;
  }

  function getResponseHeaders() {
    return $this->httpHeadersIn;
  }

  function getResponseStatus($part = NULL) {
    return empty($part) ? $this->httpStatus : $this->httpStatus[$part];
  }

  function getResponseHeader($header) {
    return isset($this->httpHeadersOut[$header]) ? $this->httpHeadersOut[$header] : NULL;
  }

  function prepareHttpHeaders() {
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

  function execute() {
    return $this->prepareHttpHeaders()->prepareCurlHandle()->executeCurlHandle()->parseResponseHeaders();
  }
  
  function parseResponseHeaders() {
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
