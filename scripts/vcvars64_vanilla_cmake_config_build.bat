@REM put this file in visual studio 202x cmdline tools directories, such as : C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build

@call "%~dp0vcvarsall.bat" x64
cd ../build/%1
cmake ../.. --preset %1
ninja -j 8 && ninja install