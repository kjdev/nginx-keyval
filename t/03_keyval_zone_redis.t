use strict;
use warnings;

BEGIN {
    if ((!defined $ENV{NGX_HTTP_KEYVAL_ZONE_REDIS} or $ENV{NGX_HTTP_KEYVAL_ZONE_REDIS} eq '') and (!defined $ENV{NGX_KEYVAL_ZONE_REDIS} or $ENV{NGX_KEYVAL_ZONE_REDIS} eq '')) {
        require Test::More;
        Test::More::plan(skip_all => "redis is not supported");
        exit 0;
    }
}

use Test::Nginx::Socket 'no_plan';

no_root_location();
no_shuffle();

run_tests();

__DATA__

=== get
--- init
system "redis-cli flushall"
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

=== timeout (1h)
--- init
system "sleep 2"
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
  "/get(key): \n",
  "/set(key): test1h\n",
  "/get(key): test1h\n"
]

=== conf not found keyval_zone_redis
--- http_config
keyval $cookie_data_key $keyval_data zone=invalid;
--- config
--- must_die

=== conf not found zone parameter
--- http_config
keyval_zone_redis zone=invalid ttl=30s;
keyval $cookie_data_key $keyval_data;
--- config
--- must_die

=== conf invalid zone parameter
--- http_config
keyval_zone_redis zone=invalid ttl=30s;
keyval $cookie_data_key $keyval_data zone=test;
--- config
--- must_die

=== conf same for keyval_zone and keyval_zone_redis
--- http_config
keyval_zone zone=invalid;
keyval_zone_redis zone=invalid ttl=30s;
keyval $cookie_data_key $keyval_data zone=invalid;
--- config
--- must_die

=== conf same for keyval_zone_redis and keyval_zone
--- http_config
keyval_zone_redis zone=invalid ttl=30s;
keyval_zone zone=invalid;
keyval $cookie_data_key $keyval_data zone=invalid;
--- config
--- must_die

=== conf redis invalid ttl
--- http_config
keyval_zone_redis zone=test ttl=abc;
keyval $cookie_data_key $keyval_data zone=test;
--- config
--- must_die

=== conf redis unknown parameter
--- http_config
keyval_zone_redis zone=test foo=bar;
keyval $cookie_data_key $keyval_data zone=test;
--- config
--- must_die

=== conf duplicate keyval_zone_redis
--- http_config
keyval_zone_redis zone=test ttl=1s;
keyval_zone_redis zone=test ttl=1s;
keyval $cookie_data_key $keyval_data zone=test;
--- config
--- must_die
