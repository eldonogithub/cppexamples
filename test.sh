#!/bin/bash

( perl -e 'print <<HTTP;
GET /test HTTP/1.1\r
Host: 127.0.0.1:4444\r
User-Agent: 1\r
Accept: */*\r
Content-Length: 0\r
\r
GET /test HTTP/1.1\r
Host: 127.0.0.1:4444\r
User-Agent: 2\r
Accept: */*\r
Content-Length: 0\r
\r
GET /test HTTP/1.1\r
Host: 127.0.0.1:4444\r
User-Agent: 3\r
Accept: */*\r
Content-Length: 0\r
\r
GET /test HTTP/1.1\r
Host: 127.0.0.1:4444\r
User-Agent: 4\r
Accept: */*\r
Content-Length: 0\r
\r
GET /test HTTP/1.1\r
Host: 127.0.0.1:4444\r
User-Agent: 5\r
Accept: */*\r
Content-Length: 0\r
\r
HTTP
') | nc 127.0.0.1 4444
