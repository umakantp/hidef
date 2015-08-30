--TEST--
HIDEF: string/numeric index access
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
hidef.data_path=./tests/
--FILE--
<?php

function test_data($a)
{
	/* 3 elements and one FAIL to check error messages */
	for($i = 0; $i < 4; $i++) 
	{
		var_dump($a[$i]);
		var_dump($a["$i"]);
		var_dump(isset($a[$i]));
		var_dump(isset($a["$i"]));
	}
}

$a = hidef_fetch("assoc", true);
var_dump($a);
test_data($a);
if(is_array($a)) {
	test_data($a);
}

$a = hidef_fetch("numeric", true);
var_dump($a);
test_data($a);
if(is_array($a)) {
	test_data($a);
}

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
array(3) {
  [1]=>
  string(3) "xyz"
  [2]=>
  string(3) "abc"
  [0]=>
  string(5) "hello"
}
string(5) "hello"
string(5) "hello"
bool(true)
bool(true)
string(3) "xyz"
string(3) "xyz"
bool(true)
bool(true)
string(3) "abc"
string(3) "abc"
bool(true)
bool(true)

Notice: Undefined offset:%w3 in %s on line 8
NULL

Notice: Undefined index:%w3 in %s on line 9
NULL
bool(false)
bool(false)
string(5) "hello"
string(5) "hello"
bool(true)
bool(true)
string(3) "xyz"
string(3) "xyz"
bool(true)
bool(true)
string(3) "abc"
string(3) "abc"
bool(true)
bool(true)

Notice: Undefined offset:%w3 in %s on line 8
NULL

Notice: Undefined index:%w3 in %s on line 9
NULL
bool(false)
bool(false)
array(3) {
  [0]=>
  string(5) "hello"
  [1]=>
  string(3) "xyz"
  [2]=>
  string(3) "abc"
}
string(5) "hello"
string(5) "hello"
bool(true)
bool(true)
string(3) "xyz"
string(3) "xyz"
bool(true)
bool(true)
string(3) "abc"
string(3) "abc"
bool(true)
bool(true)

Notice: Undefined offset:%w3 in %s on line 8
NULL

Notice: Undefined index:%w3 in %s on line 9
NULL
bool(false)
bool(false)
string(5) "hello"
string(5) "hello"
bool(true)
bool(true)
string(3) "xyz"
string(3) "xyz"
bool(true)
bool(true)
string(3) "abc"
string(3) "abc"
bool(true)
bool(true)

Notice: Undefined offset:%w3 in %s on line 8
NULL

Notice: Undefined index:%w3 in %s on line 9
NULL
bool(false)
bool(false)
===DONE===
