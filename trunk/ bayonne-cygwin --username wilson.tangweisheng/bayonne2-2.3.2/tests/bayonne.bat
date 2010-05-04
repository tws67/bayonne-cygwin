@echo off
if %1 == "" goto soundcard
if %1 == "soundcard" goto soundcard
if %1 == "sip" goto sip
if %1 == "vpb" goto voicetronix
if %1 == "voicetronix" goto voicetronix

:soundcard
        bayonne.exe -load soundcard
        exit

:sip
        bayonne.exe -load sip
        exit

:voicetronix
        bayonne.exe -load voicetronix
        exit


