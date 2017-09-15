/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2017 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <time.h>

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_loc.h"
#include "zend_smart_str.h"
#include "SAPI.h"

#include "c_cache/c_cache.h"
#include "c_cache/c_storage.h"

#include "serializer/loc_serializer.h"
#include "lzf/lzf.h"
#include "lzf/lzfP.h"

extern c_shared_header *shared_header;

zend_class_entry *loc_class_ce;

ZEND_DECLARE_MODULE_GLOBALS(loc)

ZEND_BEGIN_ARG_INFO_EX(arginfo_loc_setter, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, value)
	ZEND_ARG_INFO(0, ttl)	
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_loc_get, 0, 0, 1)
	ZEND_ARG_INFO(0, key)	
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_loc_delete, 0, 0, 1)
	ZEND_ARG_INFO(0, key)	
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_loc_void, 0, 0, 0)
ZEND_END_ARG_INFO()

/* True global resources - no need for thread safety here */
static int le_loc;

static PHP_INI_MH(OnChangeKeysMemoryLimit) /* {{{ */ {
	if (new_value) {
		LOC_G(k_msize) = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
	}
	return SUCCESS;
}
/* }}} */

static PHP_INI_MH(OnChangeValsMemoryLimit) /* {{{ */ {
	if (new_value) {
		LOC_G(v_msize) = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
	}
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
    STD_PHP_INI_BOOLEAN("loc.enable",      "1", PHP_INI_ALL, OnUpdateLong, enable, zend_loc_globals, loc_globals)
    STD_PHP_INI_ENTRY("loc.keys_memory_size", "4M", PHP_INI_SYSTEM, OnChangeKeysMemoryLimit, k_msize, zend_loc_globals, loc_globals)
    STD_PHP_INI_ENTRY("loc.values_memory_size", "64M", PHP_INI_SYSTEM, OnChangeValsMemoryLimit, v_msize, zend_loc_globals, loc_globals)
    STD_PHP_INI_ENTRY("loc.shm_file", "/tmp/shm.file.dat", PHP_INI_ALL, OnUpdateString, shm_file, zend_loc_globals, loc_globals)
PHP_INI_END()

/* }}} */

/* {{{ php_loc_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_loc_init_globals(zend_loc_globals *loc_globals)
{
	loc_globals->global_value = 0;
	loc_globals->global_string = NULL;
}
*/
/* }}} */

int loc_set(char *key, size_t key_len, zval *value, long ttl, int add) {
	time_t tv;
	int flag = Z_TYPE_P(value);
	int result = 0;
	void *compress;
	unsigned int compress_len, out_len;
	smart_str buf = {0};

	if(key_len > C_STORAGE_MAX_KEY_LEN) {
		php_error_docref(NULL, E_WARNING, "Key too big, can not storage");
		return 0;
	}

	add = add > 0? 1: 0;

	tv = time((time_t *)NULL);
	if(ttl <= 0) {
		ttl = tv + 7200;
	} else {
		ttl += tv;
	}
	switch(flag) {
		case IS_TRUE:
		case IS_FALSE:
			result = c_storage_update(key, key_len, (void *)&flag, sizeof(int), ttl, flag, add, tv);
			break;
		case IS_LONG:
			result = c_storage_update(key, key_len, (void *)&Z_LVAL_P(value), sizeof(long), ttl, flag, add, tv);
			break;
		case IS_DOUBLE:
			result = c_storage_update(key, key_len, (void *)&Z_DVAL_P(value), sizeof(double), ttl, flag, add, tv);
			break;
		case IS_STRING:
		case IS_CONSTANT: 
			if(Z_STRLEN_P(value) > C_STORAGE_SEGMENT_BODY_SIZE(shared_header)) {
				php_error_docref(NULL, E_WARNING, "Value too long(%d), can not be storaged", Z_STRLEN_P(value));
				return 0;				
			}

			if(Z_STRLEN_P(value) > LOC_VAL_COMPRESS_MIN_LEN) {
				out_len = Z_STRLEN_P(value) - 4;
				compress = malloc(out_len);
				compress_len = lzf_compress((void*)Z_STRVAL_P(value), Z_STRLEN_P(value), compress, out_len);
				if(!compress_len || compress_len > out_len) {
					php_error_docref(NULL, E_WARNING, "Compressd fail");
					free(compress);
					return 0;
				}

				flag |= (Z_STRLEN_P(value) << LOC_VAL_ORIG_LEN_SHIT);
				result = c_storage_update(key, key_len, compress, compress_len, ttl, flag, add, tv);
				free(compress);
			} else {
				result = c_storage_update(key, key_len, (void*)Z_STRVAL_P(value), Z_STRLEN_P(value), ttl, flag, add, tv);
			}
			break;
#ifdef IS_CONSTANT_ARRAY
		case IS_CONSTANT_ARRAY:
#endif				
		case IS_ARRAY:
		case IS_OBJECT:

			loc_serializer_pack(value, &buf);
			if(buf.s->len > C_STORAGE_SEGMENT_BODY_SIZE(shared_header)) {
				php_error_docref(NULL, E_WARNING, "Value too long(%d), can not be storaged", buf.s->len);
				smart_str_free(&buf);
				return 0;						
			}

			if(buf.s->len > LOC_VAL_COMPRESS_MIN_LEN) {
				out_len = buf.s->len - 4;
				compress = malloc(out_len);
				compress_len = lzf_compress((void*)ZSTR_VAL(buf.s), ZSTR_LEN(buf.s), compress, out_len);
				if(!compress_len || compress_len > out_len) {
					php_error_docref(NULL, E_WARNING, "Compressd fail");
					free(compress);
					smart_str_free(&buf);
					return 0;
				}

				flag |= (buf.s->len << LOC_VAL_ORIG_LEN_SHIT);
				result = c_storage_update(key, key_len, compress, compress_len, ttl, flag, add, tv);
				free(compress);					
			} else {
				result = c_storage_update(key, key_len, (void*)ZSTR_VAL(buf.s), ZSTR_LEN(buf.s), ttl, flag, add, tv);
				smart_str_free(&buf);
			}
			break;
		case IS_RESOURCE:
			php_error_docref(NULL, E_WARNING, "Type 'IS_RESOURCE' cannot be stored");
			break;
		default:
			php_error_docref(NULL, E_WARNING, "Unsupported valued type to be stored '%d'", flag);
			break;
	}

	return result;
}

/* {{{ loc_functions[]
 *
 * Every user visible function must have an entry in loc_functions[].
 */
PHP_METHOD(loc, __construct) {
	if (!LOC_G(enable)) {
		return;
	}	
}
/* }}} */

PHP_METHOD(loc, get) {

	char *key;
	size_t key_len;
	void *data;
	unsigned int size, flag;
	time_t tv;

	if(!LOC_G(enable)) {
		RETURN_FALSE; 
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &key, &key_len) == FAILURE) {
		return;
	}	

	if(key_len > C_STORAGE_MAX_KEY_LEN) {
		php_error_docref(NULL, E_WARNING, "Key too big, can not storage");
		RETURN_FALSE;
	}

	tv = time((time_t *)NULL);

	if(c_storage_find(key, key_len, &data, &size, &flag, tv)) {
		switch((flag & LOC_VAL_TPYE_MASK)) {
			case IS_TRUE:
				if(size == sizeof(int)) {
					ZVAL_TRUE(return_value);
				}
				free(data);
				break;
			case IS_FALSE:
				if(size == sizeof(int)) {
					ZVAL_FALSE(return_value);
				}
				free(data);
				break;		
			case IS_LONG:
				if(size == sizeof(long)) {
					ZVAL_LONG(return_value, *(long*)data);
				}	
				free(data);
				break;
			case IS_DOUBLE:
				if(size == sizeof(double)) {
					ZVAL_DOUBLE(return_value, *(double*)data);
				}	
				free(data);
				break;
			case IS_STRING:
			case IS_CONSTANT:
				if(flag & LOC_VAL_COMPRESS) {
					unsigned int orig_len = flag >> LOC_VAL_ORIG_LEN_SHIT;
					void *orig_data;
					unsigned int length;
					orig_data = malloc(orig_len);
					length = lzf_decompress(data, size, orig_data, orig_len);
					if(!length) {
						php_error_docref(NULL, E_WARNING, "Loc::get decompress value error");
						free(orig_data);
						free(data);
						return_value = NULL;
						RETURN_FALSE;
					}

					ZVAL_STRINGL(return_value, (char*)orig_data, orig_len);
					free(orig_data);
					free(data);
				} else {
					ZVAL_STRINGL(return_value, (char*)data, size);	
					free(data);						
				}
				break;
#ifdef IS_CONSTANT_ARRAY
			case IS_CONSTANT_ARRAY:
#endif				
			case IS_ARRAY:
			case IS_OBJECT:
				if(flag & LOC_VAL_COMPRESS) {
					unsigned int orig_len = flag >> LOC_VAL_ORIG_LEN_SHIT;
					void *orig_data;
					unsigned int length;
					orig_data = malloc(orig_len);
					length = lzf_decompress(data, size, orig_data, orig_len);
					if(!length) {
						php_error_docref(NULL, E_WARNING, "Loc::get decompress value error");
						free(orig_data);
						free(data);
						return_value = NULL;
						RETURN_FALSE;
					}

					free(data);
					data = orig_data;
					size = orig_len;
				} 

				return_value = loc_serializer_pack_unpack((char*)data, size, return_value);
				if(!return_value) {
					php_error_docref(NULL, E_WARNING, "Loc::get unserializer fail");
					RETURN_FALSE;
				}

				free(data);
				break;
			default:
				free(data);
				php_error_docref(NULL, E_WARNING, "Unexpected valued type '%d'", flag);
				return_value = NULL;
				break;			
		}
	} else {
		return_value = NULL;
	}
}
/* }}} */

