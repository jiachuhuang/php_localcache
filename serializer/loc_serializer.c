
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/php_var.h" /* for serialize */
#include "zend_smart_str.h"
#include "loc_serializer.h"

int loc_serializer_pack(zval *data, smart_str *sb) {
	php_serialize_data_t s_data;

	PHP_VAR_SERIALIZE_INIT(s_data);
	php_var_serialize(sb, data, &s_data);
	PHP_VAR_SERIALIZE_DESTROY(s_data);	

	return 1;
}
/* }}} */

zval* loc_serializer_pack_unpack(char *content, size_t len, zval *return_val) {
	const unsigned char *p;
	php_unserialize_data_t us_data;
	p = (const unsigned char*)content;

	ZVAL_FALSE(return_val);
	PHP_VAR_UNSERIALIZE_INIT(us_data);
	if (!php_var_unserialize(return_val, &p, p + len,  &us_data)) {
		zval_ptr_dtor(return_val);
		PHP_VAR_UNSERIALIZE_DESTROY(us_data);
		return NULL;
	}
	PHP_VAR_UNSERIALIZE_DESTROY(us_data);

	return return_val;
}
/* }}} */