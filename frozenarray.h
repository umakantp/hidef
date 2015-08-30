/*
  +----------------------------------------------------------------------+
  | Hidef                                                                |
  +----------------------------------------------------------------------+
  | Copyright (c) 2008 The PHP Group                                     |
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

/* $Id: frozenarray.h 326185 2012-06-15 14:24:24Z felipe $ */

#ifndef PHP_FROZENARRAY_H
#define PHP_FROZENARRAY_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_hidef.h"
#include "zend.h"

typedef struct {
	zend_object zo;
	zval *data;
	zval *thawed;
	zval *pinned;
} frozen_array_object;

void frozen_array_init(TSRMLS_D);
void frozen_array_shutdown(TSRMLS_D);

zval* frozen_array_unserialize(const char* filename TSRMLS_DC);
zval* frozen_array_wrap_zval(zval* src TSRMLS_DC);
zval* frozen_array_pin_zval(zval* src TSRMLS_DC);

zval* frozen_array_copy_zval_ptr(zval *dst, zval *src, int persistent, size_t *allocated TSRMLS_DC);
void frozen_array_free_zval_ptr(zval** val, int persistent TSRMLS_DC);

#define FROZEN_METHOD(func) PHP_METHOD(FrozenArray, func)
#define PHP_HIDEF_OBJECT_NAME "FrozenArray"

#endif /* PHP_FROZENARRAY_H */
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim>600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
