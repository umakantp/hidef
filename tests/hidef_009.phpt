--TEST--
HIDEF: hidef set exception
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
hidef.data_path=./tests/
--FILE--
<?php
$a = hidef_fetch("array2");

try
{
	$a[] = null;
}
catch(Exception $e)
{
	echo "Caught set exception\n";
	echo "Message:".$e->getMessage()."\n";
}

try
{
	unset($a[0]);
}
catch(Exception $e)
{
	echo "Caught unset exception\n";
	echo "Message:".$e->getMessage()."\n";
}

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Caught set exception
Message:Cannot set elements of a FrozenArray
Caught unset exception
Message:Cannot unset elements of a FrozenArray
===DONE===
