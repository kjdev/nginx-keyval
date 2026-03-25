use Test::Nginx::Socket 'no_plan';

no_root_location();
no_shuffle();

run_tests();

__DATA__

=== Same host but different UA
--- http_config
keyval_zone zone=test:1M;
keyval $remote_addr:$http_user_agent $keyval_data zone=test;
--- config
location = /get {
  return 200 "get($http_user_agent): $keyval_data\n";
}
location = /set {
  set $keyval_data "test $http_user_agent";
  return 200 "$keyval_data\n";
}
--- request eval
[
    "GET /get",
    "GET /get",
    "GET /set",
    "GET /set",
    "GET /get",
    "GET /get"
]
--- more_headers eval
[
    "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0",
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36",
    "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0",
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36",
    "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0",
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36"
]
--- response_body eval
[
    "get(Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0): \n",
    "get(Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36): \n",
    "test Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0\n",
    "test Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36\n",
    "get(Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0): test Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0\n",
    "get(Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36): test Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36\n"
]

=== Two vars with different UAs
--- http_config
keyval_zone zone=test:5M;
keyval "$remote_addr:$http_user_agent" $keyval_data1 zone=test;
keyval "$remote_addr:$http_user_agent" $keyval_data2 zone=test;
--- config
location = /get {
  return 200 "get($http_user_agent): $keyval_data1 $keyval_data2\n";
}
location = /set1 {
  set $keyval_data1 "test1 $http_user_agent";
  return 200 "$keyval_data1\n";
}
location = /set2 {
  set $keyval_data2 "test2 $http_user_agent";
  return 200 "$keyval_data2\n";
}
--- request eval
[
    "GET /get",
    "GET /get",
    "GET /set1",
    "GET /set1",
    "GET /set2",
    "GET /set2",
    "GET /get",
    "GET /get"
]
--- more_headers eval
[
    "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0",
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36",
    "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0",
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36",
    "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0",
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36",
    "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0",
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36"
]
--- response_body eval
[
    "get(Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0):  \n",
    "get(Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36):  \n",
    "test1 Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0\n",
    "test1 Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36\n",
    "test2 Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0\n",
    "test2 Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36\n",
    "get(Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0): test2 Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0 test2 Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0\n",
    "get(Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36): test2 Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36 test2 Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36\n"
]

=== Same host but different UA and other chars in string
--- http_config
keyval_zone zone=test:1M;
keyval "$remote_addr $http_user_agent random string random number 123456 test" $keyval_data zone=test;
--- config
location = /get {
  return 200 "get($http_user_agent): $keyval_data\n";
}
location = /set {
  set $keyval_data "test $http_user_agent";
  return 200 "$keyval_data\n";
}
--- request eval
[
    "GET /get",
    "GET /get",
    "GET /set",
    "GET /set",
    "GET /get",
    "GET /get"
]
--- more_headers eval
[
    "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0",
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36",
    "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0",
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36",
    "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0",
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36"
]
--- response_body eval
[
    "get(Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0): \n",
    "get(Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36): \n",
    "test Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0\n",
    "test Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36\n",
    "get(Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0): test Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0\n",
    "get(Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36): test Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36\n"
]

=== Variables that exist and one that doesn't
--- http_config
keyval_zone zone=test:1M;
keyval "$remote_addr $http_user_agent random string random number 123456 $hosst $request" $keyval_data zone=test;
--- config
--- must_die

=== Pure text is still functional
--- http_config
keyval_zone zone=test:1M;
keyval "text1 text2" $keyval_data zone=test;
--- config
location = /get {
  return 200 "get($http_user_agent): $keyval_data\n";
}
location = /set {
  set $keyval_data "test $http_user_agent";
  return 200 "$keyval_data\n";
}
--- request eval
[
    "GET /get",
    "GET /get",
    "GET /set",
    "GET /set",
    "GET /get",
    "GET /get"
]
--- more_headers eval
[
    "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0",
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36",
    "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0",
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36",
    "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0",
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36"
]
--- response_body eval
[
    "get(Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0): \n",
    "get(Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36): \n",
    "test Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0\n",
    "test Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36\n",
    "get(Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0): test Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36\n",
    "get(Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36): test Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36\n"
]

