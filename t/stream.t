use Test::Nginx::Socket 'no_plan';

no_root_location();
no_shuffle();

run_tests();

__DATA__

=== get
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/
--- post_main_config
stream {
  keyval_zone zone=stream:1M;
  keyval "stream_get" $keyval_host zone=stream;
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
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/
--- post_main_config
stream {
  keyval_zone zone=stream:1M;
  keyval "stream_set" $keyval_host zone=stream;
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
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/
--- post_main_config
stream {
  keyval_zone zone=stream:1M;
  keyval "stream_set_and_get" $keyval_host zone=stream;
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
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/
--- post_main_config
stream {
  keyval_zone zone=stream:1M;
  keyval "stream_multiple_set" $keyval_host zone=stream;
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
  proxy_pass http://127.0.0.2:8082;
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
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/
--- post_main_config
stream {
  keyval_zone zone=stream:1M;
  keyval "stream_multiple_key" $keyval_key zone=stream;
  keyval $keyval_key $keyval_host zone=stream;
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
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/
--- post_main_config
stream {
  keyval_zone zone=stream:1M;
  keyval "stream_multiple_variable" $keyval_host zone=stream;
  keyval "stream_multiple_variable" $keyval_data zone=stream;
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
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/
--- post_main_config
stream {
  keyval_zone zone=stream1:1M;
  keyval_zone zone=stream2:1M;
  keyval "stream_multiple_zone" $keyval_host zone=stream1;
  keyval "stream_multiple_zone" $keyval_data zone=stream2;
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

=== conf not found keyval_zone
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/
--- post_main_config
stream {
  keyval "stream_conf_not_found" $keyval_host zone=stream;
}
--- config
--- must_die

=== conf not found zone parameter
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/
--- post_main_config
stream {
  keyval_zone zone=stream:1M;
  keyval "stream_conf_not_found_zone_parameter" $keyval_host;
}
--- config
--- must_die

=== conf invalid zone parameter
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/
--- post_main_config
stream {
  keyval_zone zone=stream:1M;
  keyval "stream_conf_invalid_zone_parameter" $keyval_host zone=invalid;
}
--- config
--- must_die

=== conf same keyval_zone for stream{} and http{}
--- skip_eval
1: $ENV{"TEST_NGINX_LOAD_MODULES"} !~ /ngx_stream_keyval_module/
--- post_main_config
stream {
  keyval_zone zone=stream:1M;
}
--- http_config
keyval_zone zone=stream:1M;
--- config
--- must_die
