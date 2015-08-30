--TEST--
HIDEF: test hidef.per_request_ini 
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
hidef.data_path=none
hidef.ini_path=none
hidef.per_request_ini=tests/hidef_test.ini
--FILE--
<?php
echo "XYZ=".XYZ."\n";
echo "ABC=".ABC."\n";
echo "PIE=".PIE."\n";
echo "ART=".ART."\n";
echo "bleh=".bleh."\n";
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
XYZ=-191
ABC=xyz
PIE=3.1419
ART=1
bleh=<><<MLM<>>
===DONE===
