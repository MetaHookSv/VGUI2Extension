@echo off
setlocal
set "Configuration=Debug"
call "%~dp0build-VGUI2Extension-x86.bat" %*
exit /b %errorlevel%
