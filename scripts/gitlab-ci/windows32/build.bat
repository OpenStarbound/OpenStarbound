set CMAKE_PREFIX_PATH="C:\Program Files\CMake"
set PATH=%PATH%;%CMAKE_PREFIX_PATH%\bin;%QT_PREFIX_PATH%\bin

mkdir build
cd build || exit /b 1

del /f CMakeCache.txt

cmake.exe ^
  -G"Visual Studio 14" ^
  -T"v140_xp" ^
  -DSTAR_ENABLE_STATIC_MSVC_RUNTIME=ON ^
  -DSTAR_ENABLE_STEAM_INTEGRATION=ON ^
  -DSTAR_ENABLE_DISCORD_INTEGRATION=ON ^
  -DCMAKE_INCLUDE_PATH="..\lib\windows32\include" ^
  -DCMAKE_LIBRARY_PATH="..\lib\windows32" ^
  ..\source || exit /b 1

cmake.exe --build . --config RelWithDebInfo || exit /b 1

cd ..

move dist windows32_binaries || exit /b 1

copy lib\windows32\*.dll windows32_binaries\ || exit /b 1
