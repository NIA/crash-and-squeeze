@echo off
rem config, feel free to change
set LAUNCH=..\_Bin\Release\Renderer.exe
set REPEATS=4
set PHY_LOGFILE=measure_phy.log
set GFX_LOGFILE=measure_gfx.log

rem -----------------------------------
rem measurement commands, do NOT change
rem -----------------------------------

set LOGFILE=%PHY_LOGFILE%
echo.
echo *******************************
echo * Measuring physical vertices *
echo *******************************

echo. >> %LOGFILE%
echo =========================================================================== >> %LOGFILE%
rem iterating over the set of low model parameters
rem fixed params are params of high model
set FIXED_PARAMS=3/2/1
for %%p in (
            12/12/4
            18/17/5
            25/25/7
            41/40/10
            57/57/15
            82/81/20
            129/127/33
            182/182/46
            259/258/64
            407/408/103
           ) do (
  echo.
  echo Using params %%p %FIXED_PARAMS%
  for /L %%i in (1,1,%REPEATS%) do (
    echo Performing measurement %%i of %REPEATS%...
    %LAUNCH% %%p %FIXED_PARAMS% %LOGFILE%
  )
  echo. >> %LOGFILE%
  echo --------------------------------------------------------------------------- >> %LOGFILE%
)

set LOGFILE=%GFX_LOGFILE%
echo.
echo ********************************
echo * Measuring graphical vertices *
echo ********************************

echo. >> %LOGFILE%
echo =========================================================================== >> %LOGFILE%
rem iterating over the set of high model parameters
rem fixed params are params of low model
set FIXED_PARAMS=12/12/4
for %%p in (
            7/7/3
            12/12/4
            18/17/5
            25/25/7
            41/40/10
            57/57/15
            82/81/20
            129/127/33
            182/182/46
            259/258/64
            407/408/103
            577/576/145
            817/815/204
            1154/1155/289
            1825/1825/457
            2582/2580/646
           ) do (
  echo.
  echo Using params %FIXED_PARAMS% %%p
  for /L %%i in (1,1,%REPEATS%) do (
    echo Performing measurement %%i of %REPEATS%...
    %LAUNCH% %FIXED_PARAMS% %%p %LOGFILE%
  )
  echo. >> %LOGFILE%
  echo --------------------------------------------------------------------------- >> %LOGFILE%
)
pause

