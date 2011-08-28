#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <libmemcached/memcached.h>


typedef struct {
    ngx_str_t host;
    /* ngx_int_t port; */
    ngx_str_t port;
    memcached_st *memc;
} ngx_http_mod_memcache_conf_t;

static u_char ngx_hello_string[] = "Hello, world!";
static ngx_int_t ngx_http_mod_memcache_handler(ngx_http_request_t *r);

static char *ngx_http_mod_memcache_usecache(ngx_conf_t *cf, ngx_command_t *cmd, void *conf){
    ngx_http_core_loc_conf_t *clcf;
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_mod_memcache_handler; /* handler to process the 'hello' directive */
    return NGX_CONF_OK;
}

static void *ngx_http_mod_memcache_create_main_conf(ngx_conf_t *cf){
    printf("Create Main\n");
    ngx_http_mod_memcache_conf_t *conf;
    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_mod_memcache_conf_t));
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }
    /* conf->port = 0; */
    return conf;
}

static char *ngx_http_mod_memcache_init_main_conf(ngx_conf_t *cf, void *conf)
{
    printf("IN INIT MAIN\n");
    ngx_http_mod_memcache_conf_t *ptr = (ngx_http_mod_memcache_conf_t *)conf;

    /* if (ptr->host.len == 0 || ptr->port == 0) { */
    if (ptr->host.len == 0 || ptr->port.len == 0) {
        /* ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "mod_memcache init fail: ptr->host.len=%d, ptr->port=%d\n", ptr->host.len, ptr->port); */
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "mod_memcache init fail: ptr->host.len=%d, ptr->port.len=%d\n", ptr->host.len, ptr->port.len);
        return NGX_CONF_ERROR;
    }
    /**
     * init memcache
     * 
     */
    memcached_server_st *servers = NULL;
    memcached_return rc;

    ptr->memc = memcached_create(NULL);
    servers = memcached_server_list_append(servers, ptr->host.data, ngx_atoi(ptr->port.data, ptr->port.len), &rc);   
    rc= memcached_server_push(ptr->memc, servers);

    if (rc == MEMCACHED_SUCCESS) {
	fprintf(stderr,"Added server successfully\n");   
	return NGX_CONF_OK;
    }
    else{
	fprintf(stderr,"Couldn't add server: %s\n",memcached_strerror(ptr->memc, rc));
	return NGX_CONF_OK;
    }
    return NGX_CONF_OK;
}

static ngx_http_module_t ngx_http_mod_memcache_module_ctx = {
    NULL,                                   /* preconfiguration */
    NULL,                                   /* postconfiguration */

    ngx_http_mod_memcache_create_main_conf, /* create main configuration */
    ngx_http_mod_memcache_init_main_conf,   /* init main configuration */

    NULL,                                   /* create server configuration */
    NULL,                                   /* merge server configuration */

    NULL,                                   /* create location configuration */
    NULL                                    /* merge location configuration */

};


