make clean
export CC="gcc -g -I/usr/local/include "
./configure --add-module=../ngx_memcache/ --with-debug --without-http_rewrite_module --prefix=/tmp/
#CPPFLAGS="-I/usr/local/include" ./configure --add-module=../hello/ --with-debug --without-http_rewrite_module --prefix=/tmp/
make
