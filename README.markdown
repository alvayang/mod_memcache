This module is a nginx module that demo how make nginx dance with memcached

## Install
   drop the debug.sh to the base dir of nginx, then run debug.sh
   the directory tree is like this:
   ├── nginx-1.0.5 \n
   │   └── debug.sh \n
   └── mod_memcache  \n

## nginx.conf
   worker_processes  1;
   daemon off; 
   master_process  off;
   error_log  /tmp/error.log debug;
   pid /tmp/nginx_demo.pid;
   events {
	   worker_connections  1024;
   }
   http {
   	include       mime.types;
   	sendfile        on;
   	keepalive_timeout  65;
   	tcp_nodelay        on;
   	mm_host  127.0.0.1;
   	mm_port  11211;
   	server {
   		listen   80;
   		access_log  /tmp/access.log;
   		error_log  /tmp/error.log debug;
   		location /get {
   			mm_cache on;
   		}
   	}
   }
   
## Simple Usage:
   objs/nginx -c /path/to/your/config
   curl http://127.0.0.1/get/mk=key

## TODO:
   Set Key
   Multi Key
   Delete Key
   AutoIncr
   Connection Pool