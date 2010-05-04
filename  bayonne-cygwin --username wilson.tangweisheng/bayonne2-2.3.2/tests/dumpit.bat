@set oldpath=%PATH%
@path %PATH%;%CAPE_DBG_PATH%

@cd ..\w32\Debug
server.exe -dump-testing
@cd ..\..\tests
@path %oldpath%

