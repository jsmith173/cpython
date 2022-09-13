@echo off

if "%PYSRC%" == "" goto error
if "%PYDIST%" == "" goto error

rd /s /q "%PYDIST%"
md "%PYDIST%"

xcopy "%PYSRC%\PCBuild\amd64\" "%PYDIST%\PCBuild\amd64\" /s /q
xcopy "%PYSRC%\PCBuild\win32\" "%PYDIST%\PCBuild\win32\" /s /q

xcopy "%PYSRC%\Lib\" "%PYDIST%\Lib\" /s /q
xcopy "%PYSRC%\Scripts\" "%PYDIST%\Scripts\" /s /q
xcopy "%PYSRC%\Examples\" "%PYDIST%\Examples\" /s /q

copy "%PYSRC%\python64.b" "%PYDIST%\python64.bat"
copy "%PYSRC%\python32.b" "%PYDIST%\python32.bat"

cd "%PYDIST%"
FOR /d /r . %%d IN ("__pycache__") DO @IF EXIST "%%d" rd /s /q "%%d"
cd %~dp0

del /q "%PYDIST%\PCBuild\amd64\*.pdb"
del /q "%PYDIST%\PCBuild\amd64\*.ilk"

del /q "%PYDIST%\PCBuild\win32\*.pdb"
del /q "%PYDIST%\PCBuild\win32\*.ilk"

:done
echo Done
goto end

:error
echo Error found

:end
