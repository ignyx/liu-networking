# Simple HTTP proxy

Proxies HTTP requests and conditionnally modifies contents.

## Features

- multiple concurrent connections (multi-threaded)
- persistent connections (ingress only)
- first real program written in C/C++

## Limitations

- assumes all HTTP headers are received in a single TCP packet
- always uses port 80 for client connections
- Does not remove the protocol, host and port from request path
  (`http://tawkie.fr/en/faq` is not changed to `/en/faq`)
- does not handle chunked replies

## Building from source

Listens on port `3490` by default. You can change this before compilation by
changing the `PORT` constant in `server.cc`.

You can also change the log verbosity by updating the `log_level` constant.

```
g++ -o proxy -Wall -Wextra -Wpedantic server.cc
./proxy
```

## Usage

```
curl 'http://zebroid.ida.liu.se/fakenews/test1.txt' -v -x localhost:3490
```

You can configure your browser to use the proxy :

```
Type : HTTP proxy (manual)
Host : localhost
Port : 3490 (by default)
```
