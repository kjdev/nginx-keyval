use Test::Nginx::Socket 'no_plan';

no_root_location();
no_shuffle();

run_tests();

__DATA__

=== get
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- init
system "redis-cli flushall"
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=30s;
  keyval "stream_get" $keyval_host zone=stream_redis;
  upstream test {
    server 127.0.0.1:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
--- request
GET /get
--- response_body_like
^.*$
--- error_code: 502

=== set
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- init
system "redis-cli flushall"
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=30s;
  keyval "stream_set" $keyval_host zone=stream_redis;
  upstream test {
    server 127.0.0.1:8181;
  }
  server {
    listen 8081;
    proxy_pass test;
    set $keyval_host "test";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /set {
  proxy_pass http://127.0.0.1:8081;
}
--- request
GET /set
--- response_body
/set: 127.0.0.1:8181

=== get and set
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- init
system "redis-cli flushall"
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=30s;
  keyval "stream_get_and_set" $keyval_host zone=stream_redis;
  upstream test {
    server 127.0.0.1:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test;
    set $keyval_host "test";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
location = /set {
  proxy_pass http://127.0.0.1:8081;
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get",
]
--- response_body_like eval
[
  "^.*\$",
  "^/set: 127\.0\.0\.1:8181\$",
  "^/get: 127\.0\.0\.1:8181\$",
]
--- error_code eval
[
  502,
  200,
  200,
]

=== multiple set
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- init
system "redis-cli flushall"
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=30s;
  keyval "stream_multiple_set" $keyval_host zone=stream_redis;
  upstream test1 {
    server 127.0.0.1:8181;
  }
  upstream test2 {
    server 127.0.0.2:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test1;
    set $keyval_host "test1";
  }
  server {
    listen 8082;
    proxy_pass test2;
    set $keyval_host "test2";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
location = /set1 {
  proxy_pass http://127.0.0.1:8081;
}
location = /set2 {
  proxy_pass http://127.0.0.1:8082;
}
--- request eval
[
  "GET /get",
  "GET /set1",
  "GET /get",
  "GET /set2",
  "GET /get",
]
--- response_body_like eval
[
  "^.*\$",
  "^/set1: 127\.0\.0\.1:8181\$",
  "^/get: 127\.0\.0\.1:8181\$",
  "^/set2: 127\.0\.0\.2:8181\$",
  "^/get: 127\.0\.0\.2:8181\$",
]
--- error_code eval
[
  502,
  200,
  200,
  200,
  200,
]

=== multiple key
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- init
system "redis-cli flushall"
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=30s;
  keyval "stream_multiple_key" $keyval_key zone=stream_redis;
  keyval $keyval_key $keyval_host zone=stream_redis;
  upstream test1 {
    server 127.0.0.1:8181;
  }
  upstream test2 {
    server 127.0.0.2:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test1;
    set $keyval_key "key1";
    set $keyval_host "test1";
  }
  server {
    listen 8082;
    proxy_pass test2;
    set $keyval_key "key2";
    set $keyval_host "test2";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
location = /set1 {
  proxy_pass http://127.0.0.1:8081;
}
location = /set2 {
  proxy_pass http://127.0.0.1:8082;
}
--- request eval
[
  "GET /set1",
  "GET /get",
  "GET /set2",
  "GET /get",
]
--- response_body eval
[
  "/set1: 127.0.0.1:8181\n",
  "/get: 127.0.0.1:8181\n",
  "/set2: 127.0.0.2:8181\n",
  "/get: 127.0.0.2:8181\n",
]

=== multiple variable
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- init
system "redis-cli flushall"
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=30s;
  keyval "stream_multiple_variable" $keyval_host zone=stream_redis;
  keyval "stream_multiple_variable" $keyval_data zone=stream_redis;
  upstream test1 {
    server 127.0.0.1:8181;
  }
  upstream test2 {
    server 127.0.0.2:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test1;
    set $keyval_host "test1";
  }
  server {
    listen 8082;
    proxy_pass test2;
    set $keyval_data "test2";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
location = /set1 {
  proxy_pass http://127.0.0.1:8081;
}
location = /set2 {
  proxy_pass http://127.0.0.1:8082;
}
--- request eval
[
  "GET /get",
  "GET /set1",
  "GET /get",
  "GET /set2",
  "GET /get",
]
--- response_body_like eval
[
  "^.*\$",
  "^/set1: 127\.0\.0\.1:8181\$",
  "^/get: 127\.0\.0\.1:8181\$",
  "^/set2: 127\.0\.0\.2:8181\$",
  "^/get: 127\.0\.0\.2:8181\$",
]
--- error_code eval
[
  502,
  200,
  200,
  200,
  200,
]

=== multiple zone
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- init
system "redis-cli flushall"
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis1 ttl=30s;
  keyval_zone_redis zone=stream_redis2 ttl=30s;
  keyval "stream_multiple_zone" $keyval_host zone=stream_redis1;
  keyval "stream_multiple_zone" $keyval_data zone=stream_redis2;
  upstream test1 {
    server 127.0.0.1:8181;
  }
  upstream test2 {
    server 127.0.0.2:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test1;
    set $keyval_host "test1";
  }
  server {
    listen 8082;
    proxy_pass test2;
    set $keyval_data "test2";
  }
  server {
    listen 8083;
    proxy_pass $keyval_data;
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get1 {
  proxy_pass http://127.0.0.1:8080;
}
location = /set1 {
  proxy_pass http://127.0.0.1:8081;
}
location = /set2 {
  proxy_pass http://127.0.0.1:8082;
}
location = /get2 {
  proxy_pass http://127.0.0.1:8083;
}
--- request eval
[
  "GET /set1",
  "GET /get1",
  "GET /set2",
  "GET /get2",
  "GET /get1",
]
--- response_body eval
[
  "/set1: 127.0.0.1:8181\n",
  "/get1: 127.0.0.1:8181\n",
  "/set2: 127.0.0.2:8181\n",
  "/get2: 127.0.0.2:8181\n",
  "/get1: 127.0.0.1:8181\n",
]

=== keep set
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- init
system "redis-cli flushall"
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=300s;
  keyval "stream_keep" $keyval_host zone=stream_redis;
  upstream test {
    server 127.0.0.1:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test;
    set $keyval_host "test";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
location = /set {
  proxy_pass http://127.0.0.1:8081;
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get",
]
--- response_body_like eval
[
  "^.*\$",
  "^/set: 127\.0\.0\.1:8181\$",
  "^/get: 127\.0\.0\.1:8181\$",
]
--- error_code eval
[
  502,
  200,
  200,
]

=== keep get
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=300s;
  keyval "stream_keep" $keyval_host zone=stream_redis;
  upstream test {
    server 127.0.0.1:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test;
    set $keyval_host "test";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
--- request eval
[
  "GET /get",
]
--- response_body eval
[
  "/get: 127.0.0.1:8181\n",
]

=== hostname (default)
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- init
system "redis-cli flushall"
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=30s;
  keyval "stream_hostname" $keyval_host zone=stream_redis;
  upstream test1 {
    server 127.0.0.1:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test1;
    set $keyval_host "test1";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
location = /set {
  proxy_pass http://127.0.0.1:8081;
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get",
]
--- response_body_like eval
[
  "^.*\$",
  "^/set: 127\.0\.0\.1:8181\$",
  "^/get: 127\.0\.0\.1:8181\$",
]
--- error_code eval
[
  502,
  200,
  200,
]

=== hostname (127.0.0.1)
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis hostname=127.0.0.1 ttl=30s;
  keyval "stream_hostname" $keyval_host zone=stream_redis;
  upstream test1 {
    server 127.0.0.1:8181;
  }
  upstream test2 {
    server 127.0.0.2:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test1;
    set $keyval_host "test1";
  }
  server {
    listen 8082;
    proxy_pass test2;
    set $keyval_host "test2";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
location = /set {
  proxy_pass http://127.0.0.1:8082;
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get",
]
--- response_body eval
[
  "/get: 127.0.0.1:8181\n",
  "/set: 127.0.0.2:8181\n",
  "/get: 127.0.0.2:8181\n",
]

=== hostname (192.168.0.1)
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis hostname=192.168.0.1 ttl=30s;
  keyval "stream_hostname" $keyval_host zone=stream_redis;
  upstream test1 {
    server 127.0.0.1:8181;
  }
  upstream test2 {
    server 127.0.0.2:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test1;
    set $keyval_host "test1";
  }
  server {
    listen 8082;
    proxy_pass test2;
    set $keyval_host "test2";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
--- request
GET /get
--- response_body_like
^.*$
--- error_code: 502
--- timeout: 5s

=== port (default)
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- init
system "redis-cli flushall"
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=300s;
  keyval "stream_port" $keyval_host zone=stream_redis;
  upstream test1 {
    server 127.0.0.1:8181;
  }
  upstream test2 {
    server 127.0.0.2:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test1;
    set $keyval_host "test1";
  }
  server {
    listen 8082;
    proxy_pass test2;
    set $keyval_host "test2";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
location = /set {
  proxy_pass http://127.0.0.1:8081;
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get",
]
--- response_body_like eval
[
  "^.*\$",
  "^/set: 127\.0\.0\.1:8181\$",
  "^/get: 127\.0\.0\.1:8181\$",
]
--- error_code eval
[
  502,
  200,
  200,
]

=== port (6379)
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis port=6379 ttl=300s;
  keyval "stream_port" $keyval_host zone=stream_redis;
  upstream test1 {
    server 127.0.0.1:8181;
  }
  upstream test2 {
    server 127.0.0.2:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test1;
    set $keyval_host "test1";
  }
  server {
    listen 8082;
    proxy_pass test2;
    set $keyval_host "test2";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
location = /set {
  proxy_pass http://127.0.0.1:8082;
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get",
]
--- response_body eval
[
  "/get: 127.0.0.1:8181\n",
  "/set: 127.0.0.2:8181\n",
  "/get: 127.0.0.2:8181\n",
]

=== port (6380)
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis port=6380 ttl=300s;
  keyval "stream_port" $keyval_host zone=stream_redis;
  upstream test1 {
    server 127.0.0.1:8181;
  }
  upstream test2 {
    server 127.0.0.2:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test1;
    set $keyval_host "test1";
  }
  server {
    listen 8082;
    proxy_pass test2;
    set $keyval_host "test2";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
location = /set {
  proxy_pass http://127.0.0.1:8082;
}
--- request
GET /get
--- response_body_like
^.*$
--- error_code: 502
--- timeout: 5s

=== database (default)
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- init
system "redis-cli flushall"
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=300s;
  keyval "stream_database" $keyval_host zone=stream_redis;
  upstream test1 {
    server 127.0.0.1:8181;
  }
  upstream test2 {
    server 127.0.0.2:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test1;
    set $keyval_host "test1";
  }
  server {
    listen 8082;
    proxy_pass test2;
    set $keyval_host "test2";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
location = /set {
  proxy_pass http://127.0.0.1:8081;
}
--- request eval
[
  "GET /set",
  "GET /get",
]
--- response_body eval
[
  "/set: 127.0.0.1:8181\n",
  "/get: 127.0.0.1:8181\n",
]

=== database (0)
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis database=0 ttl=300s;
  keyval "stream_database" $keyval_host zone=stream_redis;
  upstream test1 {
    server 127.0.0.1:8181;
  }
  upstream test2 {
    server 127.0.0.2:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test1;
    set $keyval_host "test1";
  }
  server {
    listen 8082;
    proxy_pass test2;
    set $keyval_host "test2";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
location = /set {
  proxy_pass http://127.0.0.1:8082;
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get",
]
--- response_body eval
[
  "/get: 127.0.0.1:8181\n",
  "/set: 127.0.0.2:8181\n",
  "/get: 127.0.0.2:8181\n",
]

=== database (1)
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis database=1 ttl=300s;
  keyval "stream_database" $keyval_host zone=stream_redis;
  upstream test1 {
    server 127.0.0.1:8181;
  }
  upstream test2 {
    server 127.0.0.2:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test1;
    set $keyval_host "test1";
  }
  server {
    listen 8082;
    proxy_pass test2;
    set $keyval_host "test2";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
location = /set {
  proxy_pass http://127.0.0.1:8081;
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get",
]
--- response_body_like eval
[
  "^.*\$",
  "^/set: 127\.0\.0\.1:8181\$",
  "^/get: 127\.0\.0\.1:8181\$",
]
--- error_code eval
[
  502,
  200,
  200,
]

=== timeout (1s)
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- init
system "redis-cli flushall"
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=1s;
  keyval "stream_timeout" $keyval_host zone=stream_redis;
  upstream test1 {
    server 127.0.0.1:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test1;
    set $keyval_host "test1";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
location = /set {
  proxy_pass http://127.0.0.1:8081;
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get",
]
--- response_body_like eval
[
  "^.*\$",
  "^/set: 127\.0\.0\.1:8181\$",
  "^/get: 127\.0\.0\.1:8181\$",
]
--- error_code eval
[
  502,
  200,
  200,
]

=== timeout (1m)
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- init
system "sleep 1"
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=1m;
  keyval "stream_timeout" $keyval_host zone=stream_redis;
  upstream test1 {
    server 127.0.0.1:8181;
  }
  upstream test2 {
    server 127.0.0.2:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test2;
    set $keyval_host "test2";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
location = /set {
  proxy_pass http://127.0.0.2:8081;
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get",
]
--- response_body_like eval
[
  "^.*\$",
  "^/set: 127\.0\.0\.2:8181\$",
  "^/get: 127\.0\.0\.2:8181\$",
]
--- error_code eval
[
  502,
  200,
  200,
]

=== timeout (1h)
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=1h;
  keyval "stream_timeout" $keyval_host zone=stream_redis;
  upstream test2 {
    server 127.0.0.2:8181;
  }
  upstream test3 {
    server 127.0.0.3:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test3;
    set $keyval_host "test3";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
location = /set {
  proxy_pass http://127.0.0.3:8081;
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get",
]
--- response_body_like eval
[
  "^/get: 127\.0\.0\.2:8181\$",
  "^/set: 127\.0\.0\.3:8181\$",
  "^/get: 127\.0\.0\.3:8181\$",
]
--- error_code eval
[
  200,
  200,
  200,
]

=== timeout (1d)
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=1d;
  keyval "stream_timeout" $keyval_host zone=stream_redis;
  upstream test3 {
    server 127.0.0.3:8181;
  }
  upstream test4 {
    server 127.0.0.4:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test4;
    set $keyval_host "test4";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
location = /set {
  proxy_pass http://127.0.0.4:8081;
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get",
]
--- response_body_like eval
[
  "^/get: 127\.0\.0\.3:8181\$",
  "^/set: 127\.0\.0\.4:8181\$",
  "^/get: 127\.0\.0\.4:8181\$",
]
--- error_code eval
[
  200,
  200,
  200,
]

=== timeout (1w)
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=1w;
  keyval "stream_timeout" $keyval_host zone=stream_redis;
  upstream test4 {
    server 127.0.0.4:8181;
  }
  upstream test5 {
    server 127.0.0.5:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test5;
    set $keyval_host "test5";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
location = /set {
  proxy_pass http://127.0.0.5:8081;
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get",
]
--- response_body_like eval
[
  "^/get: 127\.0\.0\.4:8181\$",
  "^/set: 127\.0\.0\.5:8181\$",
  "^/get: 127\.0\.0\.5:8181\$",
]
--- error_code eval
[
  200,
  200,
  200,
]

=== timeout (1M)
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=1M;
  keyval "stream_timeout" $keyval_host zone=stream_redis;
  upstream test5 {
    server 127.0.0.5:8181;
  }
  upstream test6 {
    server 127.0.0.6:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test6;
    set $keyval_host "test6";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
location = /set {
  proxy_pass http://127.0.0.6:8081;
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get",
]
--- response_body_like eval
[
  "^/get: 127\.0\.0\.5:8181\$",
  "^/set: 127\.0\.0\.6:8181\$",
  "^/get: 127\.0\.0\.6:8181\$",
]
--- error_code eval
[
  200,
  200,
  200,
]

=== timeout (1y)
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=1y;
  keyval "stream_timeout" $keyval_host zone=stream_redis;
  upstream test6 {
    server 127.0.0.6:8181;
  }
  upstream test7 {
    server 127.0.0.7:8181;
  }
  server {
    listen 8080;
    proxy_pass $keyval_host;
  }
  server {
    listen 8081;
    proxy_pass test7;
    set $keyval_host "test7";
  }
}
--- http_config
server {
  listen 8181;
  location / {
    return 200 "$request_uri: $server_addr:$server_port\n";
  }
}
--- config
location = /get {
  proxy_pass http://127.0.0.1:8080;
}
location = /set {
  proxy_pass http://127.0.0.7:8081;
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get",
]
--- response_body_like eval
[
  "^/get: 127\.0\.0\.6:8181\$",
  "^/set: 127\.0\.0\.7:8181\$",
  "^/get: 127\.0\.0\.7:8181\$",
]
--- error_code eval
[
  200,
  200,
  200,
]

=== conf not found keyval_zone_redis
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- post_main_config
stream {
  keyval "stream_conf_not_found" $keyval_host zone=stream_redis;
}
--- config
--- must_die

=== conf not found zone parameter
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=30s;
  keyval "stream_conf_not_found_zone_parameter" $keyval_host;
}
--- config
--- must_die

=== conf invalid zone parameter
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=30s;
  keyval "stream_conf_invalid_zone_parameter" $keyval_host zone=invalid;
}
--- config
--- must_die

=== conf same for keyval_zone and keyval_zone_redis
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- post_main_config
stream {
  keyval_zone zone=stream_redis;
  keyval_zone_redis zone=stream_redis ttl=30s;
  keyval "stream_conf_same_for_zone_redis" $keyval_host zone=stream_redis;
}
--- config
--- must_die

=== conf same for keyval_zone_redis and keyval_zone
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=30s;
  keyval_zone zone=stream_redis;
  keyval "stream_conf_same_for_redis_zone" $keyval_host zone=stream_redis;
}
--- config
--- must_die

=== conf duplicate for keyval_zone_redis
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=30s;
  keyval_zone_redis zone=stream_redis ttl=30s;
  keyval "stream_conf_duplicate" $keyval_host zone=stream_redis;
}
--- config
--- must_die

=== conf same keyval_zone_redis for stream{} and http{}
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/ or ((!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq ''))
--- post_main_config
stream {
  keyval_zone_redis zone=stream_redis ttl=30s;
}
--- http_config
keyval_zone_redis zone=stream_redis ttl=30s;
--- config
--- must_die
