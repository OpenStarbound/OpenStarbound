pushd %~dp0
pushd ..\..\dist
map_grep "invalid=true" ..\assets\packed\dungeons\
map_grep "invalid=true" ..\assets\devel\dungeons\
pause
popd
popd
