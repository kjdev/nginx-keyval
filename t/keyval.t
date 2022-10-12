use Test::Nginx::Socket 'no_plan';

no_root_location();
no_shuffle();

run_tests();

__DATA__

=== get
--- http_config
keyval_zone zone=test:1M;
keyval $cookie_data_key $keyval_data zone=test;
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
keyval_zone zone=test:1M;
keyval $cookie_data_key $keyval_data zone=test;
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
keyval_zone zone=test:1M;
keyval $cookie_data_key $keyval_data zone=test;
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
keyval_zone zone=test:1M;
keyval $cookie_data_key $keyval_data zone=test;
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
keyval_zone zone=test:1M;
keyval $cookie_data_key $keyval_data zone=test;
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
keyval_zone zone=test:1M;
keyval $cookie_data_key $keyval_data1 zone=test;
keyval $cookie_data_key $keyval_data2 zone=test;
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
keyval_zone zone=test1:1M;
keyval_zone zone=test2:1M;
keyval $cookie_data_key $keyval_data1 zone=test1;
keyval $cookie_data_key $keyval_data2 zone=test2;
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
