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

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_loc.h"

#include "c_cache/c_cache.h"
#include "c_cache/c_storage.h"

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

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_loc_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_loc_compiled)
{
	char *arg = NULL;
	size_t arg_len, len;
	zend_string *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	strg = strpprintf(0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "loc", arg);

	RETURN_STR(strg);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and
   unfold functions in source code. See the corresponding marks just before
   function definition, where the functions purpose is also documented. Please
   follow this convention for the convenience of others editing your code.
*/


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

/* {{{ loc_functions[]
 *
 * Every user visible function must have an entry in loc_functions[].
 */
PHP_METHOD(loc, __construct) {
	php_error_docref(NULL, E_WARNING, "c_storage_segment_num: %d", C_STORAGE_SEGMENT_NUM(shared_header));
}
/* }}} */

PHP_METHOD(loc, get) {

}
/* }}} */

PHP_METHOD(loc, set) {

}
/* }}} */

PHP_METHOD(loc, add) {

}
/* }}} */

PHP_METHOD(loc, delete) {

}
/* }}} */

PHP_METHOD(loc, flush) {

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
