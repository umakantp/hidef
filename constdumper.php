<?php
/*
  +----------------------------------------------------------------------+
  | Hidef                                                                |
  +----------------------------------------------------------------------+
  | Copyright (c) 2008 The PHP Group                                     |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Gopal Vijayaraghavan <gopalv@php.net>                       |
  +----------------------------------------------------------------------+
*/

/* $Id: constdumper.php 264685 2008-08-12 11:05:31Z gopalv $ 
 *
 * This file has to be added to auto_append_file in php.ini
 */

$dumpfile = tempnam('/tmp','hidef.');
$constants     = get_defined_constants(true);
$defines       = $constants['user'];
$fp = fopen($dumpfile, 'w');
fwrite($fp, serialize($defines));
fclose($fp);
error_log("written constants to $dumpfile");
?>
