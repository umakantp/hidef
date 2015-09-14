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

/* $Id: frozenarray.c 326579 2012-07-11 10:47:33Z gopalv $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "zend_globals.h"
#include "zend_interfaces.h"
#include "zend_exceptions.h"
#include "php_streams.h"
#include "ext/standard/php_var.h"
#include "ext/standard/php_smart_str.h"
#include "ext/standard/php_incomplete_class.h"

#ifdef HAVE_SPL
#include "ext/spl/spl_iterators.h"
#endif

#include "php_hidef.h"
#include "frozenarray.h"

/* {{{ refcount macros */
#ifndef Z_ADDREF_P
#define Z_ADDREF_P(pz)                (pz)->refcount++
#define Z_ADDREF_PP(ppz)              Z_ADDREF_P(*(ppz))
#endif

#ifndef Z_DELREF_P
#define Z_DELREF_P(pz)                (pz)->refcount--
#define Z_DELREF_PP(ppz)              Z_DELREF_P(*(ppz))
#endif
/* }}} */

/* {{{ arginfo static macro */
#if PHP_MAJOR_VERSION > 5 || PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 3
#define HIDEF_ARGINFO_STATIC
#else
#define HIDEF_ARGINFO_STATIC static
#endif
/* }}} */

/* {{{ zend_parse_parameters_none() macro */
#ifndef zend_parse_parameters_none
#define zend_parse_parameters_none() zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "")
#endif
/* }}} */


static zend_class_entry* frozen_array_ce = NULL;

static zend_object_handlers frozen_array_object_handlers;

PHP_HIDEF_API zend_class_entry* frozen_array_get_ce()
{
	return frozen_array_ce;
}

#define FROZEN_ME(func, arg_info, flags) \
	PHP_ME(FrozenArray, func, arg_info, flags)

#define FROZEN_METHOD(func) \
	PHP_METHOD(FrozenArray, func)

static frozen_array_object* frozen_array_object_new(zend_class_entry *ce TSRMLS_DC);
static zend_object_value frozen_array_register_object(frozen_array_object* intern TSRMLS_DC);

static void frozen_array_object_clone(void *object, void **clone_ptr TSRMLS_DC);

static void frozen_array_object_dtor(void *object, zend_object_handle handle TSRMLS_DC);
static void frozen_array_object_free_storage(void *object TSRMLS_DC);

static frozen_array_object* frozen_array_object_new(zend_class_entry *ce TSRMLS_DC) 
{
	frozen_array_object* intern = NULL;

	intern = ecalloc(1, sizeof(frozen_array_object));

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION == 1 && PHP_RELEASE_VERSION > 2) || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION > 1) || (PHP_MAJOR_VERSION > 5)
	zend_object_std_init(&intern->zo, ce TSRMLS_CC);
#else
	ALLOC_HASHTABLE(intern->zo.properties);
	zend_hash_init(intern->zo.properties, 0, NULL, ZVAL_PTR_DTOR, 0);

	intern->zo.ce = ce;
	intern->zo.guards = NULL;
#endif

	intern->thawed = NULL;
	intern->pinned = NULL;

	return intern;
}

static zend_object_value frozen_array_register_object(frozen_array_object* intern TSRMLS_DC) 
{

	zend_object_value rv;

	rv.handle = zend_objects_store_put(intern, 
					frozen_array_object_dtor, 
					(zend_objects_free_object_storage_t)frozen_array_object_free_storage, 
					frozen_array_object_clone TSRMLS_CC);
	
	/* does not support (EG(ze1_compatibility_mode)) */

	rv.handlers = (zend_object_handlers *) &frozen_array_object_handlers;

	return rv;
}

static void frozen_array_object_clone(void *object, void **clone_ptr TSRMLS_DC)
{
	frozen_array_object *intern = (frozen_array_object*) object;
	frozen_array_object *clone = NULL;

	clone = frozen_array_object_new(intern->zo.ce TSRMLS_CC);

	/* move data across */
	clone->data = intern->data;
	clone->thawed = NULL; /* do not clone thawed objects (for now) */

	*clone_ptr = clone;
}


static void frozen_array_object_dtor(void *object, zend_object_handle handle TSRMLS_DC)
{
	frozen_array_object *intern = (frozen_array_object*) object;
}

static void frozen_array_object_free_storage(void *object TSRMLS_DC)
{
	frozen_array_object *intern = (frozen_array_object*) object;

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION == 1 && PHP_RELEASE_VERSION > 2) || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION > 1) || (PHP_MAJOR_VERSION > 5)
	zend_object_std_dtor(&intern->zo TSRMLS_CC);
