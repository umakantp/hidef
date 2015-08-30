dnl $Id: config.m4 318410 2011-10-25 16:59:04Z gopalv $
dnl config.m4 for extension hidef

PHP_ARG_ENABLE(hidef, whether to enable hidef support,
[  --enable-hidef           Enable hidef support])

AC_CHECK_FUNCS(malloc_trim)

if test "$PHP_HIDEF" != "no"; then

  hidef_sources="	hidef.c\
					frozenarray.c"

  PHP_NEW_EXTENSION(hidef, $hidef_sources, $ext_shared)
  PHP_ADD_EXTENSION_DEP(hidef, spl, true)
fi
