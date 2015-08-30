--TEST--
HIDEF: hidef_fetch - simple array
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
hidef.data_path=./tests/
--FILE--
<?php
$a = hidef_fetch("array1");
echo "the answer is {$a['answer']}\n";
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
the answer is 42
===DONE===
