@echo off
if not exist build mkdir build
pushd build
cl /nologo /Zi /FS /Fd:win32_handmade.pdb /Fe:win32_handmade.exe ..\code\win32_handmade.cpp user32.lib
 
popd