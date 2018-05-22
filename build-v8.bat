rem Run this batch file in a Visual Studio command prompt. After the batch file has been executed, build v8pp with:
rem msbuild /p:Configuration=Release /p:Platform=x64 v8pp.sln

set DEPOT_TOOLS_WIN_TOOLCHAIN=0

git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
set PATH=%PATH%;%CD%\depot_tools
fetch --no-history v8
cd v8
gclient sync

set GYP_DIR=build
set V8GYP_DIR=tools\gyp
if not exist %GYP% (
  set GYP_DIR=gypfiles
  set V8GYP_DIR=src
)
python %GYP_DIR%\gyp_v8 -Dtarget_arch=x64 --depth=. -I../v8_options.gypi %V8GYP_DIR%\v8.gyp

:Release
msbuild /m /p:Configuration=Release /p:Platform=x64 %V8GYP_DIR%\v8.sln
mkdir lib\x64\Release
copy build\Release\lib\*.lib lib\x64\Release
copy build\Release\lib\*.exp lib\x64\Release
copy build\Release\v8_lib*.lib lib\x64\Release
copy build\Release\*.dll lib\x64\Release
copy build\Release\*.pdb lib\x64\Release
goto done

:Debug
msbuild /m /p:Configuration=Debug /p:Platform=x64 %V8GYP_DIR%\v8.sln
mkdir lib\x64\Debug
copy build\Debug\lib\*.lib lib\x64\Debug
copy build\Debug\lib\*.exp lib\x64\Debug
copy build\Debug\v8_lib*.lib lib\x64\Debug
copy build\Debug\*.dll lib\x64\Debug
copy build\Debug\*.pdb lib\x64\Debug

:done
cd ..
