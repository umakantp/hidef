--TEST--
HIDEF: hidef thaw - complex array
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
hidef.data_path=./tests/
--FILE--
<?php
$a = hidef_fetch("array2");
$a = $a->thaw();

var_dump($a);

echo '"0" => '.$a[0]."\n";
echo '"1" => '.$a[1]."\n";
echo '"2" => '.$a[2]."\n";
echo '"3" => '.$a[3]."\n";
echo '"4" => '.$a[4]."\n";
echo '"5" => '.$a[5]."\n";

$sub = $a[6];

var_dump($sub);

echo '"one" => '.$sub["one"]."\n";
echo '"two" => '.$sub["two"]."\n";
echo '"three" => '.$sub["three"]."\n";
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
array(7) {
  [0]=>
  bool(true)
  [1]=>
  bool(false)
  [2]=>
  int(0)
  [3]=>
  int(1)
  [4]=>
  float(3.14)
  [5]=>
  string(11) "hello world"
  [6]=>
  array(3) {
    ["one"]=>
    int(1)
    ["two"]=>
    int(2)
    ["three"]=>
    int(3)
  }
}
"0" => 1
"1" => 
"2" => 0
"3" => 1
"4" => 3.14
"5" => hello world
array(3) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
  ["three"]=>
  int(3)
}
"one" => 1
"two" => 2
"three" => 3
===DONE===
