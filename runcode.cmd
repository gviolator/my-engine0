@echo off

@set PROJECT_PATH=%~dp0
call "%PROJECT_PATH%\extras\win\env_vc.cmd" 18
rem set PATH=%LLVM%\bin;%PATH%
rem set PATH=s:\Programs\llvm-18\bin;s:\Programs\ninja;%PATH%
set PATH=l:\Programs\llvm-22\bin;l:\Programs\ninja;%PATH%

code "%PROJECT_PATH%" | exit /b
