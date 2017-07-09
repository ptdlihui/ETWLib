@echo off
set "sourceDir=%~dp0"
set "currentDir=%cd%"
cd "%sourceDir%"
git submodule init
git submodule update --init --recursive
cd "%currentDir%"
pause
