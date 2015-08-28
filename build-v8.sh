#!/usr/bin/env sh

git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=`pwd`/depot_tools:"$PATH"
fetch --no-history v8
cd v8
gclient sync
export CXXFLAGS="-Wno-unknown-warning-option"
export GYPFLAGS="-Dv8_use_external_startup_data=0"
make native library=shared werror=no -j8
mkdir -p lib
cp out/native/lib.target/*.so lib
cp out/native/obj.target/tools/gyp/lib*.a lib
cd ..