#else
	if (intern->zo.guards)
	{
		zend_hash_destroy(intern->zo.guards);
		FREE_HASHTABLE(intern->zo.guards);
	}

	if (intern->zo.properties)
	{
		zend_hash_destroy(intern->zo.properties);
		FREE_HASHTABLE(intern->zo.properties);
	}
#endif

	/* other cleanups (here or in dtor, check docs) */
	if(intern->thawed)
	{
		Z_DELREF_P(intern->thawed);
		if(Z_REFCOUNT_P(intern->thawed) == 0) 
		{
			zval_dtor(intern->thawed);
			FREE_ZVAL(intern->thawed);
		}
		intern->thawed = NULL;
	}

	if(intern->pinned)
	{
		/* release the array */
		Z_DELREF_P(intern->pinned);
		intern->pinned = NULL;
	}
	
	if(intern->data) 
	{
		efree(intern->data);
	}
	efree(intern);
}

PHP_HIDEF_API zend_object_value frozen_array_new(zend_class_entry *ce TSRMLS_DC)
{
	frozen_array_object *intern = frozen_array_object_new(ce TSRMLS_CC);
	return frozen_array_register_object(intern TSRMLS_CC);
}

zval* frozen_array_wrap_zval(zval* src TSRMLS_DC)
{
	zval* dst;
	frozen_array_object *intern;

	MAKE_STD_ZVAL(dst);

	if(Z_TYPE_P(src) != IS_ARRAY)
	{
		*dst = *src;
		INIT_PZVAL(dst);
		if(Z_TYPE_P(src) == IS_STRING)
		{
			Z_STRVAL_P(dst) = emalloc(sizeof(char)*Z_STRLEN_P(src)+1);
			memcpy(Z_STRVAL_P(dst), Z_STRVAL_P(src), Z_STRLEN_P(src)+1);
		}
		return dst;
	}

	intern = frozen_array_object_new(frozen_array_ce TSRMLS_CC);

	MAKE_STD_ZVAL(intern->data);
	*intern->data = *src;

	dst->type = IS_OBJECT;
	dst->value.obj = frozen_array_register_object(intern TSRMLS_CC);

	return dst;
}

zval* frozen_array_pin_zval(zval* src TSRMLS_DC)
{
	zval* dst;
	frozen_array_object *intern;

	dst = frozen_array_wrap_zval(src TSRMLS_CC);

	if(dst && Z_TYPE_P(dst) == IS_OBJECT)
	{
		intern = (frozen_array_object*)zend_object_store_get_object(dst TSRMLS_CC);
		if(intern->data)
		{
			Z_ADDREF_P(src);
			intern->pinned = src;
		}
	}

	return dst;
}

zval* frozen_array_thaw_zval(zval* object, size_t* allocated TSRMLS_DC)
{
	frozen_array_object *intern = (frozen_array_object*)zend_object_store_get_object(object TSRMLS_CC);

	if(intern->data && !intern->thawed) 
	{
		intern->thawed = frozen_array_copy_zval_ptr(NULL, intern->data, 0, allocated TSRMLS_CC);

		if(intern->thawed)
		{
			if(Z_REFCOUNT_P(intern->thawed) == 0) 
			{
				Z_SET_REFCOUNT_P(intern->thawed, 1);
			}
		}
	}
	else if(intern->data == NULL)
	{
		/* in case someone calls unserialize() with some old data */
		MAKE_STD_ZVAL(intern->thawed);
		array_init(intern->thawed);
	}

	if(intern->thawed)
	{
		return intern->thawed;
	}

	return NULL;
}

FROZEN_METHOD(offsetExists)
{
	zval *zindex;
	zval **hentry = NULL;

	zval *object = getThis();
	frozen_array_object *intern = (frozen_array_object*)zend_object_store_get_object(object TSRMLS_CC);

	/* check data */
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zindex) == FAILURE)
	{
		return;
	}

	if(Z_TYPE_P(zindex) != IS_STRING && Z_TYPE_P(zindex) != IS_LONG)
	{
		convert_to_string(zindex);
	}

	if(intern->data && (Z_TYPE_P(intern->data) == IS_ARRAY))
	{
		if(Z_TYPE_P(zindex) == IS_STRING)
		{
			if(zend_symtable_find(Z_ARRVAL_P(intern->data), Z_STRVAL_P(zindex), Z_STRLEN_P(zindex)+1, (void**)(&hentry)) != FAILURE)
			{
				if(hentry != NULL && Z_TYPE_PP(hentry) != IS_NULL)
				{
					RETURN_BOOL(1);
				}
			}
		}
		else if(Z_TYPE_P(zindex) == IS_LONG)
		{
			if(zend_hash_index_find(Z_ARRVAL_P(intern->data), Z_LVAL_P(zindex), (void**)(&hentry)) != FAILURE)
			{
				if(hentry != NULL && Z_TYPE_PP(hentry) != IS_NULL)
				{
					RETURN_BOOL(1);
				}
			}
		}
	}

	RETURN_BOOL(0);
}

