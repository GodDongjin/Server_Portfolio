pushd %~dp0

protoc.exe -I=./ --cpp_out=./ ./Enum.proto
protoc.exe -I=./ --cpp_out=./ ./Struct.proto
protoc.exe -I=./ --cpp_out=./ ./Protocol.proto

GenPackets.exe --path=./Protocol.proto --output=ServerPacketHandler --recv=REQ --send=ACK

IF ERRORLEVEL 1 PAUSE

XCOPY /Y Enum.pb.h "../../../GameServer/Protobuf"
XCOPY /Y Enum.pb.cc "../../../GameServer/Protobuf"
XCOPY /Y Struct.pb.h "../../../GameServer/Protobuf"
XCOPY /Y Struct.pb.cc "../../../GameServer/Protobuf"
XCOPY /Y Protocol.pb.h "../../../GameServer/Protobuf"
XCOPY /Y Protocol.pb.cc "../../../GameServer/Protobuf"
XCOPY /Y ServerPacketHandler.h "../../../GameServer/Packet"

DEL /Q /F *.pb.h
DEL /Q /F *.pb.cc
DEL /Q /F *.h

PAUSE