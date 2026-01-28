@echo off
REM Relance le script cicd/launch.bat (racine du projet)
cd /d "%~dp0"
call "cicd\launch.bat"
