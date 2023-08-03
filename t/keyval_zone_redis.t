use Test::Nginx::Socket 'no_plan';

no_root_location();
no_shuffle();

run_tests();

__DATA__

=== get
--- init
system "redis-cli flushall"
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=get ttl=30s;
keyval $cookie_data_key $keyval_data zone=get;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set {
  set $keyval_data "test";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request
GET /get
--- more_headers
Cookie: data_key=key
--- response_body
/get(key): 

=== set
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=set ttl=30s;
keyval $cookie_data_key $keyval_data zone=set;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set {
  set $keyval_data "test";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request
GET /set
--- more_headers
Cookie: data_key=key
--- response_body
/set(key): test

=== get and set
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=test3 ttl=30s;
keyval $cookie_data_key $keyval_data zone=test3;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set {
  set $keyval_data "test";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): \n",
  "/set(key): test\n",
  "/get(key): test\n"
]

=== multiple set
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=multiple_set ttl=30s;
keyval $cookie_data_key $keyval_data zone=multiple_set;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set1 {
  set $keyval_data "test";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set2 {
  set $keyval_data "foo";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request eval
[
  "GET /get",
  "GET /set1",
  "GET /get",
  "GET /set2",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): \n",
  "/set1(key): test\n",
  "/get(key): test\n",
  "/set2(key): foo\n",
  "/get(key): foo\n"
]

=== multiple key
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=multiple_key ttl=30s;
keyval $cookie_data_key $keyval_data zone=multiple_key;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set {
  set $keyval_data "test";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request eval
[
  "GET /set",
  "GET /get",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=foo"
]
--- response_body eval
[
  "/set(key): test\n",
  "/get(key): test\n",
  "/get(foo): \n"
]

=== multiple variable
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=multiple_variable ttl=30s;
keyval $cookie_data_key $keyval_data1 zone=multiple_variable;
keyval $cookie_data_key $keyval_data2 zone=multiple_variable;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): 1:$keyval_data1 2:$keyval_data2\n";
}
location = /set1 {
  set $keyval_data1 "test";
  return 200 "$request_uri($cookie_data_key): 1:$keyval_data1 2:$keyval_data2\n";
}
location = /set2 {
  set $keyval_data2 "foo";
  return 200 "$request_uri($cookie_data_key): 1:$keyval_data1 2:$keyval_data2\n";
}
--- request eval
[
  "GET /get",
  "GET /set1",
  "GET /get",
  "GET /set2",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): 1: 2:\n",
  "/set1(key): 1:test 2:test\n",
  "/get(key): 1:test 2:test\n",
  "/set2(key): 1:foo 2:foo\n",
  "/get(key): 1:foo 2:foo\n"
]

=== multiple zone
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=multiple_zone1 ttl=30s;
keyval_zone_redis zone=multiple_zone2 ttl=30s;
keyval $cookie_data_key $keyval_data1 zone=multiple_zone1;
keyval $cookie_data_key $keyval_data2 zone=multiple_zone2;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): 1:$keyval_data1 2:$keyval_data2\n";
}
location = /set1 {
  set $keyval_data1 "test1";
  return 200 "$request_uri($cookie_data_key): 1:$keyval_data1 2:$keyval_data2\n";
}
location = /set2 {
  set $keyval_data2 "test2";
  return 200 "$request_uri($cookie_data_key): 1:$keyval_data1 2:$keyval_data2\n";
}
--- request eval
[
  "GET /get",
  "GET /set1",
  "GET /get",
  "GET /set2",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): 1: 2:\n",
  "/set1(key): 1:test1 2:\n",
  "/get(key): 1:test1 2:\n",
  "/set2(key): 1:test1 2:test2\n",
  "/get(key): 1:test1 2:test2\n"
]

=== keep set
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=keep ttl=300s;
keyval $cookie_data_key $keyval_data zone=keep;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set {
  set $keyval_data "test";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): \n",
  "/set(key): test\n",
  "/get(key): test\n",
]

=== keep get
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=keep ttl=300s;
keyval $cookie_data_key $keyval_data zone=keep;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request eval
[
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): test\n"
]

=== hostname (default)
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=hostname ttl=300s;
keyval $cookie_data_key $keyval_data zone=hostname;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set {
  set $keyval_data "test";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): \n",
  "/set(key): test\n",
  "/get(key): test\n"
]

=== hostname (127.0.0.1)
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=hostname hostname=127.0.0.1 ttl=300s;
keyval $cookie_data_key $keyval_data zone=hostname;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set {
  set $keyval_data "test1";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): test\n",
  "/set(key): test1\n",
  "/get(key): test1\n"
]

=== hostname (192.168.0.1)
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- timeout: 5s
--- http_config
keyval_zone_redis zone=hostname hostname=192.168.0.1 ttl=300s;
keyval $cookie_data_key $keyval_data zone=hostname;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set {
  set $keyval_data "test2";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): \n",
  "/set(key): \n",
  "/get(key): \n"
]

