# torrest-cpp

[![Build Status](https://github.com/i96751414/torrest-cpp/actions/workflows/build.yml/badge.svg)](https://github.com/i96751414/torrest-cpp/actions/workflows/build.yml)
[![cross-build](https://github.com/i96751414/torrest-cpp/actions/workflows/cross-compilers.yml/badge.svg)](https://github.com/i96751414/torrest-cpp/actions/workflows/cross-compilers.yml)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/f16ca71d4f034660ac593fafce2479b7)](https://www.codacy.com/gh/i96751414/torrest-cpp/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=i96751414/torrest-cpp&amp;utm_campaign=Badge_Grade)
[![CodeQL](https://github.com/i96751414/torrest-cpp/actions/workflows/codeql.yml/badge.svg)](https://github.com/i96751414/torrest-cpp/actions/workflows/codeql.yml)

Torrent service with a REST api, specially made for streaming. This is the C++ implementation of the
[Torrest (golang)](https://github.com/i96751414/torrest) project.

## Running Torrest

One can run it like any other binary, that is:

```shell
./torrest
```

Although Torrest doesn't have any required arguments, it accepts optional arguments:

| Argument       | Type   | Default                                       | Description            |
|----------------|--------|-----------------------------------------------|------------------------|
| -p, --port     | uint16 | 8080                                          | The server listen port |
| -s, --settings | string | settings.json                                 | The settings path      |
| --log-level    | string | INFO                                          | The global log level   |
| --log-pattern  | string | `%Y-%m-%d %H:%M:%S.%e %l [%n] [thread-%t] %v` | The log pattern        |
| --log-path     | string | n/a                                           | The log path           |
| -v, --version  | n/a    | n/a                                           | Print version          |
| -h, --help     | n/a    | n/a                                           | Print help message     |
