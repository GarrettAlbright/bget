<?php

/**
 * These tests use the SimpleTest testing library. To run them, download
 * SimpleTest 1.0 (as of the time of this writing, the 1.1alpha version seems
 * be broken) and place the "simpletest" directory in the "test" directory that
 * this file is in. Then run this file, either in a browser or on the command
 * line (php -f bget.test.php).
 */

/**
 * Define a URL of an accessible site to use for testing. Please only use your
 * own web sites! They don't have to give any response in particular, but they
 * should both accept and reply with a valid HTTP response. There should be a 
 * slash at the end of the URL.
 */
define('BGET_TEST_SITE_URL', 'http://localhost/');

define('BGET_TEST_BASE_DIR', dirname(__FILE__));

// Require Simpletest
require BGET_TEST_BASE_DIR . '/simpletest/autorun.php';

require BGET_TEST_BASE_DIR . '/../includes/Bget.class.inc';
require BGET_TEST_BASE_DIR . '/../includes/BgetHttp.class.inc';

class BgetTestCase extends UnitTestCase {
  function getInfo() {
    return array(
      'name' => 'Better Getter tests',
      'description' => 'Tests for Better Getter',
      'group' => 'Better Getter',
    );
  }

  /**
   * Test getting and setting the URI to access.
   */
  function testGetSetUri() {
    $bg = new Bget(BGET_TEST_SITE_URL);
    $this->assertEqual($bg->getUri(), BGET_TEST_SITE_URL, 'Setting URI with constructor method and fetching with getUri() method');
    $foourl = BGET_TEST_SITE_URL . 'foo';
    $this->assertEqual($bg->setUri($foourl)->getUri(), $foourl, 'Setting URI with setUri() method and fetching with getUri() method');
  }

  /**
   * Test getting and setting cURL library options in various ways.
   */
  function testGetSetCurlOpts() {
    $bg = new Bget(BGET_TEST_SITE_URL);
    $opts = array(
      CURLOPT_AUTOREFERER => TRUE,
      CURLOPT_USERAGENT => 'Better Getter',
      CURLOPT_USERPWD => 'foo:bar',
      CURLOPT_HTTPPROXYTUNNEL => FALSE,
    );
    $bg->setCurlOpt(CURLOPT_AUTOREFERER, $opts[CURLOPT_AUTOREFERER])->setCurlOpt(CURLOPT_USERAGENT, $opts[CURLOPT_USERAGENT]);
    $this->assertEqual($bg->getCurlOpt(CURLOPT_USERAGENT), $opts[CURLOPT_USERAGENT], 'Setting single options with setCurlOpt() method and getting with getCurlOpt() method');
    $this->assertEqual(
      $bg->getCurlOpts(),
      array(
        CURLOPT_AUTOREFERER => $opts[CURLOPT_AUTOREFERER],
        CURLOPT_USERAGENT => $opts[CURLOPT_USERAGENT],
      ),
      'Fetching multiple options with getCurlOpts() method'
    );
    unset($bg);
    $bg = new Bget(BGET_TEST_SITE_URL);
    $bg->setCurlOpts($opts);
    $this->assertEqual($bg->getCurlOpts(), $opts, 'Setting multiple options with setCurlOpts() method');
  }

  /**
   * Test executing a cURL handle.
   */
  function testExecution() {
    $bg = new Bget(BGET_TEST_SITE_URL);
    $this->assertTrue($bg->execute()->getRawResponse(), 'Simple execution for site front page returns non-empty response');

    $message = 'BgetCurlException thrown predictably for invalid URI';
    $bg = new Bget();
    try {
      $bg->execute();
      $this->fail($message);
    }
    catch (Exception $e) {
      if (is_a($e, 'BgetCurlException') && $e->getCode() == 3) {
        $this->pass($message);
      }
      else {
        $this->fail($message);
      }
    }
  }
}

class BgetHttpTestCase extends UnitTestCase {
  function getInfo() {
    return array(
      'name' => 'Better Getter HTTP tests',
      'description' => 'Tests for Better Getter HTTP class',
      'group' => 'Better Getter',
    );
  }

  function testSetCurlOpt() {
    $message = 'Setting CURLOPT_HTTPHEADER cURL option throws BgetConfigException';
    $bg = new BgetHttp(BGET_TEST_SITE_URL);
    try {
      $bg->setCurlOpt(CURLOPT_HTTPHEADER, array());
      $this->fail($message);
    }
    catch (Exception $e) {
      if (is_a($e, 'BgetConfigException') && $e->getCode() == BgetHttp::ERR_USE_SETREQUESTHEADER) {
        $this->pass($message);
      }
      else {
        $this->fail($message);
      }
    }

    $message = 'Setting CURLOPT_POSTFIELDS cURL option throws BgetConfigException';
    $bg = new BgetHttp(BGET_TEST_SITE_URL);
    try {
      $bg->setCurlOpt(CURLOPT_POSTFIELDS, array('foo' => 'bar'));
      $this->fail($message);
    }
    catch (Exception $e) {
      if (is_a($e, 'BgetConfigException') && $e->getCode() == BgetHttp::ERR_USE_SETPOSTFIELD) {
        $this->pass($message);
      }
      else {
        $this->fail($message);
      }
    }

  }

