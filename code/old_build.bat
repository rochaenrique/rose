@echo off

raddbg --ipc kill_all

if not exist build (
	mkdir build
)

rem /wd4189 
rem /wd4100 unreferenced parameter
rem /wd4189 local variable initialized but no referenced
set COMPILER_FLAGS=/WX /W4 /Zi /FC /FS /wd4201 /nologo
set LINKER_FLAGS=/incremental:no
set PROJECT_COMPILER_FLAGS=/I.. /I..\ext /I"..\ext\freetype-2.14.2\include"
set PROJECT_LIBS=user32.lib gdI32.lib winmm.lib shell32.lib "..\ext\freetype-2.14.2\objs\freetype.lib"

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

   cl %COMPILER_FLAGS% %PROJECT_COMPILER_FLAGS% ..\examples\vulkan_stress_test.c %PROJECT_LIBS% /link %LINKER_FLAGS%  
   cl %COMPILER_FLAGS% %PROJECT_COMPILER_FLAGS% ..\examples\vulkan_multiple.c %PROJECT_LIBS% /link %LINKER_FLAGS%  
   cl %COMPILER_FLAGS% %PROJECT_COMPILER_FLAGS% ..\examples\gl_stress_test.c %PROJECT_LIBS% /link %LINKER_FLAGS%  
   cl %COMPILER_FLAGS% %PROJECT_COMPILER_FLAGS% ..\examples\gl_multiple.c %PROJECT_LIBS% /link %LINKER_FLAGS%  

   echo app dll
   cl %COMPILER_FLAGS% /D_USRDLL /D_WINDLL ..\marker_app.c /LD /link %LINKER_FLAGS% /DLL /PDB:marker_app_%timestamp%.pdb /EXPORT:update_and_render
popd

wsl ./build_linux.sh
