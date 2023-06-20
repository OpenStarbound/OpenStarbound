cd /d %~dp0
cd ..\..\dist

set STEAM_STARBOUND_DIR=c:\Program Files (x86)\Steam\steamapps\common\Starbound - Unstable

.\asset_packer.exe -c ..\assets\packing.config "custom assets" ..\assets\packed .\packed.pak
move packed.pak "%STEAM_STARBOUND_DIR%\assets\packed.pak"
copy starbound.exe "%STEAM_STARBOUND_DIR%\win32\"
