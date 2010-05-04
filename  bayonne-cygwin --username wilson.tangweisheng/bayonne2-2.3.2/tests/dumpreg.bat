@set oldpath=%PATH%
@path %PATH%;%CAPE_DBG_PATH%

@cd ..\w32\Debug
server.exe -dump-registry
@cd ..\..\tests
@path %oldpath%

