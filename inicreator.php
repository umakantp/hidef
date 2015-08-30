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

/* $Id: inicreator.php 264689 2008-08-12 11:27:31Z gopalv $ 
 * Usage :
 *   php dump_parser.php -f <yourdumpedfile> -t ini|php -o outputfile 
 */

/* usage {{{ */
function usage() 
{
	echo "{$GLOBALS['argv'][0]} -f <filename> [-t ini|php] [-o <output_file>]\n";
	exit(1);
}
/* }}} */

/* dump_ini {{{ */
function dump_ini($prefix, $data) {

	$fp = fopen("$prefix.ini", "wb");

	$dataTypeHash = array(
			'integer' => 'int',
			'double'  => 'float',
			'float'   => 'float',
			'string'  => 'str',
			'boolean' => 'bool'
			);

	$output = "[hidef]\n";
	foreach($data as $k=>$v) 
	{
		$dataType  = (string) gettype($v);
		$v = var_export($v, true);
		if($dataType == 'string')
		{
			$output.= "{$dataTypeHash[$dataType]} $k = '$v';\n";
		}
		else 
		{
			if(empty($v)) $v = 0;
			$output.= "{$dataTypeHash[$dataType]} $k = $v;\n";
		}

	}

	fwrite($fp, $output);
	error_log("Written file $prefix.ini");
	fclose($fp);
}
/* }}} */

/* dump_php {{{ */
function dump_php($prefix, $data) 
{
	$fp = fopen("$prefix.php", "wb");

	$output = <<<EOF
# Use as an auto_prepend_file for cases when hidef is disabled
		<?php 
		if(!extension_loaded('hidef')) 
		{

			EOF;
			foreach($data as $k=>$v) 
			{
				$dataType = (string) gettype($v);
				$v = var_export($v, true);
				if($dataType == 'string') 
				{
					$output.= "define('$k', '$v');\n";
				}
				else 
				{
					if(empty($v)) $v = 0;
					$output.= "define('$k', $v);\n";
				}
			}

			$output.=<<<EOF
		}
	EOF;
	fwrite($fp, $output);
	error_log("Written file $prefix.php");
	fclose($fp);
}
/* }}}*/

$options = getopt("f:p:");

if(!isset($options['f'])) usage();
else $infile = $options['f'];

if(!isset($options['p'])) $prefix = "hidef.";
else $prefix = $options['p'];

$data = unserialize(file_get_contents($infile));

dump_ini($prefix, $data);
dump_php($prefix, $data);

?>
