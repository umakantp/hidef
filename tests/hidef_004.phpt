--TEST--
HIDEF: hidef_fetch() - complex array
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
hidef.data_path=./tests/
--FILE--
<?php
$a = hidef_fetch("array2");

echo $a."\n";

echo '"0" => '.$a[0]."\n";
echo '"1" => '.$a[1]."\n";
echo '"2" => '.$a[2]."\n";
echo '"3" => '.$a[3]."\n";
echo '"4" => '.$a[4]."\n";
echo '"5" => '.$a[5]."\n";

$sub = $a[6];

echo $sub."\n";

echo '"one" => '.$sub["one"]."\n";
echo '"two" => '.$sub["two"]."\n";
echo '"three" => '.$sub["three"]."\n";
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
FrozenArray[7]
"0" => 1
"1" => 
"2" => 0
"3" => 1
"4" => 3.14
"5" => hello world
FrozenArray[3]
"one" => 1
"two" => 2
"three" => 3
===DONE===
