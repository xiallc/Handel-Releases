@echo off
set hammer=.\swtoolkit\hammer.bat
call %hammer% -j4 --no-serial --no-xw --samples %*
exit /b %ERRORLEVEL%