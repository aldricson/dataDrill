@echo off
setlocal

:: Define source folder and destination folder paths
set "sourceFolder=%~dp0"   :: Folder where the script is located
set "destinationFolder=C:\build\23.0\x64\sysroots\core2-64-nilrt-linux\usr\lib"

:: Define filenames to be copied
set "file1=libmodbus.so"
set "file2=libmodbus.so.5.0.5"
set "file3=libIniParser.so"

:: Check if the source files exist
if exist "%sourceFolder%%file1%" (
    :: Copy the first file
    copy /Y "%sourceFolder%%file1%" "%destinationFolder%\%file1%"
    echo Copied %file1% to %destinationFolder%
) else (
    echo %file1% not found in the source folder.
)

if exist "%sourceFolder%%file2%" (
    :: Copy the second file
    copy /Y "%sourceFolder%%file2%" "%destinationFolder%\%file2%"
    echo Copied %file2% to %destinationFolder%
) else (
    echo %file2% not found in the source folder.
)

if exist "%sourceFolder%%file3%" (
    :: Copy the second file
    copy /Y "%sourceFolder%%file3%" "%destinationFolder%\%file3%"
    echo Copied %file3% to %destinationFolder%
) else (
    echo %file3% not found in the source folder.
)

endlocal