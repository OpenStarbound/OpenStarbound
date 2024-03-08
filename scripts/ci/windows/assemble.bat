@echo off
set dst=windows

if exist %dst% rmdir %dst% /S /Q

mkdir %dst%
mkdir %dst%\storage
mkdir %dst%\mods
mkdir %dst%\assets

set bin=%dst%\win
mkdir %bin%

.\dist\asset_packer.exe -c scripts\packing.config assets\opensb %dst%\assets\opensb.pak

for /f "delims=" %%f in (scripts\ci\windows\files.txt) do (
    xcopy "%%f" "%bin%\" /Y
)