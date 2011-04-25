@echo off
rem config, feel free to change
set LAUNCH=..\_Bin\Release\Renderer.exe
set REPEATS=1
set LOGFILE=measure_threads.log

rem -----------------------------------
rem measurement commands, do NOT change
rem -----------------------------------

echo. >> %LOGFILE%
echo =========================================================================== >> %LOGFILE%
rem iterating over the different threads count
for %%n in (1 2 3 4) do (
  echo.
  echo Using %%n threads
  for /L %%i in (1,1,%REPEATS%) do (
    echo Performing measurement %%i of %REPEATS%...
    %LAUNCH% %%n %LOGFILE%
  )
  echo. >> %LOGFILE%
  echo --------------------------------------------------------------------------- >> %LOGFILE%
)
pause

