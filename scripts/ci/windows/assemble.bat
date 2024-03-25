@echo off
set client=client_distribution
if exist %client% rmdir %client% /S /Q

mkdir %client%
mkdir %client%\storage
mkdir %client%\mods
mkdir %client%\logs
mkdir %client%\assets
mkdir %client%\win

set server=server_distribution
if exist %server% rmdir %server% /S /Q
xcopy %client% %server% /E /I

.\dist\asset_packer.exe -c scripts\packing.config assets\opensb %client%\assets\opensb.pak

for /f "delims=" %%f in (scripts\ci\windows\files_client.txt) do (
    xcopy "%%f" "%client%\win\" /Y
)

.\dist\asset_packer.exe -c scripts\packing.config -s assets\opensb %server%\assets\opensb.pak

for /f "delims=" %%f in (scripts\ci\windows\files_server.txt) do (
    xcopy "%%f" "%server%\win\" /Y
)

set win=windows
xcopy %client% %win% /E /I /Y
xcopy %server% %win% /E /I /Y