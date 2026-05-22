pushd %~dp0

protoc.exe -I=./ --cpp_out=./ ./Enum.proto
protoc.exe -I=./ --cpp_out=./ ./Struct.proto
protoc.exe -I=./ --cpp_out=./ ./Protocol.proto

GenPackets.exe --path=./Protocol.proto --output=ClientPacketHandler --recv=C_ --send=S_

IF ERRORLEVEL 1 PAUSE

XCOPY /Y Enum.pb.h "../../../TestClient/Protocol"
XCOPY /Y Enum.pb.cc "../../../TestClient/Protocol"
XCOPY /Y Struct.pb.h "../../../TestClient/Protocol"
XCOPY /Y Struct.pb.cc "../../../TestClient/Protocol"
XCOPY /Y Protocol.pb.h "../../../TestClient/Protocol"
XCOPY /Y Protocol.pb.cc "../../../TestClient/Protocol"
XCOPY /Y ClientPacketHandler.h "../../../TestClient/Packet"

DEL /Q /F *.pb.h
DEL /Q /F *.pb.cc
DEL /Q /F *.h

PAUSE