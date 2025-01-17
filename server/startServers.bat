@echo off

echo serverStart script started.

start "" "D:\Cat_Codes\ChatProject\server\GateServer\x64\Debug\GateServer.exe"
echo [GateServer] start success!


start "" "D:\Cat_Codes\ChatProject\server\StatusServer\x64\Debug\StatusServer.exe"
echo [StatusServer] start success!

start "" "D:\Cat_Codes\ChatProject\server\ChatServer1\x64\Debug\ChatServer.exe"
echo [ChatServer1] start success!

start "" "D:\Cat_Codes\ChatProject\server\ChatServer2\x64\Debug\ChatServer.exe" 
echo [ChatServer2] start success!
