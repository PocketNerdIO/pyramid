@echo off
if "%1" == "" goto usage

tsc /m games /sprjname=%1
goto quit

:usage
echo "Usage MKGAME <GAMENAME>"

:quit
