<?php

class BetterGetterHttp extends BetterGetter {

  protected httpHeadersOut = array();
  protected httpHeadersIn = array();

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
}
