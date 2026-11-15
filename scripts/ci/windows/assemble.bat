@echo off
set client=client_distribution
if exist %client% rmdir %client% /S /Q

mkdir %client%
mkdir %client%\storage
mkdir %client%\mods
mkdir %client%\logs
mkdir %client%\assets
mkdir %client%\win
echo 211820 > %client%\win\steam_appid.txt

set server=server_distribution
if exist %server% rmdir %server% /S /Q
xcopy %client% %server% /E /I

rem the OpenStarbound page of the credits lists the contributors of the repository (leaves the committed list alone if GitHub can't be reached)
bash scripts/credits/generate-contributors.sh

.\dist\asset_packer.exe -c scripts\packing.config assets\opensb %client%\assets\opensb.pak
bash scripts/pack-mods.sh ./dist/asset_packer.exe %client%/mods

for /f "delims=" %%f in (scripts\ci\windows\files_client.txt) do (
    xcopy "%%f" "%client%\win\" /Y
)

.\dist\asset_packer.exe -c scripts\packing.config -s assets\opensb %server%\assets\opensb.pak
bash scripts/pack-mods.sh ./dist/asset_packer.exe %server%/mods -s

for /f "delims=" %%f in (scripts\ci\windows\files_server.txt) do (
    xcopy "%%f" "%server%\win\" /Y
)

set win=windows
if exist %win% rmdir %win% /S /Q
xcopy %client% %win% /E /I /Y
xcopy %server% %win% /E /I /Y

7z -bb3 -mtp=2 -mm=Deflate64 -mx=9 a client.zip -r %client%
7z -bb3 -mtp=2 -mm=Deflate64 -mx=9 a server.zip -r %server%
