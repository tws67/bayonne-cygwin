@set oldpath=%PATH%
@path debug;%PATH%;%CAPE_DBG_PATH%

@cd ..\w32
server.exe -debug soundcard ..\tests\%1.scr
@cd ..\tests
@path %oldpath%

