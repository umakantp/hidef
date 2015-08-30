--TEST--
HIDEF: hidef isset() tests
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
hidef.data_path=./tests/
--FILE--
<?php
$a = hidef_fetch("empties");

for ($i = 0; $i < count($a); $i++)
{
	echo "isset(\$a[$i]) = ".(isset($a[$i]) ? "true" : "false")."\n";
	/* note that $a[6] = FrozenArray, which does not match empty() */
	echo "empty(\$a[$i]) = ".(empty($a[$i]) ? "true" : "false")."\n";
}

echo "isset(\$a[$i]) = ".(isset($a[$i]) ? "true" : "false")."\n";
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
isset($a[0]) = false
empty($a[0]) = true
isset($a[1]) = true
empty($a[1]) = true
isset($a[2]) = true
empty($a[2]) = true
isset($a[3]) = true
empty($a[3]) = true
isset($a[4]) = true
empty($a[4]) = true
isset($a[5]) = true
empty($a[5]) = true
isset($a[6]) = true
empty($a[6]) = true
isset($a[7]) = false
===DONE===
