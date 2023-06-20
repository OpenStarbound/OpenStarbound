set QT_PREFIX_PATH="C:\Qt\5.7\msvc2015_64"
set CMAKE_PREFIX_PATH="C:\Program Files\CMake"
set PATH=%PATH%;%CMAKE_PREFIX_PATH%\bin;%QT_PREFIX_PATH%\bin

mkdir build
cd build || exit /b 1

del /f CMakeCache.txt

cmake.exe ^
  -G"Visual Studio 14 Win64" ^
  -T"v140" ^
  -DCMAKE_PREFIX_PATH=%QT_PREFIX_PATH% ^
  -DSTAR_USE_JEMALLOC=OFF ^
  -DSTAR_ENABLE_STEAM_INTEGRATION=ON ^
  -DSTAR_ENABLE_DISCORD_INTEGRATION=ON ^
  -DSTAR_BUILD_QT_TOOLS=ON ^
  -DCMAKE_INCLUDE_PATH="..\lib\windows64\include" ^
  -DCMAKE_LIBRARY_PATH="..\lib\windows64" ^
  ..\source || exit /b 1

cmake.exe --build . --config RelWithDebInfo || exit /b 1

cd ..

move dist windows64_binaries || exit /b 1

windeployqt.exe windows64_binaries\mod_uploader.exe || exit /b 1

copy lib\windows64\*.dll windows64_binaries\ || exit /b 1
