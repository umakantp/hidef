--TEST--
HIDEF: hidef count() tests
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
hidef.data_path=./tests/
--FILE--
<?php
echo "array1 has ".count(hidef_fetch("array1"))." element\n";

$a = hidef_fetch("array2");
echo "array2 has ".count($a)." elements\n";
$a = $a->thaw();
echo "array2->thaw() has ".count($a)." elements\n";
$sub = $a[6];
echo "array2[6] has ".count($sub)." elements\n";
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
array1 has 1 element
array2 has 7 elements
array2->thaw() has 7 elements
array2[6] has 3 elements
===DONE===
