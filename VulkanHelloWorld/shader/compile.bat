cd /d "%~dp0"
echo Compiling shaders...

glslangValidator.exe -V vert.vert -o vert.spv
glslangValidator.exe -V frag.frag -o frag.spv
glslangValidator.exe -V shadowVert.vert -o shadowVert.spv
pause