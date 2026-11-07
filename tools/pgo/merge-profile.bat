@echo off
rem Merges every training session in the pgo folder into hortensia.profdata.
cd /d "%~dp0"
if not exist pgo\*.profraw (
  echo No profile found in the pgo folder. Play a few minutes first, then close Hortensia.
  pause
  exit /b 1
)
llvm-profdata.exe merge -o hortensia.profdata pgo\*.profraw
if errorlevel 1 (
  echo The merge failed.
  pause
  exit /b 1
)
echo.
echo Done: hortensia.profdata is next to this file. Send it to be built into Hortensia.
pause
