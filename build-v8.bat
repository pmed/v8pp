rem Run this batch file in a Visual Studio command prompt. After the batch file has been executed, build v8pp with:
rem msbuild /p:Configuration=Release /p:Platform=x64 v8pp.sln

rem Fetch V8 and dependencies
python fetch-v8.py
pushd v8

rem Generate project files
python tools\gyp\gyp_main.py -fmsvs -Dtarget_arch=x64 --depth=. -I./gypfiles/standalone.gypi -I../v8_options.gypi src\v8.gyp

rem Build

:Release
msbuild /m /p:Configuration=Release /p:Platform=x64 src\v8.sln
mkdir lib\x64\Release
copy build\Release\lib\*.lib lib\x64\Release
copy build\Release\lib\*.exp lib\x64\Release
copy build\Release\v8_lib*.lib lib\x64\Release
copy build\Release\*.dll lib\x64\Release
copy build\Release\*.pdb lib\x64\Release

goto done

:Debug
msbuild /m /p:Configuration=Debug /p:Platform=x64 src\v8.sln
mkdir lib\x64\Debug
copy build\Debug\lib\*.lib lib\x64\Debug
copy build\Debug\lib\*.exp lib\x64\Debug
copy build\Debug\v8_lib*.lib lib\x64\Debug
copy build\Debug\*.dll lib\x64\Debug
copy build\Debug\*.pdb lib\x64\Debug

:done
popd