PHP_METHOD(loc, set) {
	char *key;
	size_t key_len;
	long ttl = 0;
	zval *data;

	if(!LOC_G(enable)) {
		RETURN_FALSE; 
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "sz|l", &key, &key_len, &data, &ttl) == FAILURE) {
		return;
	}

	if(loc_set(key, key_len, data, ttl, 0)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}

}
/* }}} */

PHP_METHOD(loc, add) {
	char *key;
	size_t key_len;
	long ttl = 0;
	zval *data;

	if(!LOC_G(enable)) {
		RETURN_FALSE; 
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "sz|l", &key, &key_len, &data, &ttl) == FAILURE) {
		return;
	}

	if(loc_set(key, key_len, data, ttl, 1)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}

}
/* }}} */

PHP_METHOD(loc, delete) {
	char *key;
	size_t key_len;

	if(!LOC_G(enable)) {
		RETURN_FALSE; 
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &key, &key_len) == FAILURE) {
		return;
	}

	if(c_storage_delete(key, key_len)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

PHP_METHOD(loc, flush) {

	if(!LOC_G(enable)) {
		RETURN_FALSE; 
	}

	c_storage_flush();

	RETURN_TRUE;
}
/* }}} */

zend_function_entry loc_methods[] = {
	PHP_ME(loc, __construct, arginfo_loc_void, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)	
	PHP_ME(loc, get, arginfo_loc_get, ZEND_ACC_PUBLIC)	
	PHP_ME(loc, set, arginfo_loc_setter, ZEND_ACC_PUBLIC)	
	PHP_ME(loc, add, arginfo_loc_setter, ZEND_ACC_PUBLIC)	
	PHP_ME(loc, delete, arginfo_loc_delete, ZEND_ACC_PUBLIC)	
	PHP_ME(loc, flush, arginfo_loc_void, ZEND_ACC_PUBLIC)	
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(loc)
{
	char *msg;
	zend_class_entry ce;

	REGISTER_INI_ENTRIES();

	if(LOC_G(enable)) {
		if(!c_storage_startup(LOC_G(shm_file), LOC_G(k_msize), LOC_G(v_msize), &msg)) {
			php_error(E_ERROR, "Shared memory allocator startup failed at '%s': %s", msg, strerror(errno));
			return FAILURE;
		} 
	}

	INIT_CLASS_ENTRY(ce, "Loc", loc_methods);
	loc_class_ce = zend_register_internal_class(&ce);
	
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(loc)
{
	UNREGISTER_INI_ENTRIES();

	if(LOC_G(enable)) {
		c_storage_shutdown();
	}
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(loc)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "loc support", "enabled");
	php_info_print_table_row(2, "Version", PHP_LOC_VERSION);
	php_info_print_table_row(2, "Shm file", LOC_G(shm_file));
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ loc_module_entry
 */
zend_module_entry loc_module_entry = {
	STANDARD_MODULE_HEADER,
	"loc",
	NULL,
	PHP_MINIT(loc),
	PHP_MSHUTDOWN(loc),
	NULL,		
	NULL,
	PHP_MINFO(loc),
	PHP_LOC_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_LOC
ZEND_GET_MODULE(loc)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
