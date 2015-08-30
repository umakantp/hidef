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

/* $Id: hidef.c 326605 2012-07-12 05:56:12Z gopalv $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "php_scandir.h"
#include "zend_globals.h"
#include "zend_ini_scanner.h"
#include "zend_hash.h"
#include "ext/standard/info.h"
#include "SAPI.h"
#include "malloc.h"

#include "php_hidef.h"
#include "frozenarray.h"

typedef int (*hidef_walk_dir_cb)(const char* filename, void* ctxt TSRMLS_DC);

/* {{{ stupid stringifcation */
#if DEFAULT_SLASH == '/'
	#define DEFAULT_SLASH_STRING "/"
#elif DEFAULT_SLASH == '\\'
	#define DEFAULT_SLASH_STRING "\\"
#else
	#error "Unknown value for DEFAULT_SLASH"
#endif
/* }}} */


/* {{{ hidef globals 
 *
 * true globals, no need for thread safety here 
 */
static HashTable *hidef_constants_table = NULL;
static HashTable *hidef_data_hash = NULL;
static const char default_ini_path[] = PHP_CONFIG_FILE_SCAN_DIR DEFAULT_SLASH_STRING "hidef";
/* }}} */

/* {{{ PHP_FUNCTION declarations */
PHP_FUNCTION(hidef_fetch);
PHP_FUNCTION(hidef_wrap);
/* }}} */

/* {{{ ZEND_DECLARE_MODULE_GLOBALS(hidef) */
ZEND_DECLARE_MODULE_GLOBALS(hidef)
/* }}} */

/* {{{ php_hidef_init_globals */
static void php_hidef_init_globals(zend_hidef_globals* hidef_globals TSRMLS_DC)
{
	hidef_globals->ini_path = NULL;
	hidef_globals->data_path = NULL;
	hidef_globals->per_request_ini = NULL;
	hidef_globals->enable_cli = 1;
	hidef_globals->memory_limit = 256 * 1024 * 1024;
}
/* }}} */

/* {{{ php_hidef_shutdown_globals */
static void php_hidef_shutdown_globals(zend_hidef_globals* hidef_globals TSRMLS_DC)
{
	/* nothing ? */
}
/* }}} */

static PHP_INI_MH(OnUpdate_request_ini);

/* {{{ ini entries */
PHP_INI_BEGIN()
STD_PHP_INI_ENTRY("hidef.ini_path",  (char*)default_ini_path,  PHP_INI_SYSTEM, OnUpdateString,       ini_path,   zend_hidef_globals, hidef_globals)
STD_PHP_INI_ENTRY("hidef.data_path", (char*)NULL,              PHP_INI_SYSTEM, OnUpdateString,       data_path,  zend_hidef_globals, hidef_globals)

STD_PHP_INI_ENTRY("hidef.memory_limit",      "256M",           PHP_INI_SYSTEM, OnUpdateLongGEZero,   memory_limit,  zend_hidef_globals, hidef_globals)
STD_PHP_INI_BOOLEAN("hidef.enable_cli",      "1",              PHP_INI_SYSTEM, OnUpdateBool,         enable_cli,    zend_hidef_globals, hidef_globals)

PHP_INI_ENTRY("hidef.per_request_ini",      NULL,              PHP_INI_ALL,    OnUpdate_request_ini)

PHP_INI_END()
/* }}} */

/* {{{ arginfo static macro */
#if PHP_MAJOR_VERSION > 5 || PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 3
#define HIDEF_ARGINFO_STATIC
#else
#define HIDEF_ARGINFO_STATIC static
#endif
/* }}} */

/* {{{ arginfo */
HIDEF_ARGINFO_STATIC
ZEND_BEGIN_ARG_INFO(php_hidef_fetch_arginfo, 0)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, thaw)
ZEND_END_ARG_INFO()

HIDEF_ARGINFO_STATIC
ZEND_BEGIN_ARG_INFO(php_hidef_wrap_arginfo, 0)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()
/* }}} */


/* {{{ hidef_functions[]
 *
 * Every user visible function must have an entry in hidef_functions[].
 */
zend_function_entry hidef_functions[] = {
	PHP_FE(hidef_fetch,               php_hidef_fetch_arginfo)
	PHP_FE(hidef_wrap,                php_hidef_wrap_arginfo)
	{NULL, NULL, NULL}	/* Must be the last line in hidef_functions[] */
};
/* }}} */

