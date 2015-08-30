/*
  +----------------------------------------------------------------------+
  | Hidef                                                                |
  +----------------------------------------------------------------------+
  | Copyright (c) 2007 The PHP Group                                     |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Gopal Vijayaraghavan <gopalv@php.net>                       |
  +----------------------------------------------------------------------+
*/

/* $Id: php_hidef.h 326605 2012-07-12 05:56:12Z gopalv $ */

#ifndef PHP_HIDEF_H
#define PHP_HIDEF_H

extern zend_module_entry hidef_module_entry;
#define phpext_hidef_ptr &hidef_module_entry

#define PHP_HIDEF_VERSION "0.1.12"

#ifdef PHP_WIN32
#define PHP_HIDEF_API __declspec(dllexport)
#else
#define PHP_HIDEF_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#if ZEND_MODULE_API_NO > 20060613
#define ZEND_ENGINE_2_3
#endif

#if ZEND_MODULE_API_NO >= 20100409
#define ZEND_ENGINE_2_4
#endif

#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION == 3 && PHP_RELEASE_VERSION < 2
#define ZEND_BROKEN_INI_SCANNER 1
#endif

/* b/c for the new macros */
#ifndef Z_REFCOUNT_P
#define Z_REFCOUNT_P(pz)              (pz)->refcount
#define Z_REFCOUNT_PP(ppz)            Z_REFCOUNT_P(*(ppz))
#endif

#ifndef Z_SET_REFCOUNT_P
#define Z_SET_REFCOUNT_P(pz, rc)      (pz)->refcount = rc
#define Z_SET_REFCOUNT_PP(ppz, rc)    Z_SET_REFCOUNT_P(*(ppz), rc)
#endif

#ifndef Z_ADDREF_P
#define Z_ADDREF_P(pz)                (pz)->refcount++
#define Z_ADDREF_PP(ppz)              Z_ADDREF_P(*(ppz))
#endif

#ifndef Z_DELREF_P
#define Z_DELREF_P(pz)                (pz)->refcount--
#define Z_DELREF_PP(ppz)              Z_DELREF_P(*(ppz))
#endif

#ifndef Z_ISREF_P
#define Z_ISREF_P(pz)                 (pz)->is_ref
#define Z_ISREF_PP(ppz)               Z_ISREF_P(*(ppz))
#endif

#ifndef Z_SET_ISREF_P
#define Z_SET_ISREF_P(pz)             (pz)->is_ref = 1
#define Z_SET_ISREF_PP(ppz)           Z_SET_ISREF_P(*(ppz))
#endif

#ifndef Z_UNSET_ISREF_P
#define Z_UNSET_ISREF_P(pz)           (pz)->is_ref = 0
#define Z_UNSET_ISREF_PP(ppz)         Z_UNSET_ISREF_P(*(ppz))
#endif

#ifndef Z_SET_ISREF_TO_P
#define Z_SET_ISREF_TO_P(pz, isref)   (pz)->is_ref = isref
#define Z_SET_ISREF_TO_PP(ppz, isref) Z_SET_ISREF_TO_P(*(ppz), isref)
#endif

PHP_MINIT_FUNCTION(hidef);
PHP_RINIT_FUNCTION(hidef);
PHP_MSHUTDOWN_FUNCTION(hidef);
PHP_RSHUTDOWN_FUNCTION(hidef);
PHP_MINFO_FUNCTION(hidef);


/* {{{ zend_hidef_globals */
ZEND_BEGIN_MODULE_GLOBALS(hidef)
	/* configuration parameters */
	char *ini_path;		/* search path for ini files */
	char *data_path;	/* search path for the data files */
	zval* data_hash;	/* hash storing the data */
#ifdef ZTS
	THREAD_T p_tid;		/* thread-id of parent thread */
#else
	pid_t p_pid;		/* process-id of parent process */
#endif
	char* per_request_ini; /* per request ini (load at RINIT) */
	zend_bool enable_cli;  /* enable hidef for cli apps */
	long memory_limit;	   /* start-up memory limit */
ZEND_END_MODULE_GLOBALS(hidef)
/* }}} */

/* {{{ extern hidef_globals */
ZEND_EXTERN_MODULE_GLOBALS(hidef)
/* }}} */

#ifdef ZTS
#define HIDEF_G(v) TSRMG(hidef_globals_id, zend_hidef_globals *, v)
#else
#define HIDEF_G(v) (hidef_globals.v)
#endif

#endif	/* PHP_HIDEF_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim>600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
