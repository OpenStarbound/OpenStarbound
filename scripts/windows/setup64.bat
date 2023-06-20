cd /d %~dp0
cd ..\..

mkdir dist
del dist\*.dll
copy lib\windows64\*.dll dist\
copy scripts\windows\sbinit.config dist\

mkdir build
cd build

del /f CMakeCache.txt

if exist "C:\Program Files (x86)\CMake\bin" (
  set CMAKE_EXE_PATH="C:\Program Files (x86)\CMake\bin"
) else (
  set CMAKE_EXE_PATH="C:\Program Files\CMake\bin"
)

set QT_PREFIX_PATH=C:\Qt\5.6\msvc2015_64

if exist %QT_PREFIX_PATH% (

%CMAKE_EXE_PATH%\cmake.exe ^
  -G"Visual Studio 17 2022" ^
  -T"v143" ^
  -DCMAKE_PREFIX_PATH=%QT_PREFIX_PATH% ^
  -DSTAR_USE_JEMALLOC=OFF ^
  -DSTAR_ENABLE_STEAM_INTEGRATION=ON ^
  -DSTAR_ENABLE_DISCORD_INTEGRATION=ON ^
  -DSTAR_BUILD_QT_TOOLS=ON ^
  -DCMAKE_INCLUDE_PATH="..\lib\windows64\include" ^
  -DCMAKE_LIBRARY_PATH="..\lib\windows64" ^
  ..\source

) else (

%CMAKE_EXE_PATH%\cmake.exe ^
  -G"Visual Studio 17 2022" ^
  -T"v143" ^
  -DSTAR_USE_JEMALLOC=OFF ^
  -DSTAR_ENABLE_STEAM_INTEGRATION=ON ^
  -DSTAR_ENABLE_DISCORD_INTEGRATION=ON ^
  -DCMAKE_INCLUDE_PATH="..\lib\windows64\include" ^
  -DCMAKE_LIBRARY_PATH="..\lib\windows64" ^
  ..\source

)

pause
