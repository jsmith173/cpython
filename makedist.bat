@echo off

if "%PYSRC%" == "" goto error
if "%PYDIST%" == "" goto error

rd /s /q "%PYDIST%"
md "%PYDIST%"

xcopy "%PYSRC%\PCBuild\amd64\" "%PYDIST%\PCBuild\amd64\" /s /q

xcopy "%PYSRC%\Lib\" "%PYDIST%\Lib\" /s /q
xcopy "%PYSRC%\Scripts\" "%PYDIST%\Scripts\" /s /q


cd "%PYDIST%"
FOR /d /r . %%d IN ("__pycache__") DO @IF EXIST "%%d" rd /s /q "%%d"
cd %~dp0

del /q "%PYDIST%\PCBuild\amd64\*.pdb"

:done
echo Done
goto end

:error
echo Error found

:end
