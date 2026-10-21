@echo off
setlocal
python "%~dp0..\..\..\..\Server\Tool\PacketGenerator\PacketGenerator.py" --target client %*
exit /b %errorlevel%
