@echo off
setlocal
powershell -ExecutionPolicy Bypass -File "%~dp0detener_proyecto.ps1" %*
endlocal