=== many variable keys
--- http_config
keyval_zone zone=test:1M;
keyval $remote_addr:$http_user_agent:$cookie_key1:$cookie_key2:$cookie_key3:$cookie_key4:$cookie_key5:$cookie_key6:$cookie_key7:$cookie_key8:$cookie_key9:$cookie_key10:$cookie_key11:$cookie_key12:$cookie_key13:$cookie_key14:$cookie_key15 $keyval_data zone=test;
--- config
location = /get {
  return 200 "get($http_user_agent): $keyval_data\n";
}
location = /set {
  set $keyval_data "test $http_user_agent:$cookie_key1:$cookie_key2:$cookie_key3:$cookie_key4:$cookie_key5:$cookie_key6:$cookie_key7:$cookie_key8:$cookie_key9:$cookie_key10:$cookie_key11:$cookie_key12:$cookie_key13:$cookie_key14:$cookie_key15";
  return 200 "$keyval_data\n";
}
--- request eval
[
    "GET /get",
    "GET /get",
    "GET /set",
    "GET /set",
    "GET /get",
    "GET /get"
]
--- more_headers eval
[
    "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0\nCookie: key1=key1\nCookie: key2=key2\nCookie: key3=key3\nCookie: key4=key4\nCookie: key5=key5\nCookie: key6=key6\nCookie: key7=key7\nCookie: key8=key8\nCookie: key9=key9\nCookie: key10=key10\nCookie: key11=key11\nCookie: key12=key12\nCookie: key13=key13\nCookie: key14=key14\nCookie: key15=key15",
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36\nCookie: key1=key1\nCookie: key2=key2\nCookie: key3=key3\nCookie: key4=key4\nCookie: key5=key5\nCookie: key6=key6\nCookie: key7=key7\nCookie: key8=key8\nCookie: key9=key9\nCookie: key10=key10\nCookie: key11=key11\nCookie: key12=key12\nCookie: key13=key13\nCookie: key14=key14\nCookie: key15=key15",
    "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0\nCookie: key1=key1\nCookie: key2=key2\nCookie: key3=key3\nCookie: key4=key4\nCookie: key5=key5\nCookie: key6=key6\nCookie: key7=key7\nCookie: key8=key8\nCookie: key9=key9\nCookie: key10=key10\nCookie: key11=key11\nCookie: key12=key12\nCookie: key13=key13\nCookie: key14=key14\nCookie: key15=key15",
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36\nCookie: key1=key1\nCookie: key2=key2\nCookie: key3=key3\nCookie: key4=key4\nCookie: key5=key5\nCookie: key6=key6\nCookie: key7=key7\nCookie: key8=key8\nCookie: key9=key9\nCookie: key10=key10\nCookie: key11=key11\nCookie: key12=key12\nCookie: key13=key13\nCookie: key14=key14\nCookie: key15=key15",
    "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0\nCookie: key1=key1\nCookie: key2=key2\nCookie: key3=key3\nCookie: key4=key4\nCookie: key5=key5\nCookie: key6=key6\nCookie: key7=key7\nCookie: key8=key8\nCookie: key9=key9\nCookie: key10=key10\nCookie: key11=key11\nCookie: key12=key12\nCookie: key13=key13\nCookie: key14=key14\nCookie: key15=key15",
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36\nCookie: key1=key1\nCookie: key2=key2\nCookie: key3=key3\nCookie: key4=key4\nCookie: key5=key5\nCookie: key6=key6\nCookie: key7=key7\nCookie: key8=key8\nCookie: key9=key9\nCookie: key10=key10\nCookie: key11=key11\nCookie: key12=key12\nCookie: key13=key13\nCookie: key14=key14\nCookie: key15=key15"
]
--- response_body eval
[
    "get(Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0): \n",
    "get(Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36): \n",
    "test Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0:key1:key2:key3:key4:key5:key6:key7:key8:key9:key10:key11:key12:key13:key14:key15\n",
    "test Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36:key1:key2:key3:key4:key5:key6:key7:key8:key9:key10:key11:key12:key13:key14:key15\n",
    "get(Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0): test Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0:key1:key2:key3:key4:key5:key6:key7:key8:key9:key10:key11:key12:key13:key14:key15\n",
    "get(Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36): test Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36:key1:key2:key3:key4:key5:key6:key7:key8:key9:key10:key11:key12:key13:key14:key15\n"
]
