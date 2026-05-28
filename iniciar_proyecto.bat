@echo off
setlocal
powershell -ExecutionPolicy Bypass -File "%~dp0iniciar_proyecto.ps1" %*
endlocal
