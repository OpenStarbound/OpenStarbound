cd windows32_binaries

set PATH="%PATH%;..\lib\windows32"

copy ..\scripts\windows\sbinit.config .

.\core_tests || exit /b 1
.\game_tests || exit /b 1