/* {{{ hidef_module_entry
 */
zend_module_entry hidef_module_entry = {
	STANDARD_MODULE_HEADER,
	"hidef",
	hidef_functions,
	PHP_MINIT(hidef),
	PHP_MSHUTDOWN(hidef),
	PHP_RINIT(hidef),
	PHP_RSHUTDOWN(hidef),
	PHP_MINFO(hidef),
#if ZEND_MODULE_API_NO >= 20010901
	PHP_HIDEF_VERSION,
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_HIDEF
ZEND_GET_MODULE(hidef)
#endif

/* {{{ struct hidef_parser_ctxt */
typedef struct _hidef_parser_ctxt 
{
	int module_number;
	const char * filename;
	int type;
	int flags;
} hidef_parser_ctxt;
/* }}} */

/* {{{ hidef_zval_pfree */
static void hidef_zval_pfree(void *p) 
{
	TSRMLS_FETCH();
	frozen_array_free_zval_ptr((zval**)p, 1 TSRMLS_CC);
}
/* }}} */

/* {{{ hidef_define_constant */
static int hidef_define_constant(char *key, zval *v, hidef_parser_ctxt* ctxt TSRMLS_DC)
{
	zval value;
	zend_constant c;
	
	c.flags = ctxt->flags;
	c.module_number = ctxt->module_number;

	/* this is free'd by zend_register_constant, if the addition fails */
	value = *v;
	zval_copy_ctor(&value); 

	switch(ctxt->type) 
	{
		case IS_LONG:
		{
			convert_to_long(&value);
		}
		break;
		case IS_DOUBLE:
		{
			convert_to_double(&value);
		}
		break;
		case IS_BOOL:
		{
			convert_to_boolean(&value);
		}
		break;
		case IS_STRING:
		{
			convert_to_string(&value);
		}
		break;
	}
			
	/* TODO: check conversion results */
			
	c.value = value;
	if(ctxt->flags & CONST_PERSISTENT && Z_TYPE(value) == IS_STRING)
	{
		/* we do this because the zval_copy_ctor might gives us addresses that would be cleaned up after MINIT() */
		Z_STRVAL_P(&c.value) = zend_strndup(Z_STRVAL_P(&c.value), Z_STRLEN_P(&c.value));
	}
	c.name_len = strlen(key)+1;
	c.name = zend_strndup(key, c.name_len-1);
	
	if(zend_register_constant(&c TSRMLS_CC) == FAILURE) 
	{
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Constant '%s' redefined in %s line %d\n", key, ctxt->filename, INI_SCNG(lineno) - 1);
		if(ctxt->flags & CONST_PERSISTENT)
		{
			zval_dtor(&value);
		}
		return 0;
	}

	if(ctxt->flags & CONST_PERSISTENT)
	{
		zend_hash_add(hidef_constants_table, c.name, c.name_len, &c.value, sizeof(c.value), NULL);
		zval_dtor(&value);
	}
	

	return 1;
}
/* }}} */

#ifdef ZEND_BROKEN_INI_SCANNER
/* 
 * php 5.3 changed the ini reader code, which returns two
 * callbacks for "int xyz = 0". one with ("int", NULL, NULL)
 * and then next with ("xyz", "0"). And then changed it back
 * in php 5.3.2+ (in the following revision log)
 *
 * Zend/zend_ini_scanner.l?r1=286913&r2=291525
 *
 * But just to make things "simple", every function is called 
 * hidef_ini_parser_cb().
 */

/* {{{ hidef_ini_parser_cb */
static void hidef_ini_parser_cb(zval *arg1, zval *arg2, zval *arg3, int callback_type, void *arg TSRMLS_DC) 
{
	char *key;
	hidef_parser_ctxt * ctxt = (hidef_parser_ctxt*)arg;

	if(!arg1) return;

	key = Z_STRVAL_P(arg1);
	
	switch (callback_type) 
	{
		case ZEND_INI_PARSER_ENTRY: 
		{
			if(ctxt->type == IS_NULL && !arg2)
			{
				/* callback #1 */	
				if(strncmp(key, "int", strlen("int")) == 0)
				{
					ctxt->type = IS_LONG;
				} 
				else if(strncmp(key, "str", strlen("str")) == 0)
				{
					ctxt->type = IS_STRING;
				} 
				else if(strncmp(key, "float", strlen("float")) == 0)
				{
					ctxt->type = IS_DOUBLE;
				} 
				else if(strncmp(key, "bool", strlen("bool")) == 0)
				{
					ctxt->type = IS_BOOL;
				} 
				else
				{
					ctxt->type = IS_NULL;
				}
			}
			else if(ctxt->type != IS_NULL && arg2)
			{
				hidef_define_constant(key, arg2, ctxt TSRMLS_CC);
				ctxt->type = IS_NULL;
			}
		}
		break;
	
#ifdef ZEND_INI_PARSER_POP_ENTRY
		case ZEND_INI_PARSER_POP_ENTRY: 
		{
			/* do nothing here, I suppose */
		}
		break;
#endif

		case ZEND_INI_PARSER_SECTION:
		{
			/* ignore sectioning */
		}
		break;

		default:
			assert(0);
	}
}
/* }}} */
#else
/* 
 * php 5.2 & php 5.3.x returns one callback with ("int xyz","0"), except the signatures
 * of the callback are different.
 */
/* {{{ hidef_ini_parser_cb */
#ifdef ZEND_ENGINE_2_3
static void hidef_ini_parser_cb(zval *arg1, zval *arg2, zval *arg3, int callback_type, void *arg TSRMLS_DC) 
{
#else
static void hidef_ini_parser_cb(zval *arg1, zval *arg2, int callback_type, void *arg) 
{
	TSRMLS_FETCH();
#endif
	char *p;
	char *key = Z_STRVAL_P(arg1);
	int type = IS_STRING;
	hidef_parser_ctxt * ctxt = (hidef_parser_ctxt*)arg;

	
	switch (callback_type) 
	{
		case ZEND_INI_PARSER_ENTRY: 
		{
			if(!arg2)
			{
				return;
			}

			if(strncmp(key, "int ", strlen("int ")) == 0)
			{
				ctxt->type = IS_LONG;
			} 
			else if(strncmp(key, "str ", strlen("str ")) == 0)
			{
				ctxt->type = IS_STRING;
			} 
			else if(strncmp(key, "float ", strlen("float ")) == 0)
			{
				ctxt->type = IS_DOUBLE;
			} 
			else if(strncmp(key, "bool ", strlen("bool ")) == 0)
			{
				ctxt->type = IS_BOOL;
			} 
			else
			{
				/* syntax error, but we'll assume IS_STRING if there's no space in the name */
				ctxt->type = IS_NULL;
			}

			if(ctxt->type == IS_NULL)
			{
				if(strrchr(key, ' ') == NULL)
				{
					/* honest mistake, let it through as a string */
					p = key;
					ctxt->type = IS_STRING;
				}
				else
				{
					zend_error(E_WARNING, "hidef cannot parse .ini at %s", key);
					return;
				}
			}
			else
			{
				p = strrchr(key, ' ');
				if((p == NULL) || strlen(p) == 1)
				{
					/* TODO: error handling */
					return;	
				}

				p = p + 1; /* skip the space */
			}

			hidef_define_constant(p, arg2, ctxt TSRMLS_CC);

		}
		break;
	
#ifdef ZEND_INI_PARSER_POP_ENTRY
		case ZEND_INI_PARSER_POP_ENTRY: 
		{
			/* do nothing here, I suppose */
		}
		break;
#endif

		case ZEND_INI_PARSER_SECTION:
		{
			/* ignore sectioning */
		}
		break;

		default:
			assert(0);
	}
}
/* }}} */
#endif

/* {{{ hidef_walk_dir */
static int hidef_walk_dir(const char *path, const char *ext, hidef_walk_dir_cb cb, void *ctxt TSRMLS_DC)
{
	char file[MAXPATHLEN]={0,};
	int ndir, i, k;
	char *p = NULL;
	struct dirent **namelist = NULL;

	if ((ndir = php_scandir(path, &namelist, 0, php_alphasort)) > 0)
	{
		for (i = 0; i < ndir; i++) 
		{
			/* check for extension */
			if (!(p = strrchr(namelist[i]->d_name, '.')) 
					|| (p && strcmp(p, ext))) 
			{
				free(namelist[i]);
				continue;
			}
			snprintf(file, MAXPATHLEN, "%s%c%s", 
					path, DEFAULT_SLASH, namelist[i]->d_name);
			if(!cb(file, ctxt TSRMLS_CC))
			{
				goto cleanup;
			}
			free(namelist[i]);
		}
		free(namelist);
	}

	return 1;

cleanup:
	for(k = i; k < ndir; k++)
	{
		free(namelist[k]);
	}
	free(namelist);

	return 0;
}
/* }}} */

/* {{{ hidef_parse_ini */
static int hidef_parse_ini(const char *ini_file, void* pctxt TSRMLS_DC)
{
	struct stat sb;
	zend_file_handle fh = {0,};
	hidef_parser_ctxt *ctxt = (hidef_parser_ctxt*)pctxt;

	if (VCWD_STAT(ini_file, &sb) == 0 && S_ISREG(sb.st_mode)) 
	{
		if ((fh.handle.fp = VCWD_FOPEN(ini_file, "r"))) 
		{
			fh.filename = (char*)ini_file;
			ctxt->filename = ini_file;
			fh.type = ZEND_HANDLE_FP;
#ifdef ZEND_ENGINE_2_3
			zend_parse_ini_file(&fh, 1, ZEND_INI_SCANNER_NORMAL, (zend_ini_parser_cb_t) hidef_ini_parser_cb, ctxt TSRMLS_CC);
#else
			zend_parse_ini_file(&fh, 1, hidef_ini_parser_cb, ctxt);
#endif
			return 1;
		}
	}

	return 0;
}
/* }}} */

/* {{{ hidef_read_ini */
static int hidef_read_ini(hidef_parser_ctxt *ctxt TSRMLS_DC)
{
	const char *ini_path = NULL;

	if(HIDEF_G(ini_path)) 
	{
		ini_path = HIDEF_G(ini_path);
	}
	else if(sizeof(PHP_CONFIG_FILE_SCAN_DIR) > 1) 
	{
		ini_path = (const char *)default_ini_path;
	}
	else
	{
		return 0;
	}

	return hidef_walk_dir(ini_path, ".ini", hidef_parse_ini, ctxt TSRMLS_CC);
}
/* }}} */

static PHP_INI_MH(OnUpdate_request_ini) /* {{{ */
{
	hidef_parser_ctxt ctxt = {0,};
	ctxt.flags = CONST_CS;
	ctxt.module_number = PHP_USER_CONSTANT;

	if(stage == ZEND_INI_STAGE_STARTUP || stage == ZEND_INI_STAGE_ACTIVATE)
	{
		HIDEF_G(per_request_ini) = new_value; 
		/* read this later in RINIT */
		return SUCCESS;
	}

	if(stage != ZEND_INI_STAGE_RUNTIME)  
	{
		return FAILURE;
	}

	if(new_value != NULL)
	{
		if(!hidef_parse_ini(new_value, &ctxt TSRMLS_CC))
		{
			zend_error(E_WARNING, "hidef cannot read %s", new_value);
			return FAILURE;
		}
	}
    return SUCCESS;
}
/* }}} */


/* {{{ hidef_load_data */
static int hidef_load_data(const char *data_file, void* pctxt TSRMLS_DC)
{
	char *p;
	char key[MAXPATHLEN] = {0,};
	unsigned int key_len;
	zval *data;

	if(access(data_file, R_OK) != 0) 
	{
		/* maybe a broken symlink (skip and continue) */
		zend_error(E_WARNING, "hidef cannot read %s", data_file);
		return 1;
	}

	p = strrchr(data_file, DEFAULT_SLASH);

	if(p && p[1])
	{
		strlcpy(key, p+1, sizeof(key));
		p = strrchr(key, '.');

		if(p)
		{
			p[0] = '\0';
			key_len = strlen(key);

			zend_try 
			{
				data = frozen_array_unserialize(data_file TSRMLS_CC);
			} zend_catch
			{
				zend_error(E_ERROR, "Data corruption in %s, bailing out", data_file);
				zend_bailout();
			} zend_end_try();

			if((data == NULL) || zend_hash_add(hidef_data_hash, key, key_len+1, &data, sizeof(void*), NULL) == FAILURE)
			{
				zend_error(E_ERROR, "Unable to add %s to the hidef data hash", data_file);
				return 0;
			}
			return 1;
		}
	}

	return 0;
}
/* }}} */

/* {{{ hidef_read_data */
static int hidef_read_data(hidef_parser_ctxt *ctxt TSRMLS_DC)
{
	const char *data_path = NULL;

	if(HIDEF_G(data_path)) 
	{
		data_path = HIDEF_G(data_path);
	}
	else
	{
		return 0;
	}

	return hidef_walk_dir(data_path, ".data", hidef_load_data, ctxt TSRMLS_CC);
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(hidef)
{
	hidef_parser_ctxt ctxt = {0,};
	long memory_limit = 0;

	ZEND_INIT_MODULE_GLOBALS(hidef, php_hidef_init_globals, php_hidef_shutdown_globals);

	REGISTER_INI_ENTRIES();
	
	ctxt.flags = CONST_CS | CONST_PERSISTENT;
#ifdef CONST_CT_SUBST
	ctxt.flags |= CONST_CT_SUBST;
#endif

	ctxt.module_number = module_number;

	hidef_constants_table = pemalloc(sizeof(HashTable),1);
	hidef_data_hash = pemalloc(sizeof(HashTable),1);

	zend_hash_init(hidef_constants_table, 32,  NULL, NULL, 1);
	zend_hash_init(hidef_data_hash, 	  32,  NULL, hidef_zval_pfree, 1);

	memory_limit = PG(memory_limit);

	if(HIDEF_G(memory_limit) > memory_limit) 
	{
		zend_set_memory_limit((size_t) HIDEF_G(memory_limit)); /* 256 Mb default sounds sane right now */
	}
	else
	{
		HIDEF_G(memory_limit) = memory_limit;
	}

	if((strcmp(sapi_module.name, "cli") != 0) || HIDEF_G(enable_cli)) 
	{
		/* do not load data, but let hidef_wrap() work */
		hidef_read_ini(&ctxt TSRMLS_CC);
		hidef_read_data(&ctxt TSRMLS_CC);
	}

	zend_set_memory_limit((size_t)memory_limit); /* reset the memory limit */
	
	frozen_array_init(TSRMLS_C);

#ifdef ZTS
	HIDEF_G(p_tid) = tsrm_thread_id();
#else
	HIDEF_G(p_pid) = getpid();
#endif

#ifdef HAVE_MALLOC_TRIM
	malloc_trim(0); /* cleanup pages */
#endif

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 *  */
PHP_MSHUTDOWN_FUNCTION(hidef)
{
#ifdef ZTS
	THREAD_T tid = tsrm_thread_id();
	THREAD_T p_tid = HIDEF_G(p_tid);
	#define IS_ALLOC_THREAD() (memcmp(&(p_tid), &tid, sizeof(THREAD_T))==0)
#else
	pid_t pid = getpid();
	pid_t p_pid = HIDEF_G(p_pid);
	#define IS_ALLOC_THREAD() (p_pid == pid) 
#endif

	if(IS_ALLOC_THREAD())
	{
		/** Prevent multiple-free() calls of the same data.
		 * If free() is called on data items in each process, the 
		 * addition of that item to the libc internal free list, 
		 * results in new Copy-on-Write pages being spawned,
		 * increasing the memory-use. So it's counter-productive
		 * to cleanup in child processes.
		 */
		zend_hash_destroy(hidef_constants_table);
		zend_hash_destroy(hidef_data_hash);

		pefree(hidef_constants_table, 1);
		pefree(hidef_data_hash, 1);
	}

#ifdef ZTS
	ts_free_id(hidef_globals_id);
#else
	php_hidef_shutdown_globals(&hidef_globals);
#endif

	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION(hidef) */
PHP_RINIT_FUNCTION(hidef)
{
	hidef_parser_ctxt ctxt = {0,};
	ctxt.flags = CONST_CS;
	ctxt.module_number = PHP_USER_CONSTANT;
	if(HIDEF_G(per_request_ini)) 
	{
		if(!hidef_parse_ini(HIDEF_G(per_request_ini), &ctxt TSRMLS_CC))
		{
			zend_error(E_WARNING, "hidef cannot read %s", HIDEF_G(per_request_ini));
			/* this shouldn't be cause to fail, right? */
		}
	}
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION(hidef) */
PHP_RSHUTDOWN_FUNCTION(hidef)
{
#ifdef HAVE_MALLOC_TRIM
	malloc_trim(0);
#endif
	HIDEF_G(per_request_ini) = NULL;
	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(hidef)
{
	const char * ini_path = NULL;
	const char * data_path = "disabled";
	HashPosition pos;
	zend_constant *val;
	int module_number = zend_module->module_number;
	char tmp[31];
	
	php_info_print_table_start();

	if(HIDEF_G(ini_path)) 
	{
		ini_path = HIDEF_G(ini_path);	
	}
	else if(sizeof(PHP_CONFIG_FILE_SCAN_DIR) > 1)
	{
		ini_path = (const char *)default_ini_path;
	}
	else
	{
		php_info_print_table_header(2, "hidef support", "disabled");
		php_info_print_table_end();
		return;
	}
	
	if(HIDEF_G(data_path)) 
	{
		data_path = HIDEF_G(data_path);	
	}
	
	
	php_info_print_table_header(2, "hidef support", "enabled");
	php_info_print_table_row(2, "version", PHP_HIDEF_VERSION);
	php_info_print_table_row(2, "ini search path", ini_path);
	php_info_print_table_row(2, "data search path", data_path);
#ifdef CONST_CT_SUBST
	php_info_print_table_row(2, "substitution mode", "compile time");
#else
	php_info_print_table_row(2, "substitution mode", "runtime");
#endif
	php_info_print_table_row(2, "substitution mode", "runtime");

	snprintf(tmp, 31, "%0.0fM", HIDEF_G(memory_limit)/(1024*1024.0));
	php_info_print_table_row(2, "hidef memory_limit", tmp);

	php_info_print_table_end();
	php_info_print_table_start();
	php_info_print_table_header(2, "Constant", "Value");

	
	zend_hash_internal_pointer_reset_ex(EG(zend_constants), &pos);
	while (zend_hash_get_current_data_ex(EG(zend_constants), (void **) &val, &pos) != FAILURE) 
	{
		if (val->module_number == module_number) 
		{
			zval const_val = {{0,},};
			const_val = val->value;
			zval_copy_ctor(&const_val);
			convert_to_string(&const_val);
			
			php_info_print_table_row(2, val->name, Z_STRVAL_P(&const_val));
			zval_dtor(&const_val);
		}
		zend_hash_move_forward_ex(EG(zend_constants), &pos);
	}

	php_info_print_table_end();
}
/* }}} */

/* {{{ proto mixed hidef_fetch(string key [, bool thaw])
 */
PHP_FUNCTION(hidef_fetch) 
{
	zval **hentry;
	zval *wrapped;
	char *strkey;
	int strkey_len;
	zend_bool thaw = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|b", &strkey, &strkey_len, &thaw) == FAILURE)
	{
		return;
	}

	if(!HIDEF_G(enable_cli) && !strcmp(sapi_module.name, "cli")) 
	{
		zend_error(E_WARNING, "hidef_fetch('%s') is disabled on the cli", strkey);
		return;
	}

	if(zend_hash_find(hidef_data_hash, strkey, strkey_len+1, (void**)&hentry) == FAILURE)
	{
		return;
	}

	if(thaw)
	{
		wrapped = frozen_array_copy_zval_ptr(NULL, hentry[0], 0, NULL TSRMLS_CC);
	}
	else
	{
		wrapped = frozen_array_wrap_zval(hentry[0] TSRMLS_CC);
	}

	RETURN_ZVAL(wrapped, 0, 1);
}
/* }}} */

/* {{{ proto FrozenArray hidef_wrap(array value)
 */
PHP_FUNCTION(hidef_wrap) 
{
	zval *wrapped = NULL;
	zval *value;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &value) == FAILURE)
	{
		return;
	}

	if (Z_TYPE_P(value) == IS_OBJECT) {
		zend_error(E_ERROR, "Object is not a valid frozen array data type");
		zend_bailout();
	}

	if(Z_TYPE_P(value) == IS_ARRAY) {
		/* this is almost the same as _wrap_zval, except it does a refcount++ */
		wrapped = frozen_array_pin_zval(value TSRMLS_CC);
	}

	if (wrapped == NULL) {
		RETURN_NULL();
	}
	RETURN_ZVAL(wrapped, 0, 1);
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim>600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