=== port (default)
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=port ttl=300s;
keyval $cookie_data_key $keyval_data zone=port;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set {
  set $keyval_data "test";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): \n",
  "/set(key): test\n",
  "/get(key): test\n"
]

=== port (6379)
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=port port=6379 ttl=300s;
keyval $cookie_data_key $keyval_data zone=port;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set {
  set $keyval_data "test1";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): test\n",
  "/set(key): test1\n",
  "/get(key): test1\n"
]

=== port (6380)
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=port port=6380 ttl=300s;
keyval $cookie_data_key $keyval_data zone=port;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set {
  set $keyval_data "test2";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): \n",
  "/set(key): \n",
  "/get(key): \n"
]

=== database (default)
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=database ttl=300s;
keyval $cookie_data_key $keyval_data zone=database;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set {
  set $keyval_data "test";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): \n",
  "/set(key): test\n",
  "/get(key): test\n"
]

=== database (0)
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=database database=0 ttl=300s;
keyval $cookie_data_key $keyval_data zone=database;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set {
  set $keyval_data "test1";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): test\n",
  "/set(key): test1\n",
  "/get(key): test1\n"
]

=== database (1)
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=database database=1 ttl=300s;
keyval $cookie_data_key $keyval_data zone=database;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set {
  set $keyval_data "test2";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): \n",
  "/set(key): test2\n",
  "/get(key): test2\n"
]

=== timeout (1s)
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=timeout ttl=1s;
keyval $cookie_data_key $keyval_data zone=timeout;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set {
  set $keyval_data "test1s";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): \n",
  "/set(key): test1s\n",
  "/get(key): test1s\n"
]

=== timeout (1m)
--- init
system "sleep 1"
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=timeout ttl=1m;
keyval $cookie_data_key $keyval_data zone=timeout;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set {
  set $keyval_data "test1m";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): \n",
  "/set(key): test1m\n",
  "/get(key): test1m\n"
]

=== timeout (1h)
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=timeout ttl=1h;
keyval $cookie_data_key $keyval_data zone=timeout;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set {
  set $keyval_data "test1h";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): test1m\n",
  "/set(key): test1h\n",
  "/get(key): test1h\n"
]

=== timeout (1d)
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=timeout ttl=1d;
keyval $cookie_data_key $keyval_data zone=timeout;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set {
  set $keyval_data "test1d";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): test1h\n",
  "/set(key): test1d\n",
  "/get(key): test1d\n"
]

=== timeout (1w)
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=timeout ttl=1w;
keyval $cookie_data_key $keyval_data zone=timeout;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set {
  set $keyval_data "test1w";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): test1d\n",
  "/set(key): test1w\n",
  "/get(key): test1w\n"
]

=== timeout (1M)
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=timeout ttl=1M;
keyval $cookie_data_key $keyval_data zone=timeout;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set {
  set $keyval_data "test1M";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): test1w\n",
  "/set(key): test1M\n",
  "/get(key): test1M\n"
]

=== timeout (1y)
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=timeout ttl=1y;
keyval $cookie_data_key $keyval_data zone=timeout;
--- config
location = /get {
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
location = /set {
  set $keyval_data "test1y";
  return 200 "$request_uri($cookie_data_key): $keyval_data\n";
}
--- request eval
[
  "GET /get",
  "GET /set",
  "GET /get"
]
--- more_headers eval
[
  "Cookie: data_key=key",
  "Cookie: data_key=key",
  "Cookie: data_key=key"
]
--- response_body eval
[
  "/get(key): test1M\n",
  "/set(key): test1y\n",
  "/get(key): test1y\n"
]

=== conf not found keyval_zone_redis
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval $cookie_data_key $keyval_data zone=invalid;
--- config
--- must_die

=== conf not found zone parameter
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=invalid ttl=30s;
keyval $cookie_data_key $keyval_data;
--- config
--- must_die

=== conf invalid zone parameter
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=invalid ttl=30s;
keyval $cookie_data_key $keyval_data zone=test;
--- config
--- must_die

=== conf same for keyval_zone and keyval_zone_redis
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone zone=invalid;
keyval_zone_redis zone=invalid ttl=30s;
keyval $cookie_data_key $keyval_data zone=invalid;
--- config
--- must_die

=== conf same for keyval_zone_redis and keyval_zone
--- skip_eval
1: (!defined $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_HTTP_KEYVAL_ZONE_REDIS"} eq '') and (!defined $ENV{"NGX_KEYVAL_ZONE_REDIS"} or $ENV{"NGX_KEYVAL_ZONE_REDIS"} eq '')
--- http_config
keyval_zone_redis zone=invalid ttl=30s;
keyval_zone zone=invalid;
keyval $cookie_data_key $keyval_data zone=invalid;
--- config
--- must_die