  function testResponseSplitting() {
    $test_response = 'We the People of the United States, in Order to form a more perfect Union, establish Justice, insure domestic Tranquility, provide for the common defence, promote the general Welfare, and secure the Blessings of Liberty to ourselves and our Posterity, do ordain and establish this Constitution for the United States of America.';
    $bg = new BgetHttp(BGET_TEST_SITE_URL);
    $bg->setRawResponse($test_response);
    $this->assertEqual($bg->getRawResponse(), $test_response, 'getRawResponse() and setRawResponse() methods working predictably');
    $bg = new BgetHttp(BGET_TEST_SITE_URL);
    $message = 'Throw BgetHttpException when parsing empty response';
    try {
      $bg->setRawResponse('')->splitResponse();
      $this->fail($message);
    }
    catch (Exception $e) {
      if (is_a($e, 'BgetHttpException') && $e->getCode() == BgetHttp::ERR_HTTP_HEADERS_NOT_EXTRACTIBLE) {
        $this->pass($message);
      }
      else {
        $this->fail($message);
      }
    }
    $bg = new BgetHttp(BGET_TEST_SITE_URL);
    $message = 'Throw BgetHttpException when parsing multi-line but invalid response';
    try {
      $bg->setRawResponse("This is a totally invalid HTTP response.\r\nTotally invalid.\r\n\r\nYeah. This should fail.")->splitResponse();
      $this->fail($message);
    }
    catch (Exception $e) {
      if (is_a($e, 'BgetHttpException') && $e->getCode() == BgetHttp::ERR_HTTP_HEADERS_NOT_EXTRACTIBLE) {
        $this->pass($message);
      }
      else {
        $this->fail($message);
      }
    }
    // http://en.wikipedia.org/wiki/List_of_HTTP_status_codes#418
    $body = "<!doctype html>\r\n<html>\r\n<head><title>A silly test page.</title></head>\r\n\r\n<body><h1>PARTY TIME!</h1></body>\r\n</html>";
    $valid_response = "HTTP/1.1 418 I'm a teapot\r\nDate: Tue, 15 Mar 2011 21:49:17 GMT\r\nServer: albrighttpd/1.2.34\r\nConnection: close\r\n\r\n" . $body;
    $message = 'Parsing valid response did not fail.';
    $bg = new BgetHttp('http://example.com/');
    try {
      $bg->setRawResponse($valid_response)->splitResponse();
      if ($bg->getResponseStatus('status') === 'I\'m a teapot' &&
        $bg->getResponseHeader('Server') === array('albrighttpd/1.2.34') &&
        $bg->getResponseBody() === $body
      ) {
        $this->pass($message);
      }
      else {
        $this->fail($message);
      }
    }
    catch (Exception $e) {
      $this->fail($message);
    }
  }

  function testRequestHeaderHandling() {
    $bg = new BgetHttp(BGET_TEST_SITE_URL);
    $headers = array(
      'User-Agent' => 'Bget',
      'X-Foo' => array('Bar', 'Baz'),
    );
    $bg->setRequestHeader('User-Agent', $headers['User-Agent']);
    $this->assertEqual($bg->getRequestHeader('User-Agent'), array($headers['User-Agent']), 'Getting and setting a single header');
    $bg->setRequestHeaders($headers);
    $this->assertEqual($bg->getRequestHeader('X-Foo'), $headers['X-Foo'], 'Setting multiple headers with getRequestHeader()');
    $this->assertEqual($bg->getRequestHeaders(), array('User-Agent' => array($headers['User-Agent']), 'X-Foo' => $headers['X-Foo']), 'Getting multiple headers with getRequestHeaders');
    $bg = new BgetHttp(BGET_TEST_SITE_URL);
    $bg->setCurlOpt(CURLOPT_USERAGENT, 'Bget')->execute();
    $this->assertEqual($bg->getRequestHeader('User-Agent'), array('Bget'), 'Parsing request headers after successful request');
  }

  function testPostDataHandling() {
    $data = array(
      'foo' => 'bar',
      'bar' => 'baz',
      'baz' => 'qux',
    );
    $bg = new BgetHttp(BGET_TEST_SITE_URL);
    $bg->setPostField('foo', $data['foo']);
    $this->assertEqual(
      $bg->getPostField('foo'),
      $data['foo'],
      'Setting and getting single POST field with setPostField() and getPostField() methods'
    );
    $bg->setPostFields(array('bar' => $data['bar'], 'baz' => $data['baz']));
    $this->assertEqual(
      $bg->getPostFields(),
      $data,
      'Setting and getting multiple POST fields with setPostFields() and getPostFields()'
    );
    $bg->setPostField('baz', NULL);
    $this->assertEqual(
      $bg->getPostFields(),
      array('foo' => $data['foo'], 'bar' => $data['bar']),
      'Unsetting a post field'
    );
    $bg->setRawPostData('foobarbaz');
    $this->assertEqual(
      $bg->getRawPostData(),
      'foobarbaz',
      'Setting and getting raw POST data with setRawPostData() and getRawPostData()'
    );
    $bg->setRawPostData(NULL);
    $bg->execute();
    // Unfortunately there doesn't seem to be a precise way to test that POST
    // data is being sent to the server correctly unless we can do it from the
    // server's end. But we can at least test that cURL is doing a POST query
    // and thinks it's sending some POST data, like this:
    $post_check_msg = 'POST data added to query';
    $ct = $bg->getRequestHeader('Content-Type');
    if ($ct && strpos($ct[0], 'multipart/form-data') === 0) {
      $this->pass($post_check_msg);
    }
    else {
      $this->fail($post_check_msg);
    }
  }
}
