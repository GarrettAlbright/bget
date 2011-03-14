<?php

class BetterGetterGopher extends BetterGetter {

  protected $contentType;

  public function setUri($uri) {
    $url_parts = parse_url($uri);
    if (isset($url_parts['path']) && isset($url_parts['path']{1})) {
      $this->contentType = $url_parts['path']{1};
    }
    else {
      // Assume it's a directory.
      $this->contentType = '1';
    }
    return parent::setUri($uri);
  }

  public function getDirectoryListing() {
    if ($this->contentType !== '1') {
      throw new BetterGetterGopherException('Attempting to parse directory listing response when response is probably not a directory listing');
    }
    $rows = array();
    $lines = explode("\r\n", $this->rawResponse);
    // Proper Gopher protocol requries that the last line is a full stop/dot
    // character. However, servers don't always adhere to this. For our
    // purposes, we'll just get rid of that line anyway.
    if (end($lines) === '.') {
      array_pop($lines);
    }
    reset($lines);
    foreach ($lines as $line) {
      $line_parts = explode("\t", $line);
      $rows[] = array(
        'type' => substr($line_parts[0], 0, 1),
        'display' => substr($line_parts[0], 1),
        'selector' => $line_parts[1],
        'host' => $line_parts[2],
        'port' => $line_parts[3],
      );
    }
    return $rows;
  }

  public function getContentType() {
    return $this->contentType !== NULL ? $this->contentType : NULL;
  }
}

class BetterGetterGopherException extends Exception {}
