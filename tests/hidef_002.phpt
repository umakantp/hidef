--TEST--
HIDEF: hidef_fetch - scalars
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
hidef.data_path=./tests/
--FILE--
<?php
function testscalar($name, $value) {
	$a = hidef_fetch($name);
	if($a === $value) echo "$name - OK\n";
	else "$name - FAIL\n";
}

testscalar("null", null);
testscalar("true", true);
testscalar("one", 1);
testscalar("helloworld", "helloworld");
testscalar("pi", pi());
testscalar("doesnotexist", null);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
null - OK
true - OK
one - OK
helloworld - OK
pi - OK
doesnotexist - OK
===DONE===
