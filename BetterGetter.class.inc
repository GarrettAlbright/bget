<?php

/**
 * Basic Better Getter class.
 *
 * As it is, this class is basically an OOP wrapper around PHP's basic cURL
 * library functions. By itself, that may be useful, particularly if you abhor
 * functional programming; that aside, however, using this class by itself
 * provides little beneift over using the cURL functions directly, at the cost
 * of added computational and memory overhead. It's intended that this class is
 * overrided and methods added or overridden to make it more useful. The
 * BetterGetterHttp class is an example of that.
 */
class BetterGetter {

  /**
   * The cURL handle as returned from curl_init().
   *
   * This will be NULL if the cURL handle has not yet been initialized.
   *
   * @see http://php.net/curl_init
   * @see BetterGetter::prepareCurlHandle()
   */
  protected curlHandle;

  /**
   * The URI of the resource to be fetched.
   */
  protected uri;

  /**
   * An array of cURL library options.
   *
   * @see http://php.net/curl_setopt
   */
  protected curlOpts = array();

/*
  protected httpHeadersOut = array();
  protected httpHeadersIn = array();
*/

  /**
   * The raw server response returned by curl_exec().
   *
   * This will be NULL if the cURL handle has not been executed yet, or FALSE
   * if an error happened during execution. In other cases, it will be a string.
   */
  protected rawResponse;
  
  /**
   * Constructor.
   *
   * @param $uri
   *   The URI to connect to may be passed to the constructor for convenience.
   *   Thus, the following examples are functionally equivalent:
   *   @code
   *     $bg = new BetterGetter('http://example.com/');
   *
   *     $bg = new BetterGetter();
   *     $bg->setUri('http://example.com/');
   *   @endcode
   */
  protected function __construct($uri = NULL) {
    if (!empty($uri)) {
      $this->setUri($uri);
    }
  }

  /**
   * Getter for the URI member variable.
   */  
  public function getUri() {
    return $this->uri;
  }
  
  /**
   * Setter for the URI member variable.
   *
   * @param $uri
   *   The URI to connect to.
   * @return
   *   $this, for chaining purposes.
   */
  public function setUri($uri) {
    $this->uri = $uri;
    return $this;
  }
  

  public function getCurlOpt($option) {
    return isset($this->curlOpts[$option]) ? $this->curlOpts[$option] : NULL;
  }
  
  public function setCurlOpt($option, $value) {
    $this->curlOpts[$option] = $value;
    return $this;
  }
  
  public function getCurlOpts() {
    return $this->curlOpts;
  }
  
  public function setCurlOpts($options) {
    $this->curlOpts = array_merge($this->curlOpts, $options);
    return $this;
  }
  
/*
  function getHttpHeader($header) {
    return isset($this->httpHeadersOut[$header]) ? $this->httpHeadersOut[$header] : NULL;
  }
  
  function setHttpHeader($header, $value) {
    $this->httpHeadersOut[$header] = $value;
    return $this;
  }
  
  function getHttpHeaders() {
    return $this->httpHeadersOut;
  }
  
  function setHttpHeaders($headers) {
    $this->httpHeadersOut = array_merge($this->httpHeadersOut, $headers);
    return $this;
  }
*/
  
  public function execute() {
    $this->prepareCurlHandle()->executeCurlHandle();
  }
  
  public function prepareCurlHandle() {
    $this->curlHandler = curl_init($this->uri);
    $this->curlOpts[CURLOPT_RETURNTRANSFER] = TRUE;
    curl_setopt_array($this->curlHandler, $this->curlOpts);    
  }

  public function executeCurlHandle() {
    if (($this->rawResponse = curl_exec($this->curlHandler)) === FALSE) {
      // Get error number and handler
      $curl_errno = curl_errno($this->curlHandler);
      $curl_error = curl_error($this->curlHandler);
    }
    // Close the handler
    curl_close($this->curlHandler);
    // Throw an exception if cURL errored out. We don't do it above so that we
    // can still close the cURL handler before throwing the error.
    if ($this->rawResponse === FALSE) {
      throw new BetterGetterCurlException($curl_error, $curl_errno);
    }
    return $this;
  }
}
    
class BetterGetterCurlException extends Exception {}