static ngx_command_t ngx_http_mod_memcache_commands[] = {
    { 
	ngx_string("mm_host"),
        NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
	ngx_conf_set_str_slot,
        NGX_HTTP_MAIN_CONF_OFFSET,
        offsetof(ngx_http_mod_memcache_conf_t, host),
        NULL
    },
    { 
	ngx_string("mm_port"),
        NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
	ngx_conf_set_str_slot,
        NGX_HTTP_MAIN_CONF_OFFSET,
        offsetof(ngx_http_mod_memcache_conf_t, port),
        NULL
    },
    { 
	ngx_string("mm_cache"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
	ngx_http_mod_memcache_usecache,
        0,
        0,
        NULL
    },

    ngx_null_command
};

ngx_module_t  ngx_http_mod_memcache_module = {
    NGX_MODULE_V1,
    &ngx_http_mod_memcache_module_ctx,      /* module context */
    ngx_http_mod_memcache_commands,         /* module directives */
    NGX_HTTP_MODULE,                        /* module type */
    NULL,                                   /* init master */
    NULL,                                   /* init module */
    NULL,                                   /* init process */
    NULL,                                   /* init thread */
    NULL,                                   /* exit thread */
    NULL,                                   /* exit process */
    NULL,                                   /* exit master */
    NGX_MODULE_V1_PADDING
};


static char *_getword(ngx_pool_t *pool, ngx_str_t *s, ngx_uint_t *n, u_char split)
{
    ngx_uint_t x = *n;
    char *r;
    if (x >= s->len) {
        return NULL;
    }
    while (x < s->len) {
        if (split == s->data[x]) {
            break;
        }
        x++;
    }
    r = ngx_pcalloc(pool, (x-*n) + 1);
    ngx_memcpy(r, s->data + *n, x - *n);
    r[x-*n] = '\0';
    *n = x+1;
    return r;
}


static ngx_int_t ngx_http_mod_memcache_handler(ngx_http_request_t *r){
    printf("IN HANDLER\n");
    printf("url:%s[%d]\n", r->args.data, r->args.len);
    printf("method:%d\n", r->method);
    ngx_int_t    rc;
    ngx_buf_t   *b;
    ngx_chain_t  out;
    u_char *standard_key = ngx_palloc(r->pool, strlen("mk"));
    ngx_memzero(standard_key, strlen("mk"));
    ngx_memcpy(standard_key, "mk", strlen("mk"));
    u_char *standard_error = ngx_palloc(r->pool, strlen("-1"));
    ngx_memzero(standard_error, strlen("-1"));
    ngx_memcpy(standard_error, "-1", strlen("-1"));

    /**
     * fuck, 这么庞大的nginx类库,竟然没有处理query string的东西.让小生好纠结阿....
     * 抄qyb<qiuyingbo@sohu-inc.com>大大的一个_getword用用吧.
     */

    /* ngx_http_variable_value_t      *vv; */
    /* ngx_http_variable_t        *v; */
    /* ngx_http_core_main_conf_t  *cmcf; */

    /* we response to 'GET' and 'HEAD' requests only */
    if (!(r->method & (NGX_HTTP_GET))) {
        return NGX_HTTP_NOT_ALLOWED;
    }
    char *rargs = ngx_pcalloc(r->pool, r->args.len + 1);
    char *key, *seg;
    ngx_memzero(rargs, r->args.len + 1);
    ngx_memcpy(rargs, r->args.data, r->args.len);
    ngx_str_t s, t;
    s.data = (u_char *)rargs;
    s.len = strlen(rargs);
    ngx_uint_t n = 0, m = 0;
    u_char *val;
    // stop word = &|=
    if(r->args.len > 0){
	seg = _getword(r->pool, &s, &n, '&');
	while(seg){
	    // 获取到&,先输出下
	    const u_char *p = s.data + n;
	    t.data = (u_char *)seg;
	    t.len = strlen(seg);
	    m = 0;
	    key = _getword(r->pool, &t, &m, '=');
	    if(0 == ngx_strncasecmp(key, standard_key, strlen(standard_key))){
		printf("they equal [%s], [%s]", standard_key, key);
		val = t.data + m;
		break;
	    } else {
		key = "";
		val = "";
	    }
	    n += strlen(seg) + 1;
	    seg = _getword(r->pool, &s, &n, '&');
	}
    }
    if(key && val){
	ngx_http_mod_memcache_conf_t *conf;
	size_t val_len = 0;
	uint32_t flag;
        uint32_t *value;
	memcached_return rc;

	// 丢进memcache中去.
	conf = ngx_http_get_module_main_conf(r, ngx_http_mod_memcache_module);
	value = memcached_get(conf->memc, val, strlen(val), &val_len, &flag, &rc);
	if(rc == MEMCACHED_SUCCESS){
	    printf("get success:, %d %s\n", val_len, val);
	    /* discard request body, since we don't need it here */
	    rc = ngx_http_discard_request_body(r);
 
	    if (rc != NGX_OK) {
		return rc;
	    }
 
	    /* set the 'Content-type' header */
	    r->headers_out.content_type_len = sizeof("text/html") - 1;
	    r->headers_out.content_type.len = sizeof("text/html") - 1;
	    r->headers_out.content_type.data = (u_char *) "text/html";
	    /* allocate a buffer for your response body */
	    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
	    if (b == NULL) {
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	    }
 
	    /* attach this buffer to the buffer chain */
	    out.buf = b;
	    out.next = NULL;
	    /* adjust the pointers of the buffer */
	    b->pos = value;
	    b->last = value + strlen(value) - 1;
	    b->memory = 1;    /* this buffer is in memory */
	    b->last_buf = 1;  /* this is the last buffer in the buffer chain */
	    /* set the status line */
	    r->headers_out.status = NGX_HTTP_OK;
	    r->headers_out.content_length_n = strlen(value) + 1;
	    /* send the headers of your response */
	    rc = ngx_http_send_header(r);
	    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
		printf("Unwritten\n");
		return rc;
	    }
	    /* send the buffer chain of your response */
	    printf("Lets End With[%s][%d]\n", value, strlen(value));
	    return ngx_http_output_filter(r, &out);
	} else {
	    return NGX_HTTP_NOT_FOUND;
	}
    } else {
	return NGX_HTTP_NOT_FOUND;
    }
}
