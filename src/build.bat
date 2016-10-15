@echo off

IF NOT DEFINED clset (call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64)
SET clset=64

set CompilerFlags=-O2 -MTd -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -Z7 -WX -W4 -wd4127 -wd4201 -wd4100 -wd4189 -wd4505
set LinkerFlags=-incremental:no -opt:ref gdi32.lib user32.lib winmm.lib opengl32.lib

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

cl %CompilerFlags% ../src/win32_pong.cpp /link %LinkerFlags%