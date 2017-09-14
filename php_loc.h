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

#ifndef PHP_LOC_H
#define PHP_LOC_H

extern zend_module_entry loc_module_entry;
#define phpext_loc_ptr &loc_module_entry

#define PHP_LOC_VERSION "0.1.0" /* Replace with version number for your extension */

#ifdef PHP_WIN32
# define PHP_LOC_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
# define PHP_LOC_API __attribute__ ((visibility("default")))
#else
# define PHP_LOC_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif


ZEND_BEGIN_MODULE_GLOBALS(loc)
  zend_bool enable;
  char *shm_file;
  size_t k_msize;
  size_t v_msize;
ZEND_END_MODULE_GLOBALS(loc)

#define LOC_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(loc, v)

#if defined(ZTS) && defined(COMPILE_DL_LOC)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif  /* PHP_LOC_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
