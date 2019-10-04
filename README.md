# Template to bootstrap C++ research projects

[![Build Status](https://patrick-website.visualstudio.com/cpp-template/_apis/build/status/XiangpengHao.cpp-template?branchName=master)](https://patrick-website.visualstudio.com/cpp-template/_build/latest?definitionId=2&branchName=master)

> this language (C++) has many dark corner, stupid conventions, implicit conversion and not mention UB
> -- [Wojciech Muła](http://0x80.pl/notesen/2015-05-25-tricky-mistake.html)

Context: my research focus is high performance computing

You might also like: https://www.rust-lang.org. My intuition is unless your daily routine is to perf code line-by-line, calcuate each cache use, measure every memory hit; you should consider rust-lang.

## Features

- GTest + GLog

- Modern CMake, keep compile time in mind 

- C++ 17

- Sanitizers enabled in Debug mode, `march=native` in Release mode.

- Docker enabled

- CI included

- Clang-format: Google style

## Build

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE={Debug|Release} ..
make -j
```
