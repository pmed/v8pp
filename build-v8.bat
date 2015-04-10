rem Run this batch file in a Visual Studio command prompt. After the batch file has been executed, build v8pp with:
rem msbuild /p:Configuration=Release /p:Platform=Win32 v8pp.sln

git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
set PATH=%PATH%;%CD%\depot_tools
fetch --no-history v8
cd v8
gclient sync
python build\gyp_v8 -Dtarget_arch=ia32 -Dcomponent=shared_library
msbuild /p:Configuration=Release /p:Platform=Win32 tools\gyp\v8.sln
mkdir lib\x86\Release
copy build\Release\lib\v8.* lib\x86\Release
copy build\Release\v8.* lib\x86\Release
cd ..
