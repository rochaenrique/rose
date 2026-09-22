call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"

start "ROSE" "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"

pushd code
start "4coder_title" 4ed -W
start "raddbg_title" raddbg --project:"project.raddbg"
popd