FROZEN_METHOD(offsetGet)
{
	zval *zindex;
	zval *retval;
	zval **hentry = NULL;
	zval *object = getThis();
	frozen_array_object *intern = (frozen_array_object*)zend_object_store_get_object(object TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zindex) == FAILURE)
	{
		RETURN_NULL();
		return;
	}

	if(Z_TYPE_P(zindex) != IS_STRING && Z_TYPE_P(zindex) != IS_LONG)
	{
		convert_to_string(zindex);
	}

	if(intern->data && (Z_TYPE_P(intern->data) == IS_ARRAY))
	{
		if(Z_TYPE_P(zindex) == IS_STRING)
		{
			if(zend_symtable_find(Z_ARRVAL_P(intern->data), Z_STRVAL_P(zindex), Z_STRLEN_P(zindex)+1, (void**)(&hentry)) != FAILURE)
			{
				retval = frozen_array_wrap_zval(hentry[0] TSRMLS_CC);
				RETURN_ZVAL(retval, 0, 1);
			}
			else
			{
				zend_error(E_NOTICE, "Undefined index: %s", Z_STRVAL_P(zindex));
			}
		}
		else if(Z_TYPE_P(zindex) == IS_LONG)
		{
			if(zend_hash_index_find(Z_ARRVAL_P(intern->data), Z_LVAL_P(zindex), (void**)(&hentry)) != FAILURE)
			{
				retval = frozen_array_wrap_zval(hentry[0] TSRMLS_CC);
				RETURN_ZVAL(retval, 0, 1);
			}
			else
			{
				zend_error(E_NOTICE, "Undefined offset: %ld", Z_LVAL_P(zindex));
			}
		}
	}

	RETURN_NULL();
}

FROZEN_METHOD(offsetSet)
{
	char *error = "Cannot set elements of a FrozenArray";
	zend_throw_exception(zend_exception_get_default(TSRMLS_C), error, 0 TSRMLS_CC);
	return;
}
FROZEN_METHOD(offsetUnset)
{
	char *error = "Cannot unset elements of a FrozenArray";
	zend_throw_exception(zend_exception_get_default(TSRMLS_C), error, 0 TSRMLS_CC);
	return;
}

FROZEN_METHOD(__toString)
{
	smart_str str = {0,};
	zval *object = getThis();
	frozen_array_object *intern = (frozen_array_object*)zend_object_store_get_object(object TSRMLS_CC);
	HashTable *data = Z_ARRVAL_P(intern->data);

	if (zend_parse_parameters_none() == FAILURE)
	{
		return;
	}

	smart_str_appends(&str, "FrozenArray[");
	smart_str_append_unsigned(&str, data->nNumOfElements);
	smart_str_appends(&str, "]");
	smart_str_0(&str);

	RETVAL_STRING(str.c, 1);

	smart_str_free(&str);
	return;
}

FROZEN_METHOD(count)
{
	zval *object = getThis();
	frozen_array_object *intern = (frozen_array_object*)zend_object_store_get_object(object TSRMLS_CC);
	HashTable *data = Z_ARRVAL_P(intern->data);
	int len = -1;

	if (zend_parse_parameters_none() == FAILURE)
	{
		return;
	}

	if(data != NULL)
	{
		len = zend_hash_num_elements(data);
	}

	RETVAL_LONG((long)len);
}

FROZEN_METHOD(thaw)
{
	zval *object = getThis();
	zval *retval = NULL;
	size_t allocated = 0;
	zval **z_allocated = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|Z", &z_allocated) == FAILURE)
	{
		RETURN_NULL();
		return;
	}

	retval = frozen_array_thaw_zval(object, &allocated TSRMLS_CC); 

	if(z_allocated)
	{
		convert_to_long_ex(z_allocated);
		Z_LVAL_PP(z_allocated) = (long)(allocated);
	}

	RETURN_ZVAL(retval, 1, 0);
}

