REM
REM Keenan's Hack to remove intermediate files
REM If you find a way to enhance, please return updates to
REM ekeenan2@cdm.depaul.edu
REM

REM cleanup root director

    if exist ..\Azul rmdir /S /Q ..\Azul
    if exist ..\bin rmdir /S /Q ..\bin
   REM if exist *.user del /F /s *.user
    if exist Azul rmdir /S /Q Azul
    if exist Logs rmdir /S /Q Logs
    if exist Debug rmdir /S /Q Debug
    if exist Release rmdir /S /Q Release
    if exist x64 rmdir /S /Q x64
    if exist obj rmdir /S /Q obj
    if exist bin rmdir /S /Q bin
    if exist ipch rmdir /S /Q ipch
    if exist .vs rmdir /S /Q .vs

REM  Generated Visual Studio solution + project files.
REM  premake5.lua is the source of truth -- run GenerateProjects.bat to
REM  recreate these. Deleted recursively so every project (app, libs, tests)
REM  is wiped. Not guarded by "if exist" because those live in subfolders.

    if exist *.sln del /F /Q *.sln
    del /F /S /Q *.vcxproj         >nul 2>&1
    del /F /S /Q *.vcxproj.filters >nul 2>&1
    del /F /S /Q *.vcxproj.user    >nul 2>&1

    if exist shaders\compiled rmdir /S /Q shaders\compiled


    if exist *.suo del /F /S /A:H *.suo
    if exist *.sdf del /F /S *.sdf
    if exist *.suo del /F /S *.suo
    if exist *.db del /F *.db