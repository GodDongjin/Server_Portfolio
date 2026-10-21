@echo off
setlocal
python "%~dp0..\..\..\Tool\PacketGenerator\PacketGenerator.py" --target server %*
exit /b %errorlevel%
