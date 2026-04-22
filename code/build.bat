@echo off

if not exist build (
	mkdir build
)

rem /wd4189 
set COMPILER_FLAGS=/WX /W4 /Zi /FC /wd4201 /wd4100 /nologo

set LINKER_FLAGS=/incremental:no

for /F "tokens=1-4 delims=:. " %%a in ("%time%") do (
	set HH=%%a
	set MM=%%b
  set SS=%%c
  set MS=%%d
)
set timestamp=%date:~-4,4%%date:~-10,2%%date:~-7,2%_%HH%%MM%%SS%%MS%

pushd build
del *.pdb > NUL 2> NUL
del *.rdi > NUL 2> NUL

cl %COMPILER_FLAGS% /D_USRDLL /D_WINDLL ..\marker_app.c /LD /link %LINKER_FLAGS% /DLL /PDB:marker_app_%timestamp%.pdb /EXPORT:update_and_render

cl %COMPILER_FLAGS% /I../ext /I"..\ext\freetype-2.14.2\include" ..\win32_main.c user32.lib gdI32.lib winmm.lib /link %LINKER_FLAGS% "..\ext\freetype-2.14.2\objs\freetype.lib" 

popd

rem copy build ..\current_build\build
