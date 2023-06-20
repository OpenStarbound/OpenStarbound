cd /d %~dp0
cd ..\..

mkdir dist
del dist\*.dll
copy lib\windows32\*.dll dist\
copy scripts\windows\sbinit.config dist\

mkdir build
cd build

if exist "C:\Program Files (x86)\CMake\bin" (
  set CMAKE_EXE_PATH="C:\Program Files (x86)\CMake\bin"
) else (
  set CMAKE_EXE_PATH="C:\Program Files\CMake\bin"
)

set QT_PREFIX_PATH=C:\Qt\5.6\msvc2015

if exist %QT_PREFIX_PATH% (

%CMAKE_EXE_PATH%\cmake.exe ^
  ..\source ^
  -G"Visual Studio 14" ^
  -T"v140_xp" ^
  -DSTAR_USE_JEMALLOC=OFF ^
  -DCMAKE_PREFIX_PATH=%QT_PREFIX_PATH% ^
  -DSTAR_BUILD_QT_TOOLS=ON ^
  -DCMAKE_INCLUDE_PATH="..\lib\windows32\include" ^
  -DCMAKE_LIBRARY_PATH="..\lib\windows32"

) else (

%CMAKE_EXE_PATH%\cmake.exe ^
  ..\source ^
  -G "Visual Studio 14" ^
  -T"v140_xp" ^
  -DSTAR_USE_JEMALLOC=OFF ^
  -DCMAKE_INCLUDE_PATH="..\lib\windows32\include" ^
  -DCMAKE_LIBRARY_PATH="..\lib\windows32"

)

pause
