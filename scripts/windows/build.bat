cd /d %~dp0
cd ..\..

cd build
IF %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

"C:\Program Files (x86)\CMake\bin\cmake.exe" --build . --config %1
