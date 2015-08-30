--TEST--
HIDEF: ensure consistency of copy-on-write copies - complex array
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
hidef.data_path=./tests/
--FILE--
<?php
$a = hidef_fetch("array2");

$b = (array)($a);
$c = (array)($a);

$b6 = &$b[6];

$b6 = array();

if(empty($c[6])) 
{
	print "Consistency failure of copy-on-write array\n";
}

var_dump($c);

echo '"0" => '.$c[0]."\n";
echo '"1" => '.$c[1]."\n";
echo '"2" => '.$c[2]."\n";
echo '"3" => '.$c[3]."\n";
echo '"4" => '.$c[4]."\n";
echo '"5" => '.$c[5]."\n";

$sub = $c[6];

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
