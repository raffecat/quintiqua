@echo off
mkdir dist-win32\Quintiqua
mkdir dist-win32\Quintiqua\ui
copy bin\client.exe dist-win32\Quintiqua\
copy data\* dist-win32\Quintiqua\
copy data\ui\* dist-win32\Quintiqua\ui\
del dist-win32\Quintiqua\client.log
pause
cls
