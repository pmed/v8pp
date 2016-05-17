#!/usr/bin/env sh

git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=`pwd`/depot_tools:"$PATH"
fetch --no-history v8
cd v8
gclient sync

GYP_DIR=build
if [ ! -f "$GYP_DIR"/gyp_v8 ] ; then
  GYP_DIR=gypfiles
fi

GYP_GENERATORS=make "$GYP_DIR"/gyp_v8 --generator-output=out --depth=. -I"$GYP_DIR"/standalone.gypi -I../v8_options.gypi "$GYP_DIR"/all.gyp
make v8 v8_libplatform -C out BUILDTYPE=Release -j8 builddir=$(pwd)/out/Release
mkdir -p lib
cp out/Release/lib.target/*.so lib
cp out/Release/lib* lib
cd ..

