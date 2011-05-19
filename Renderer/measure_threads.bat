@echo off
rem config, feel free to change
set LAUNCH=..\_Bin\Release\Renderer.exe
set REPEATS=4
set LOGFILE=measure_threads.log
set VERTICES_COEF=5
set DURATION=5

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
    %LAUNCH% %%n %VERTICES_COEF% %DURATION% %LOGFILE%
  )
  echo. >> %LOGFILE%
  echo --------------------------------------------------------------------------- >> %LOGFILE%
)
pause

