--TEST--
HIDEF: test constant replacements
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
hidef.ini_path=./tests/
hidef.data_path=.
--FILE--
<?php
echo "XYZ=".XYZ."\n";
echo "ABC=".ABC."\n";
echo "PIE=".PIE."\n";
echo "ART=".ART."\n";
echo "bleh=".bleh."\n";
echo "simplified=".simplified."\n";
echo "unescaped=".unescaped."\n";
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
XYZ=-191
ABC=xyz
PIE=3.1419
ART=1
bleh=<><<MLM<>>
simplified=We're all really strings; Even if we're punctuated.
unescaped=will this always work, I dont know.
===DONE===
