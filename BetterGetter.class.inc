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
  protected $curlHandle;

  /**
   * The URI of the resource to be fetched.
   */
  protected $uri;

  /**
   * An array of cURL library options.
   *
   * @see http://php.net/curl_setopt
   */
  protected $curlOpts = array();

  /**
   * The raw server response returned by curl_exec().
   *
   * This will be NULL if the cURL handle has not been executed yet, or FALSE
   * if an error happened during execution. In other cases, it will be a string.
   */
  protected $rawResponse;
  
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
  public function __construct($uri = NULL) {
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
  
  /**
   * Getter for cURL library options.
   *
   * @param $option
   *   The cURL option constant to return the set value for.
   * @return
   *   The set value, or NULL if not set.
   *
   * @see http://php.net/curl_setopt
   */
  public function getCurlOpt($option) {
    return isset($this->curlOpts[$option]) ? $this->curlOpts[$option] : NULL;
  }
  
  /**
   * Setter for cURL library options.
   *
   * @param $ption
   *   The cURL option constant to set a value for. If a value has already been
   *   set for this option, it will be overwritten.
   * @param $value
   *   The value to set for the given option.
   * @return
   *   $this, for chaining purposes.
   *
   * @see http://php.net/curl_setopt
   */
  public function setCurlOpt($option, $value) {
    $this->curlOpts[$option] = $value;
    return $this;
  }

  /**
   * Get all set cURL options.
   *
   * @return
   *   An associative array keyed by the cURL option constant and with the
   *   associated set value.
   */
  public function getCurlOpts() {
    return $this->curlOpts;
  }

  /**
   * Set many cURL options at once.
   *
   * @param $options
   *   An array, keyed by cURL option constants and with corresponding values.
   *   Any previously set values for given options will be overwritten.
   */
  public function setCurlOpts($options) {
    $this->curlOpts = array_merge($this->curlOpts, $options);
    return $this;
  }

  public function getRawResponse() {
    return $this->rawResponse;
  }

  /**
   * Prepare and execute the cURL handle and set the response.
   */
  public function execute() {
    return $this->prepareCurlHandle()->executeCurlHandle();
  }
  
  /**
   * Prepare the cURL handle for execution.
   *
   * Set the desired cURL options on the handler.
   */
  public function prepareCurlHandle() {
    $this->curlHandler = curl_init($this->uri);
    $this->curlOpts[CURLOPT_RETURNTRANSFER] = TRUE;
    curl_setopt_array($this->curlHandler, $this->curlOpts);
    return $this;
  }

  /**
   * Execute and close the cURL handle and set the rawResponse property.
   *
   * @throws BetterGetterCurlException
   * @return
   *   $this, for chaining.
   */
  public function executeCurlHandle() {
    $this->rawResponse = curl_exec($this->curlHandler);
    if ($this->rawResponse === FALSE) {
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

/**
 * Simple exception specific to Better Getter errors.
 */
class BetterGetterCurlException extends Exception {}