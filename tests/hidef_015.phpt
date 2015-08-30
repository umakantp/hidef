--TEST--
HIDEF: test hidef_wrap() 
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
hidef.data_path=.
--FILE--
<?php
$a = array("Refcounting is hard, but it's the only way I'll survive");
$fa = hidef_wrap($a);
print $fa[0]."\n";
$a[0] = "Can I change my mind?";
print $fa[0]."\n";
unset($a);
print $fa[0]."\n";

print "REVERSE POLARITY\n";

$a = array("Refcounting is hard, but it's the only way I'll survive");
$fa = hidef_wrap($a);
print $fa[0]."\n";
$a[0] = "Can I change my mind?";
print $fa[0]."\n";
unset($fa);
print $a[0]."\n";
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Refcounting is hard, but it's the only way I'll survive
Refcounting is hard, but it's the only way I'll survive
Refcounting is hard, but it's the only way I'll survive
REVERSE POLARITY
Refcounting is hard, but it's the only way I'll survive
Refcounting is hard, but it's the only way I'll survive
Can I change my mind?
===DONE===
