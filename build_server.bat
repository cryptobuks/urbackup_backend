call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"

msbuild UrBackupBackend.sln /p:Configuration=Release /p:Platform="win32"
if %errorlevel% neq 0 exit /b %errorlevel% 

msbuild UrBackupBackend.sln /p:Configuration=Release /p:Platform="x64"
if %errorlevel% neq 0 exit /b %errorlevel%

msbuild UrBackupBackend.sln /p:Configuration="Release Server PreVista" /p:Platform="win32"
if %errorlevel% neq 0 exit /b %errorlevel%

msbuild UrBackupBackend.sln /p:Configuration="Release Server PreVista" /p:Platform="x64"
if %errorlevel% neq 0 exit /b %errorlevel% 

msbuild UrBackupBackend.sln /p:Configuration="Release Service" /p:Platform="x64"
if %errorlevel% neq 0 exit /b %errorlevel%

msbuild UrBackupBackend.sln /p:Configuration="Release Service" /p:Platform="win32"
if %errorlevel% neq 0 exit /b %errorlevel%

msbuild UrBackupBackend.sln /p:Configuration="Release x64" /p:Platform="x64"
if %errorlevel% neq 0 exit /b %errorlevel%

msbuild urbackupserver\urbackupserver.vcxproj /p:Configuration="Release Server" /p:Platform="win32"
if %errorlevel% neq 0 exit /b %errorlevel%

mkdir "Release Server"
copy /Y "urbackupserver\Release Server\*" "Release Server\"

msbuild urbackupserver\urbackupserver.vcxproj /p:Configuration="Release Server" /p:Platform="x64"
if %errorlevel% neq 0 exit /b %errorlevel%

mkdir "x64\Release Server"
copy /Y "urbackupserver\x64\Release Server\*" "x64\Release Server\"

msbuild urbackupserver\urbackupserver.vcxproj /p:Configuration="Release Server PreVista" /p:Platform="x64"
if %errorlevel% neq 0 exit /b %errorlevel%

mkdir "RRelease Server PreVista"
copy /Y "urbackupserver\Release Server PreVista\*" "Release Server PreVista\"

msbuild urbackupserver\urbackupserver.vcxproj /p:Configuration="Release Server PreVista" /p:Platform="win32"
if %errorlevel% neq 0 exit /b %errorlevel%

mkdir "x64\Release Server PreVista"
copy /Y "urbackupserver\x64\Release Server PreVista\*" "x64\Release Server PreVista\"

"%~dp0urbackupserver_installer_win/generate_msi.bat"
if %errorlevel% neq 0 exit /b %errorlevel%

%"C:\Program Files (x86)\NSIS\makensis.exe" "%~dp0urbackupserver_installer_win/urbackup_server.nsi"
if %errorlevel% neq 0 exit /b %errorlevel%

exit 0