@echo off
setlocal
set "Configuration=Release"
call "%~dp0build-VGUI2Extension-x86.bat" %*
exit /b %errorlevel%
