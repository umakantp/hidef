--TEST--
HIDEF: ensure serialization and unserialization of FrozenArrays fail 
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
hidef.data_path=./tests/
--FILE--
<?php
$a = hidef_fetch("array2");
try {
	serialize($a);
	print "Serialize(FrozenArray) is not supposed to work\n";
}
catch(Exception $e){
	print "Serialize(FrozenArray) throws an exception. All is good\n";
}

/*
This remains commented till https://bugs.php.net/bug.php?id=55303 is fixed
$s = 'O:11:"FrozenArray":3:{i:1;s:3:"xyz";i:2;s:3:"abc";i:0;s:5:"hello";}';
try {
	unserialize($s);
	print "unserialize is not supposed to work!\n";
}
catch(Exception $e){
}
*/

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Serialize(FrozenArray) throws an exception. All is good
===DONE===
