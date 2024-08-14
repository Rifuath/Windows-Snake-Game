@echo off

rem ==================================================
rem Build Script for windows
rem ------------------------
rem Using 'cl' which is microsoft compiler to compile.
rem args:
rem     -Zi for debug symbols
rem     -FC for fullpath
rem Libs:
rem     - user32 and gdi32 is needed for windows
rem       specific compilation library which is used
rem       by the the program.
rem
rem
rem ==================================================


mkdir .\build
pushd .\build

rem Debug / favours speed
 cl -Zi -FC ..\src\snake.cpp user32.lib gdi32.lib
rem Release / favours speed
rem cl -O2 -FC ..\src\snake.cpp user32.lib gdi32.lib
popd

