#!/usr/bin/env sh

git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=`pwd`/depot_tools:"$PATH"
fetch --no-history v8
cd v8
gclient sync
make native library=shared -j8
mkdir -p lib
cp out/native/lib.target/*.so lib
cd ..

