cd /d %~dp0
cd ..\..

mkdir dist
del dist\*.dll
copy lib\windows64\*.dll dist\
copy scripts\windows\sbinit.config dist\

mkdir build
cd build

if exist "C:\Program Files (x86)\CMake\bin" (
  set CMAKE_EXE_PATH="C:\Program Files (x86)\CMake\bin"
) else (
  set CMAKE_EXE_PATH="C:\Program Files\CMake\bin"
)

set QT_PREFIX_PATH=C:\Qt\5.6\msvc2015_64

if exist %QT_PREFIX_PATH% (

%CMAKE_EXE_PATH%\cmake.exe ^
  ..\source ^
  -G"Visual Studio 15 Win64" ^
  -DSTAR_USE_JEMALLOC=ON ^
  -DCMAKE_PREFIX_PATH=%QT_PREFIX_PATH% ^
  -DSTAR_BUILD_QT_TOOLS=ON ^
  -DCMAKE_INCLUDE_PATH="..\lib\windows64\include" ^
  -DCMAKE_LIBRARY_PATH="..\lib\windows64"

) else (

%CMAKE_EXE_PATH%\cmake.exe ^
  ..\source ^
  -G "Visual Studio 15 Win64" ^
  -DSTAR_USE_JEMALLOC=ON ^
  -DCMAKE_INCLUDE_PATH="..\lib\windows64\include" ^
  -DCMAKE_LIBRARY_PATH="..\lib\windows64"

)

pause