FROZEN_METHOD(__construct)
{
	char *error = "Cannot construct object of type FrozenArray";
	zend_throw_exception(zend_exception_get_default(TSRMLS_C), error, 0 TSRMLS_CC);
	return;
}

HIDEF_ARGINFO_STATIC
ZEND_BEGIN_ARG_INFO_EX(arginfo_void, 0, 0, 0)
ZEND_END_ARG_INFO()

HIDEF_ARGINFO_STATIC
ZEND_BEGIN_ARG_INFO_EX(arginfo_offsetGet, 0, 0, 1)
	ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

HIDEF_ARGINFO_STATIC
ZEND_BEGIN_ARG_INFO_EX(arginfo_offsetSet, 0, 0, 2)
	ZEND_ARG_INFO(0, index)
	ZEND_ARG_INFO(0, newval)
ZEND_END_ARG_INFO()
HIDEF_ARGINFO_STATIC
ZEND_BEGIN_ARG_INFO_EX(arginfo_thaw, 0, 0, 1)
	ZEND_ARG_INFO(1, allocated)
ZEND_END_ARG_INFO()

static zend_function_entry frozen_array_functions[] = {
	PHP_ME(FrozenArray, __construct,  arginfo_void,      ZEND_ACC_PRIVATE|ZEND_ACC_FINAL)
	PHP_ME(FrozenArray, count,        arginfo_void,      ZEND_ACC_PUBLIC)
	PHP_ME(FrozenArray, offsetExists, arginfo_offsetGet, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(FrozenArray, offsetGet,    arginfo_offsetGet, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(FrozenArray, offsetSet,    arginfo_offsetSet, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(FrozenArray, offsetUnset,  arginfo_offsetGet, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(FrozenArray, __toString,   arginfo_void,      ZEND_ACC_PUBLIC)
	PHP_ME(FrozenArray, thaw,         arginfo_thaw,      ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL}
};

/* {{{ frozen array iterator */
typedef struct _frozen_array_it 
{
	zend_user_iterator	intern;
	frozen_array_object	*object;
	HashPosition pos;
	zval *current;
} frozen_array_it;

static void frozen_array_it_dtor(zend_object_iterator *iter TSRMLS_DC)
{
	frozen_array_it *iterator = (frozen_array_it*)(iter);
	Z_DELREF_PP((zval**)(&iterator->intern.it.data));
	if(iterator->current) zval_ptr_dtor(&iterator->current);
	efree(iterator);
}


static int frozen_array_it_valid(zend_object_iterator *iter TSRMLS_DC)
{
	frozen_array_it *iterator = (frozen_array_it*)(iter);
	frozen_array_object *obj = iterator->object;
	return zend_hash_has_more_elements_ex(Z_ARRVAL_P(obj->data), &(iterator->pos));
}

static void frozen_array_it_get_current_data(zend_object_iterator *iter, zval ***data TSRMLS_DC)
{
	zval **hentry = NULL;

	frozen_array_it *iterator = (frozen_array_it*)(iter);
	frozen_array_object *obj = iterator->object;

	if(zend_hash_get_current_data_ex(Z_ARRVAL_P(obj->data), (void**)(&hentry), &(iterator->pos)) != FAILURE)
	{
		if(iterator->current) zval_ptr_dtor(&iterator->current);
		iterator->current = frozen_array_wrap_zval(hentry[0] TSRMLS_CC);
		*data = &(iterator->current);
	}
}

static int frozen_array_it_get_current_key(zend_object_iterator *iter, char **str_key, uint *str_key_len, ulong *int_key TSRMLS_DC)
{
	frozen_array_it *iterator = (frozen_array_it*)(iter);
	frozen_array_object *obj = iterator->object;

	return zend_hash_get_current_key_ex(Z_ARRVAL_P(obj->data), str_key, str_key_len, int_key, 1, &(iterator->pos));
}

static void frozen_array_it_move_forward(zend_object_iterator *iter TSRMLS_DC)
{
	frozen_array_it *iterator = (frozen_array_it*)(iter);
	frozen_array_object *obj = iterator->object;
	zend_hash_move_forward_ex(Z_ARRVAL_P(obj->data), &(iterator->pos));
}

static void frozen_array_it_rewind(zend_object_iterator *iter TSRMLS_DC)
{
	frozen_array_it *iterator = (frozen_array_it*)(iter);
	frozen_array_object *obj = iterator->object;

	zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(obj->data), &(iterator->pos));
}

zend_object_iterator_funcs frozen_array_it_funcs = {
	frozen_array_it_dtor,
	frozen_array_it_valid,
	frozen_array_it_get_current_data,
	frozen_array_it_get_current_key,
	frozen_array_it_move_forward,
	frozen_array_it_rewind
};

zend_object_iterator* frozen_array_get_iterator(zend_class_entry *ce, zval *object, int by_ref TSRMLS_DC) 
{
	frozen_array_it  *iterator;
	frozen_array_object *obj = (frozen_array_object*)zend_object_store_get_object(object TSRMLS_CC);
	
	if(by_ref)
	{
		zend_error(E_ERROR, "An iterator cannot be used with foreach by reference");
	}
	
	iterator = emalloc(sizeof(frozen_array_it));

	Z_ADDREF_P(object);

	iterator->intern.it.data = (void*)object;
	iterator->intern.it.funcs = &frozen_array_it_funcs;
	iterator->intern.ce = ce;
	iterator->intern.value = NULL;
	iterator->object = obj;
	iterator->current = NULL;
	
	return  (zend_object_iterator*)iterator;
}

/* }}} */

/* {{{ frozen_array_property_read */
static zval * frozen_array_property_read(zval *object, zval *member, int type TSRMLS_DC)
{
	frozen_array_object *intern = (frozen_array_object*)zend_object_store_get_object(object TSRMLS_CC);
	zval **hentry = NULL;
	zval *retval = NULL;

	if(Z_TYPE_P(member) != IS_STRING)
	{
		// TODO: error?
		convert_to_string(member);
	}

	if(intern->data && (Z_TYPE_P(intern->data) == IS_ARRAY))
	{
		if(Z_TYPE_P(member) == IS_STRING)
		{
			if(zend_symtable_find(Z_ARRVAL_P(intern->data), Z_STRVAL_P(member), Z_STRLEN_P(member)+1, (void**)(&hentry)) != FAILURE)
			{
				if(hentry != NULL && Z_TYPE_PP(hentry) != IS_NULL)
				{
					retval = frozen_array_wrap_zval(hentry[0] TSRMLS_CC);
					return retval;
				}
			}
		}
	}

	MAKE_STD_ZVAL(retval);
	ZVAL_NULL(retval);

	return retval;
}
/* }}} */

/* {{{ frozen_array_property_write_deny */
static void frozen_array_property_write_deny(zval *object, zval *member, zval *value TSRMLS_DC) 
{
	zend_class_entry *ce = Z_OBJCE_P(object);
	zval *tmp_member = NULL;

 	if (Z_TYPE_P(member) != IS_STRING) {
 		ALLOC_ZVAL(tmp_member);
		*tmp_member = *member;
		INIT_PZVAL(tmp_member);
		zval_copy_ctor(tmp_member);
		convert_to_string(tmp_member);
		member = tmp_member;
	}
	zend_throw_exception_ex(NULL, 0 TSRMLS_CC, "Assigning to '%s::%s' is not allowed", ce->name, Z_STRVAL_P(member));
	if (tmp_member) {
		zval_ptr_dtor(&tmp_member);
	}
	return;
}
/* }}} */

/* {{{ frozen_array_property_ref_deny */
static zval** frozen_array_property_ref_deny(zval *object, zval *member TSRMLS_DC) 
{
	zend_class_entry *ce = Z_OBJCE_P(object);
	zval *tmp_member = NULL;

 	if (Z_TYPE_P(member) != IS_STRING) {
 		ALLOC_ZVAL(tmp_member);
		*tmp_member = *member;
		INIT_PZVAL(tmp_member);
		zval_copy_ctor(tmp_member);
		convert_to_string(tmp_member);
		member = tmp_member;
	}
	zend_throw_exception_ex(NULL, 0 TSRMLS_CC, "Referencing member variable '%s::%s' is not allowed", ce->name, Z_STRVAL_P(member));
	if (tmp_member) {
		zval_ptr_dtor(&tmp_member);
	}
	return NULL;
}
/* }}} */

/* {{{ frozen_array_cast_object */
int frozen_array_cast_object(zval *readobj, zval *writeobj, int type TSRMLS_DC) /* {{{ */
{
	frozen_array_object *intern = (frozen_array_object*)zend_object_store_get_object(readobj TSRMLS_CC);
	HashTable *data = NULL;

	switch(type)
	{
		case IS_BOOL:
		{
			INIT_PZVAL(writeobj);
			ZVAL_BOOL(writeobj, 0);

			if(intern->data != NULL && 
				(data /* assign */ = Z_ARRVAL_P(intern->data)) != NULL)
			{
				ZVAL_BOOL(writeobj, data->nNumOfElements != 0);
			}

			return SUCCESS;
		}
		break;
		case IS_STRING:
		case IS_LONG:
		case IS_DOUBLE:
		{
			return zend_std_cast_object_tostring(readobj, writeobj, type TSRMLS_CC);
		}
		break;
		/* the array case is handled by get_properties() below */
		default:
		{
			INIT_PZVAL(writeobj);
			Z_TYPE_P(writeobj) = IS_NULL;
		}
		break;
	}

	return FAILURE;
}
/* }}} */

/* {{{ frozen_array_get_properties */
HashTable* frozen_array_get_properties(zval *object TSRMLS_DC) 
{
	zval* thawed = frozen_array_thaw_zval(object, NULL TSRMLS_CC);

	if(thawed) 
	{
		if(Z_TYPE_P(thawed) == IS_CONSTANT_ARRAY || 
			Z_TYPE_P(thawed) == IS_ARRAY)
		{
			return Z_ARRVAL_P(thawed);
		}
	}

	return NULL;
}
/* }}} */

#ifndef ZEND_ENGINE_2_3
/* {{{ serialize/unserialize deny */
static int frozen_array_serialize_deny(zval *object, unsigned char **buffer, zend_uint *buf_len, zend_serialize_data *data TSRMLS_DC) /* {{{ */
{
	zend_class_entry *ce = Z_OBJCE_P(object);
	zend_throw_exception_ex(NULL, 0 TSRMLS_CC, "Serialization of '%s' is not allowed", ce->name);
	return FAILURE;
}
/* }}} */

static int frozen_array_unserialize_deny(zval **object, zend_class_entry *ce, const unsigned char *buf, zend_uint buf_len, zend_unserialize_data *data TSRMLS_DC) /* {{{ */
{
	zend_throw_exception_ex(NULL, 0 TSRMLS_CC, "Unserialization of '%s' is not allowed", ce->name);
	return FAILURE;
}
/* }}} */

#define zend_class_unserialize_deny frozen_array_unserialize_deny
#define zend_class_serialize_deny frozen_array_serialize_deny
/* }}} */
#endif /* ZEND_ENGINE_2_3 */

void frozen_array_init(TSRMLS_D)
{
	zend_class_entry ce;

	/* change the ones we need to */

	INIT_CLASS_ENTRY(ce, "FrozenArray", frozen_array_functions);
	ce.create_object = frozen_array_new;
	ce.get_iterator = frozen_array_get_iterator;

	ce.serialize = zend_class_serialize_deny;
	ce.unserialize = zend_class_unserialize_deny;

	frozen_array_ce = zend_register_internal_class(&ce TSRMLS_CC);
	
	memcpy(&frozen_array_object_handlers, 
			zend_get_std_object_handlers(), 
			sizeof(zend_object_handlers));
	
	frozen_array_object_handlers.get_properties = frozen_array_get_properties;
	frozen_array_object_handlers.cast_object = frozen_array_cast_object;
	frozen_array_object_handlers.read_property = frozen_array_property_read;
	frozen_array_object_handlers.write_property = frozen_array_property_write_deny;
	frozen_array_object_handlers.get_property_ptr_ptr = frozen_array_property_ref_deny;

	zend_class_implements(frozen_array_ce TSRMLS_CC, 1, zend_ce_arrayaccess);
#ifdef HAVE_SPL
	zend_class_implements(frozen_array_ce TSRMLS_CC, 1, spl_ce_Countable);
#endif
}

void frozen_array_shutdown(TSRMLS_D)
{
	frozen_array_ce = NULL;
}

#ifndef IS_CONSTANT_TYPE_MASK
#define IS_CONSTANT_TYPE_MASK (~IS_CONSTANT_INDEX)
#endif

static inline void* frozen_array_alloc(size_t size, int persistent, size_t *allocated)
{
	if(allocated) (*allocated += size);

	return pemalloc(size, persistent);
}


static HashTable* frozen_array_copy_hashtable(HashTable* dst, HashTable* src, int persistent, size_t *allocated TSRMLS_DC)
{
	Bucket* curr = NULL;
	Bucket* prev = NULL;
	Bucket* newp = NULL;
	int first = 1;

    if (!dst)
	{
        dst = (HashTable*) frozen_array_alloc(sizeof(HashTable), persistent, allocated);
    }

	memcpy(dst, src, sizeof(HashTable));

	dst->arBuckets = frozen_array_alloc(sizeof(Bucket*) * dst->nTableSize, persistent, allocated);

	if(!persistent)
	{
		dst->persistent = persistent;
		dst->pDestructor = ZVAL_PTR_DTOR;
	}
	else
	{
		dst->persistent = persistent;
		dst->pDestructor = NULL;
	}


    memset(dst->arBuckets, 0, dst->nTableSize * sizeof(Bucket*));
    dst->pInternalPointer = NULL;
    dst->pListHead = NULL;

    for (curr = src->pListHead; curr != NULL; curr = curr->pListNext)
	{
        int n = curr->h % dst->nTableSize;

#ifdef ZEND_ENGINE_2_4
		newp = frozen_array_alloc(sizeof(Bucket), persistent, allocated);
		memcpy(newp, curr, sizeof(Bucket));
		
		newp->arKey = pestrndup(curr->arKey, curr->nKeyLength, persistent);
#else
		newp = frozen_array_alloc((sizeof(Bucket) + curr->nKeyLength - 1), persistent, allocated);
		memcpy(newp, curr, sizeof(Bucket) + curr->nKeyLength - 1);
#endif
		
        /* insert 'newp' into the linked list at its hashed index */
        if (dst->arBuckets[n])
		{
            newp->pNext = dst->arBuckets[n];
            newp->pLast = NULL;
            newp->pNext->pLast = newp;
        }
        else
		{
            newp->pNext = newp->pLast = NULL;
        }

        dst->arBuckets[n] = newp;

        /* copy the bucket data */
		newp->pDataPtr = frozen_array_copy_zval_ptr(NULL, curr->pDataPtr, persistent, allocated TSRMLS_CC);
		newp->pData = &newp->pDataPtr;

        /* insert 'newp' into the table-thread linked list */
        newp->pListLast = prev;
        newp->pListNext = NULL;

        if (prev) {
            prev->pListNext = newp;
        }

        if (first) {
            dst->pListHead = newp;
            first = 0;
        }

        prev = newp;
    }

    dst->pListTail = newp;

    return dst;
}

zval* frozen_array_copy_zval_ptr(zval* dst, zval* src, int persistent, size_t* allocated TSRMLS_DC)
{
	if(!dst)
	{
		if(persistent)
		{
			dst = frozen_array_alloc(sizeof(zval), persistent, allocated);
		}
		else
		{
			/* thanks to the zend gc, we need to allocate it in other ways */
			MAKE_STD_ZVAL(dst);
		}
	}

	memcpy(dst, src, sizeof(zval));


    switch (src->type & IS_CONSTANT_TYPE_MASK)
	{
		case IS_RESOURCE:
		case IS_BOOL:
		case IS_LONG:
		case IS_DOUBLE:
		case IS_NULL:
			break;

		case IS_CONSTANT:
		case IS_STRING:
		{
			if (Z_STRVAL_P(src))
			{
				Z_STRVAL_P(dst) = frozen_array_alloc(sizeof(char)*Z_STRLEN_P(src)+1, persistent, allocated);
				memcpy(Z_STRVAL_P(dst), Z_STRVAL_P(src), Z_STRLEN_P(src)+1);
			}
        }
        break;

		case IS_ARRAY:
		case IS_CONSTANT_ARRAY:
		{
			if(!Z_ISREF_P(src))
			{
				Z_ARRVAL_P(dst) = frozen_array_copy_hashtable(NULL, Z_ARRVAL_P(src), persistent, allocated TSRMLS_CC);
			}
			else
			{
				/* Swap out a reference with an error message */ 
				dst->type = IS_STRING;
				Z_SET_REFCOUNT_P(dst, 1);
				Z_UNSET_ISREF_P(dst);
				Z_STRVAL_P(dst) = pestrdup("**RECURSION**", persistent);
				Z_STRLEN_P(dst) = sizeof("**RECURSION**")-1;
			}
		}
		break;

		case IS_OBJECT:
		{
			/* ERROR ! */
			dst->type = IS_NULL;
			Z_SET_REFCOUNT_P(dst, 1);
			Z_UNSET_ISREF_P(dst);
			if(persistent)
			{
				zend_class_entry *zce = Z_OBJCE_P(src);
				char *class_name = NULL;
				zend_uint class_name_len;

				if(zce && zce == PHP_IC_ENTRY)
				{
					class_name = php_lookup_class_name(src, &class_name_len);
				}
				else if(zce && Z_OBJ_HANDLER_P(src, get_class_name))
				{
					Z_OBJ_HANDLER_P(src, get_class_name)(src, &class_name, &class_name_len, 0 TSRMLS_CC);
				}

				zend_error(E_ERROR, "Unknown object of type '%s' found in serialized hash", class_name ? class_name : "Unknown");
				if(class_name) efree(class_name);
				zend_bailout();
			}
		}
		break;

		default:
			assert(0);
	}

    return dst;
}

static void frozen_array_free_hashtable(HashTable **_ht, int persistent TSRMLS_DC)
{
	HashTable *ht = *_ht;
	Bucket* curr = ht->pListHead;
	Bucket* tmp = NULL;
	
    while(curr)
	{
		tmp = curr;
		curr = curr->pListNext;
		frozen_array_free_zval_ptr((zval**)(&tmp->pDataPtr), persistent TSRMLS_CC);
		pefree(tmp, persistent);
	}

	pefree(ht->arBuckets, persistent);
	pefree(ht, persistent);
	*_ht = NULL;
}

void frozen_array_free_zval_ptr(zval** val, int persistent TSRMLS_DC)
{
	zval *value = *val;

    switch (value->type & IS_CONSTANT_TYPE_MASK)
	{
		case IS_RESOURCE:
		case IS_BOOL:
		case IS_LONG:
		case IS_DOUBLE:
		case IS_NULL:
			break;

		case IS_CONSTANT:
		case IS_STRING:
		{
			if (Z_STRVAL_P(value))
			{
				pefree(Z_STRVAL_P(value), persistent);
			}
        }
        break;

		case IS_ARRAY:
		case IS_CONSTANT_ARRAY:
			frozen_array_free_hashtable(&Z_ARRVAL_P(value), persistent TSRMLS_CC);
			break;

		case IS_OBJECT:
			/* ERROR ! */
			break;

		default:
			assert(0);
	}

	pefree(value, persistent);
	*val = NULL;
}

zval* frozen_array_unserialize(const char* filename TSRMLS_DC)
{
	zval* data;
	zval* retval;
	long len = 0;
	struct stat sb;
	char *contents, *tmp, *conl;
	FILE *fp;
	php_unserialize_data_t var_hash;
	HashTable class_table = {0,};
	HashTable *orig_class_table = NULL;

	if(VCWD_STAT(filename, &sb) == -1) 
	{
		return NULL;
	}

	fp = fopen(filename, "rb");
	
	if((!fp) || (sb.st_size == 0))
	{
		/* can't read or zero size */
		return NULL;
	}

	len = sizeof(char)*sb.st_size;

	tmp = contents = malloc(len);

	len = fread(contents, 1, len, fp);

	MAKE_STD_ZVAL(data);

	PHP_VAR_UNSERIALIZE_INIT(var_hash);

	zend_hash_init_ex(&class_table, 10, NULL, ZEND_CLASS_DTOR, 1, 0);

	/* we have to do this because unserialize() calls zend_lookup_class
	 * which fails to check for EG(class_table) & segfaults directly. 
	 */
	orig_class_table = EG(class_table);
	EG(class_table) = &class_table;
	zend_objects_store_init(&EG(objects_store), 1024);
	conl = contents+len;

	/* I wish I could use json */
#ifdef ZEND_ENGINE_2_6
	if(!php_var_unserialize(&data, (const unsigned char**)&tmp, (const unsigned char*)&conl, &var_hash TSRMLS_CC))
	{
#else
    if(!php_var_unserialize(&data, (const unsigned char**)&tmp, (const unsigned char**)contents+len, &var_hash TSRMLS_CC))
    {
#endif
		zval_ptr_dtor(&data);
		free(contents);
		fclose(fp);
		return NULL;
	}

	PHP_VAR_UNSERIALIZE_DESTROY(var_hash);

	retval = frozen_array_copy_zval_ptr(NULL, data, 1, NULL TSRMLS_CC);

	zval_ptr_dtor(&data);

	zend_objects_store_free_object_storage(&EG(objects_store) TSRMLS_CC);
	zend_objects_store_destroy(&EG(objects_store));

	EG(class_table) = orig_class_table;
	zend_hash_destroy(&class_table);

	free(contents);
	fclose(fp);

	return retval;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim>600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
