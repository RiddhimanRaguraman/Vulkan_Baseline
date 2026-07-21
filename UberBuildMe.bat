@echo off
REM -----------------------------------------------------------------
REM Copyright 2026 by Riddhiman Raguraman
REM -----------------------------------------------------------------

setlocal

pushd "%~dp0"

call "UberCleanMe.Bat"

call "Vendor\bin\premake\premake5.exe" vs2022

popd
endlocal
