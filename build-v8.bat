rem Run this batch file in a Visual Studio command prompt. After the batch file has been executed, build v8pp with:
rem msbuild /p:Configuration=Release /p:Platform=x64 v8pp.sln

git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
set PATH=%PATH%;%CD%\depot_tools
fetch --no-history v8
cd v8
gclient sync
python build\gyp_v8 -Dtarget_arch=x64 --depth=. -I../v8_options.gypi tools/gyp/v8.gyp
msbuild /m /p:Configuration=Release /p:Platform=x64 tools\gyp\v8.sln
mkdir lib\x64\Release
copy build\Release\lib\*.lib lib\x64\Release
copy build\Release\lib\*.exp lib\x64\Release
copy build\Release\v8_lib*.lib lib\x64\Release
copy build\Release\*.dll lib\x64\Release
copy build\Release\*.pdb lib\x64\Release

cd ..
