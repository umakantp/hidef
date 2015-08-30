--TEST--
HIDEF: hidef thaw memory buildup tests
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
hidef.data_path=./tests/
--FILE--
<?php
function test_thaw(&$m2)
{
	$a = hidef_fetch("array2");
	$b = $a->thaw($n);
	$m2 = memory_get_usage();
}

function test_cast(&$m2)
{
	$a = hidef_fetch("array2");
	$b = (array)($a);
	$m2 = memory_get_usage();
}

function test_array_exists(&$m2)
{
	$a = hidef_fetch("array2");
	array_key_exists("foo", $a);
	$m2 = memory_get_usage();
}

function measure_memory_usage($f)
{
	$m1 = 0;
	$m2 = 0;
	$m3 = 0;
	$m1 = memory_get_usage();

	call_user_func($f, &$m2);

	$m3 = memory_get_usage();

	$d1 = ($m2 - $m1);
	$d2 = ($m3 - $m1);
	print "$f: Churn = $d1\n";
	print "$f: Buildup = $d2\n";
}

measure_memory_usage('test_thaw');
measure_memory_usage('test_cast');
measure_memory_usage('test_array_exists');

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
test_thaw: Churn = %d
test_thaw: Buildup = 0
test_cast: Churn = %d
test_cast: Buildup = 0
test_array_exists: Churn = %d
test_array_exists: Buildup = 0
===DONE===
