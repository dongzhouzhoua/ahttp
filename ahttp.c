/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
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
#include "php_ahttp.h"

#include "event2/event-config.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"
#include "event2/event.h"
#include "event2/http.h"
#include "event2/http_struct.h"

#define URL_MAX 4096

/* If you declare any globals in php_ahttp.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(ahttp)
*/

/* True global resources - no need for thread safety here */
static int le_ahttp;

zend_class_entry *ahttp_entry;


struct readcb_arg
{
	int idx;
	zval *this;
	struct evbuffer *evbuf;
	int len;
};

int php_le_ahttp(void)
{
	return le_ahttp;
}

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("ahttp.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_ahttp_globals, ahttp_globals)
    STD_PHP_INI_ENTRY("ahttp.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_ahttp_globals, ahttp_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_ahttp_compiled(string arg)
   Return a string to confirm that the module is compiled in */

/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and
   unfold functions in source code. See the corresponding marks just before
   function definition, where the functions purpose is also documented. Please
   follow this convention for the convenience of others editing your code.
*/

static void
http_request_done(struct evhttp_request *req, void *ctx)
{
	zend_class_entry *ce;
	zval *result_arr, *rv, *rv_0, *rv_err, result, *z_rsrc, *error_arr, *response_arr, response_res;
	struct readcb_arg *arg = ctx;

	ce = Z_OBJCE_P(arg->this);
	result_arr = zend_read_property(ce, arg->this, "result_arr", sizeof("result_arr") - 1, 0 TSRMLS_CC, rv);
	response_arr = zend_read_property(ce, arg->this, "response_arr", sizeof("response_arr") -1, 0 TSRMLS_CC, rv_err);
	array_init(&response_res);

	if(req)
	{
		struct evbuffer *input = evhttp_request_get_input_buffer(req);
		size_t len = evbuffer_get_length(input);
		zend_string *strg;
		zend_resource *rsrc_base;
		struct event_base *base;

		strg = strpprintf(len, "%s", evbuffer_pullup(input, len));
		evbuffer_drain(input, len);
		ZVAL_NEW_STR(&result, strg);

		add_assoc_long_ex(&response_res, "http_code", sizeof("http_code") - 1, evhttp_request_get_response_code(req));
		add_assoc_string_ex(&response_res, "message", sizeof("message") - 1, "");

	}
	else
	{

		add_assoc_long_ex(&response_res, "http_code", sizeof("http_code") - 1, 503);
		add_assoc_string_ex(&response_res, "message", sizeof("message") - 1, "Service Unavailable");
	}
	add_index_zval(response_arr, arg->idx, &response_res);
	add_index_zval(result_arr, arg->idx, &result);
}



/* {{{ php_ahttp_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_ahttp_init_globals(zend_ahttp_globals *ahttp_globals)
{
	ahttp_globals->global_value = 0;
	ahttp_globals->global_string = NULL;
}
*/
/* }}} */

PHP_METHOD(ahttp, __construct)
{
	zval url_arr, result_arr, response_arr, error_arr, header_arr;
	struct event_base *base;
	zend_resource *rsrc;
	int le_ahttp = php_le_ahttp();

	struct event_config *evconfig = event_config_new();
	event_config_avoid_method(evconfig ,"select");
	event_config_avoid_method(evconfig ,"poll");

	base = event_base_new_with_config(evconfig);
	event_config_free(evconfig);

	rsrc = zend_register_resource(base, le_ahttp);
	array_init(&url_arr);
	array_init(&result_arr);
	array_init(&response_arr);

	add_property_resource_ex(getThis(), "base", sizeof("base") - 1, rsrc);
	add_property_long_ex(getThis(), "time_out", sizeof("time_out") -1, 2000);
	add_property_zval_ex(getThis(), "url_arr", sizeof("url_arr") - 1,  &url_arr);
	add_property_zval_ex(getThis(), "result_arr", sizeof("result_arr") -1, &result_arr);
	add_property_zval_ex(getThis(), "response_arr", sizeof("response_arr") -1, &response_arr);

	Z_DELREF_P(&url_arr);
	Z_DELREF_P(&result_arr);
	Z_DELREF_P(&response_arr);
	Z_DELREF_P(&header_arr);

}

PHP_METHOD(ahttp, get)
{
	char *url = NULL;
	size_t url_len;

	zval *rv_0, *z_arr, url_arr, *opt_arr = NULL;
	zend_class_entry *ce;
	HashTable *url_ht;

	ce = Z_OBJCE_P(getThis());
	z_arr = zend_read_property(ce, getThis(), "url_arr", sizeof("url_arr") - 1, 0 TSRMLS_CC, rv_0);
	url_ht = Z_ARR_P(z_arr);

	if(url_ht->nNumUsed == 50)
	{
		php_error_docref(NULL, E_ERROR, "too many request, should  be less than 50");
	}

	if(zend_parse_parameters(ZEND_NUM_ARGS(), "s|a", &url, &url_len, &opt_arr) == FAILURE)
	{
		return;
	}

	{
		char *method = "get";
		struct evhttp_uri *http_uri;
		const char *scheme, *host, *path, *query;
		int port;

		http_uri = evhttp_uri_parse(url);
		if (http_uri == NULL) {
			php_error_docref(NULL, E_ERROR, "malformed url");
		}
		scheme = evhttp_uri_get_scheme(http_uri);
		host = evhttp_uri_get_host(http_uri);
		port = evhttp_uri_get_port(http_uri);
		path = evhttp_uri_get_path(http_uri);
		query = evhttp_uri_get_query(http_uri);

		if (scheme == NULL || (strcasecmp(scheme, "https") != 0 && strcasecmp(scheme, "http") != 0)) {
			php_error_docref(NULL, E_ERROR, "url must be http or https");
		}

		if (host == NULL) {
			php_error_docref(NULL, E_ERROR, "url must have a host");
		}

		array_init(&url_arr);
		add_assoc_str(&url_arr, "url", strpprintf(url_len, "%s", url));
		add_assoc_str(&url_arr, "method", strpprintf(strlen(method), "%s", method));


		// 处理参数选项
		if(opt_arr)
		{
			zval *header;
			zend_string *header_key = strpprintf(sizeof("header") - 1, "%s", "header");
			if(zend_hash_exists(Z_ARRVAL_P(opt_arr), header_key))
			{
				header = zend_hash_find(Z_ARRVAL_P(opt_arr), header_key);
				add_assoc_zval(&url_arr, "header", header);
				Z_ADDREF_P(header);
			}
			zend_string_free(header_key);
		}

		add_next_index_zval(z_arr, &url_arr);
	}
}


PHP_METHOD(ahttp, post)
{
	char *url = NULL;
	size_t url_len;

	zval *rv_0, *z_arr, url_arr, *opt_arr = NULL;
	zend_class_entry *ce;
	HashTable *url_ht;

	ce = Z_OBJCE_P(getThis());
	z_arr = zend_read_property(ce, getThis(), "url_arr", sizeof("url_arr") - 1, 0 TSRMLS_CC, rv_0);
	url_ht = Z_ARR_P(z_arr);

	if(url_ht->nNumUsed == 50)
	{
		php_error_docref(NULL, E_ERROR, "too many request, should  be less than 50");
	}

	if(zend_parse_parameters(ZEND_NUM_ARGS(), "s|a", &url, &url_len, &opt_arr) == FAILURE)
	{
		return;
	}

	{
		char *method = "post";
		struct evhttp_uri *http_uri;
		const char *scheme, *host, *path, *query;
		int port;

		http_uri = evhttp_uri_parse(url);
		if (http_uri == NULL) {
			php_error_docref(NULL, E_ERROR, "malformed url");
		}
		scheme = evhttp_uri_get_scheme(http_uri);
		host = evhttp_uri_get_host(http_uri);
		port = evhttp_uri_get_port(http_uri);
		path = evhttp_uri_get_path(http_uri);
		query = evhttp_uri_get_query(http_uri);

		if (scheme == NULL || (strcasecmp(scheme, "https") != 0 && strcasecmp(scheme, "http") != 0)) {
			php_error_docref(NULL, E_ERROR, "url must be http or https");
		}

		if (host == NULL) {
			php_error_docref(NULL, E_ERROR, "url must have a host");
		}

		array_init(&url_arr);
		add_assoc_str(&url_arr, "url", strpprintf(url_len, "%s", url));
		add_assoc_str(&url_arr, "method", strpprintf(strlen(method), "%s", method));

		// 处理参数选项
		if(opt_arr)
		{
			zval *header, *post_data;
			zend_string *header_key = strpprintf(sizeof("header") - 1, "%s", "header");
			zend_string *data_key = strpprintf(sizeof("data") - 1, "%s", "data");
			if(zend_hash_exists(Z_ARRVAL_P(opt_arr), header_key))
			{
				header = zend_hash_find(Z_ARRVAL_P(opt_arr), header_key);
				add_assoc_zval(&url_arr, "header", header);
				Z_ADDREF_P(header);
			}

			if(zend_hash_exists(Z_ARRVAL_P(opt_arr), data_key))
			{
				post_data = zend_hash_find(Z_ARRVAL_P(opt_arr), data_key);
				add_assoc_zval(&url_arr, "data", post_data);
				Z_ADDREF_P(post_data);
			}

			zend_string_free(header_key);
			zend_string_free(data_key);
		}

		add_next_index_zval(z_arr, &url_arr);
	}
}

PHP_METHOD(ahttp, wait_reply)
{
	struct event_base *base;
	zval *rv, *rv_1, *rv_2, *z_base, *url_arr, *time_out;
	zend_resource *base_rsrc;
	HashTable *url_ht;
	int sec, msec;

	zend_class_entry *ce = Z_OBJCE_P(getThis());
	z_base = zend_read_property(ce, getThis(), "base", sizeof("base") - 1, 0 TSRMLS_CC, rv);
	base_rsrc = Z_RES_P(z_base);
	base = zend_fetch_resource(base_rsrc, AHTTP_RES_NAME, le_ahttp);

	url_arr = zend_read_property(ce, getThis(), "url_arr", sizeof("url_arr") - 1, 0 TSRMLS_CC, rv_1);
	url_ht = Z_ARR_P(url_arr);

	time_out = zend_read_property(ce, getThis(), "time_out", sizeof("time_out") - 1, 0 TSRMLS_CC, rv_2);

	struct evhttp_uri *http_uri[url_ht->nNumUsed], *location[url_ht->nNumUsed];
	const char *scheme[url_ht->nNumUsed], *host[url_ht->nNumUsed], *path[url_ht->nNumUsed],
	*query[url_ht->nNumUsed];
	int port[url_ht->nNumUsed];
	char *port_s[url_ht->nNumUsed];
	struct readcb_arg cb_arg[url_ht->nNumUsed];
	struct evhttp_connection *evcon[url_ht->nNumUsed];
	struct evhttp_request *req[url_ht->nNumUsed];
	char buffer[url_ht->nNumUsed][URL_MAX];
	char uri[url_ht->nNumUsed][256];
	struct bufferevent *bev[url_ht->nNumUsed];
	struct evkeyvalq *output_headers[url_ht->nNumUsed];
	struct evbuffer *output_buffer[url_ht->nNumUsed];

	zend_string *url_key = strpprintf(sizeof("url") - 1, "%s", "url");
	zend_string *method_key = strpprintf(sizeof("method") - 1, "%s", "method");
	zend_string *header_key = strpprintf(sizeof("header") - 1, "%s", "header");
	zend_string *data_key = strpprintf(sizeof("data") - 1, "%s", "data");

	for(zend_hash_internal_pointer_reset(url_ht); zend_hash_has_more_elements(url_ht) == SUCCESS; zend_hash_move_forward(url_ht))
	{
		zend_string *key;
		uint keylen;
		zend_ulong idx;
		int type;
		zval *pzval, tmpcopy, *purl, *method;

		type = zend_hash_get_current_key(url_ht, &key, &idx);
		pzval = zend_hash_get_current_data(url_ht);


		purl = zend_hash_find(Z_ARRVAL_P(pzval), url_key);
		method = zend_hash_find(Z_ARRVAL_P(pzval), method_key);

		char *str_method = Z_STRVAL_P(method);

		tmpcopy = *purl;
		convert_to_string(&tmpcopy);

		http_uri[idx] = evhttp_uri_parse(Z_STRVAL(tmpcopy));
		scheme[idx] = evhttp_uri_get_scheme(http_uri[idx]);
		host[idx] = evhttp_uri_get_host(http_uri[idx]);
		port[idx] = evhttp_uri_get_port(http_uri[idx]);
		path[idx] = evhttp_uri_get_path(http_uri[idx]);
		query[idx] = evhttp_uri_get_query(http_uri[idx]);

		if (port[idx] == -1) {
			port[idx] = (strcasecmp(scheme[idx], "http") == 0) ? 80 : 443;
		}

		if (strlen(path[idx]) == 0) {
			path[idx] = "/";
			evhttp_uri_set_path(http_uri[idx], path[idx]);
		}

		location[idx] = http_uri[idx];
		evhttp_uri_set_scheme(location[idx], NULL);
		evhttp_uri_set_userinfo(location[idx], 0);
		evhttp_uri_set_host(location[idx], NULL);
		evhttp_uri_set_port(location[idx], -1);

		evcon[idx] = evhttp_connection_base_new(base, NULL, host[idx], port[idx]);

		cb_arg[idx] = (struct readcb_arg){
			.idx = idx,
			.this = getThis(),
			.evbuf = evbuffer_new(),
			.len = 0,
		};

		req[idx] = evhttp_request_new(http_request_done, &cb_arg[idx]);
		output_headers[idx] = evhttp_request_get_output_headers(req[idx]);
		evhttp_add_header(output_headers[idx], "Host", host[idx]);

		// 自定义头信息
		{
			if(zend_hash_exists(Z_ARRVAL_P(pzval), header_key))
			{
				zval *header, *hv_zval, h_tmpcopy, hk_con;
				HashTable *header_ht;
				int hk_type;
				zend_string *h_key;

				header = zend_hash_find(Z_ARRVAL_P(pzval), header_key);
				header_ht = Z_ARR_P(header);

				for(zend_hash_internal_pointer_reset(header_ht); zend_hash_has_more_elements(header_ht) == SUCCESS; zend_hash_move_forward(header_ht))
				{
					zend_ulong h_idx;
					hk_type = zend_hash_get_current_key(header_ht, &h_key, &h_idx);
					hv_zval =zend_hash_get_current_data(header_ht);
					if(hk_type ==  HASH_KEY_IS_STRING)
					{
						h_tmpcopy = *hv_zval;
						convert_to_string(&h_tmpcopy);
						ZVAL_NEW_STR(&hk_con, h_key);
						evhttp_add_header(output_headers[idx], Z_STRVAL(hk_con), Z_STRVAL(h_tmpcopy));
					}
					zval_dtor(&h_tmpcopy);
				}
			}
		}
		// 自定义头信息end


		// 处理post请求
		if(strcasecmp(str_method, "post")  == 0)
		{
			if(zend_hash_exists(Z_ARRVAL_P(pzval), data_key))
			{
				char buf[1024];
				struct evbuffer *output_buffer = evhttp_request_get_output_buffer(req[idx]);
				zval *post_data = zend_hash_find(Z_ARRVAL_P(pzval), data_key);
				evbuffer_add(output_buffer, Z_STRVAL_P(post_data), Z_STRLEN_P(post_data));
				int len = Z_STRLEN_P(post_data);
				evutil_snprintf(buf, sizeof(buf)-1, "%lu", (unsigned long)len);
				evhttp_add_header(output_headers[idx], "Content-Length", buf);
			}
		}
		// 处理post请求 end

		evhttp_add_header(output_headers[idx], "Connection", "close");

		//evhttp_connection_set_timeout
		evhttp_connection_set_timeout(evcon[idx], Z_LVAL_P(time_out));
		evhttp_connection_set_retries(evcon[idx], 2);
		evhttp_make_request(evcon[idx], req[idx], (strcasecmp(str_method, "get")  == 0) ? EVHTTP_REQ_GET : EVHTTP_REQ_POST, evhttp_uri_join(location[idx], buffer[idx], URL_MAX));
		zval_dtor(&tmpcopy);
	}

	zend_string_free(url_key);
	zend_string_free(method_key);
	zend_string_free(header_key);
	zend_string_free(data_key);
	event_base_dispatch(base);
}

PHP_METHOD(ahttp, result)
{
	zval *result_arr, *rv;
	zend_class_entry *ce;

	ce = Z_OBJCE_P(getThis());
	result_arr = zend_read_property(ce, getThis(), "result_arr", sizeof("result_arr") - 1, 0 TSRMLS_CC, rv);
	Z_ADDREF_P(result_arr);
	RETVAL_ARR(Z_ARRVAL_P(result_arr));
}


PHP_METHOD(ahttp, set_time_out)
{

	long msec;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "l", &msec) == FAILURE)
	{
		return;
	}

	zend_class_entry *ce;
	zval *z_timeout, *rv_0;
	zend_resource *rsrc_base;
	struct event_base *base;

	ce = Z_OBJCE_P(getThis());
	zend_update_property_long(ce, getThis(), "time_out", sizeof("time_out") - 1, msec);
}
const zend_function_entry ahttp_functions[] = {
		PHP_ME(ahttp, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
		PHP_ME(ahttp, get, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(ahttp, post, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(ahttp, result, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(ahttp, set_time_out, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(ahttp, wait_reply, NULL, ZEND_ACC_PUBLIC)
};

static void _php_event_base_dtor(zend_resource *rsrc) /* {{{ */
{
	struct event_base *base = (struct event_base*)rsrc->ptr;
	event_base_free(base);
}

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(ahttp)
{
	/* If you have INI entries, uncomment these lines
	REGISTER_INI_ENTRIES();
	*/

	le_ahttp = zend_register_list_destructors_ex(_php_event_base_dtor, NULL, AHTTP_RES_NAME, module_number);

	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "ahttp", ahttp_functions);
	ahttp_entry = zend_register_internal_class(&ce);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(ahttp)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(ahttp)
{
#if defined(COMPILE_DL_AHTTP) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(ahttp)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(ahttp)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "ahttp support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ ahttp_functions[]
 *
 * Every user visible function must have an entry in ahttp_functions[].
 */
PHP_FUNCTION(confirm_ahttp_compiled){}
const zend_function_entry ahttp_m_functions[] = {
	PHP_FE(confirm_ahttp_compiled,	NULL)
	PHP_FE_END
};
/* }}} */

/* {{{ ahttp_module_entry
 */
zend_module_entry ahttp_module_entry = {
	STANDARD_MODULE_HEADER,
	"ahttp",
	NULL,//ahttp_functions,
	PHP_MINIT(ahttp),
	PHP_MSHUTDOWN(ahttp),
	PHP_RINIT(ahttp),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(ahttp),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(ahttp),
	PHP_AHTTP_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_AHTTP
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(ahttp)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
