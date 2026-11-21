@echo off

if 1==1 ( 
  echo builder build
  if not exist builder (
    mkdir builder
  )
  pushd builder
    del *.pdb > NUL 2> NUL
    del *.rdi > NUL 2> NUL
    rem cl /WX /W4 /FC /FS /Zi /wd4201 /nologo /I.. ..\builder.c user32.lib gdI32.lib winmm.lib shell32.lib /link /incremental:no
    cl /WX /W4 /FC /O2 /wd4201 /nologo /I.. ..\builder.c user32.lib gdI32.lib winmm.lib shell32.lib /link /incremental:no
  popd
)

if 1==1 (
  raddbg --ipc kill_all
  set BLD_CC_FLAGS=DEBUG
  .\builder\builder examples\win32_broadcast.c win32 msvc
)

if 0==1 (
  echo app build
  pushd build
    cl /FC /nologo /WX /W4 /Z7 /FS /wd4201 /wd4100 /D_USRDLL /D_WINDLL /I.. /I..\external\freetype ..\win32_ui_app.c user32.lib shell32.lib winmm.lib gdI32.lib /LD /link /incremental:no /DLL /EXPORT:update_and_render
  popd
)
