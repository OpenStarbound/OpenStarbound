@echo off

for %%f in (dist\*.pdb) do (
    echo %%f | find "starbound" > nul || del %%f
)