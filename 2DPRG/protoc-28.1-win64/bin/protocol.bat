pushd %~dp0

protoc.exe --csharp_out=./ Enum.proto
protoc.exe --csharp_out=./ Struct.proto
protoc.exe --csharp_out=./ protocol.proto

IF ERRORLEVEL 1 PAUSE

XCOPY /Y Enum.cs "../../Assets/Scripts/Protocol"
XCOPY /Y Struct.cs "../../Assets/Scripts/Protocol"
XCOPY /Y Protocol.cs "../../Assets/Scripts/Protocol"

DEL /Q /F *.cs

PAUSE