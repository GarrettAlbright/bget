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
   * Returns whether the cURL handle has been executed yet.
   *
   * @return
   *   A boolean representing whether the cURL handle has been executed.
   */
  public function hasExecuted() {
    return !is_null($this->rawResponse);
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
   * @return
   *   $this, for chaining.
   */
  public function setCurlOpts($options) {
    foreach ($options as $option => $value) {
      $this->setCurlOpt($option, $value);
    }
    return $this;
  }

  /**
   * Get the raw server response.
   *
   * @return
   *   The string representing the data returned from the server; or FALSE if
   *   the server connection failed; or NULL if we haven't executed the cURL
   *   handle yet (we haven't tried contacting the server yet).
   */
  public function getRawResponse() {
    return $this->rawResponse;
  }

  /**
   * Set the raw server response.
   *
   * In normal usage, this doesn't have a practical application, since the
   * entire idea of using something like this is to fetch data from a remote
   * server. However, this can be useful for testing purposes, particularly
   * testing stuff that parses the response string.
   *
   * @param $response
   *   The string to be set as the response string.
   * @return
   *   $self, for chaining purposes.
   */
  public function setRawResponse($response) {
    $this->rawResponse = $response;
    return $this;
  }

  /**
   * Prepare and execute the cURL handle and set the response.
   *
   * @return
   *   $this, for chaining.
   */
  public function execute() {
    return $this->prepareCurlHandle()->executeCurlHandle()->closeCurlHandle();
  }

  /**
   * Prepare the cURL handle for execution.
   *
   * Set the desired cURL options on the handler.
   *
   * @return
   *   $this, for chaining.
   */
  public function prepareCurlHandle() {
    $this->curlHandle = curl_init($this->uri);
    $this->curlOpts[CURLOPT_RETURNTRANSFER] = TRUE;
    curl_setopt_array($this->curlHandle, $this->curlOpts);
    return $this;
  }

  /**
   * Execute the cURL handle and set the rawResponse property.
   *
   * @throws BetterGetterCurlException
   * @return
   *   $this, for chaining.
   */
  public function executeCurlHandle() {
    if (($this->rawResponse = curl_exec($this->curlHandle)) === FALSE) {
      // Get error number and handler
      $curl_errno = curl_errno($this->curlHandle);
      $curl_error = curl_error($this->curlHandle);
      $this->closeCurlHandle();
      throw new BetterGetterCurlException($curl_error, $curl_errno);
    }
    return $this;
  }

  /**
   * Close the cURL handle.
   *
   * @return
   *   $this, for chaining.
   */
  public function closeCurlHandle() {
    curl_close($this->curlHandle);
    return $this;
  }
}

/**
 * Simple exceptions specific to Better Getter errors.
 */
class BetterGetterException extends Exception {}

class BetterGetterCurlException extends BetterGetterException {}

class BetterGetterConfigException extends BetterGetterException {}
