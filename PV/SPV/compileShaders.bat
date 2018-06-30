set "vulkanBinDir=%~dp0..\..\Libraries\Vulkan\Bin\"
set "shadersDir=%~dp0..\shaders\"

for /R %shadersDir% %%f in (*.vert) do %vulkanBinDir%\glslangValidator.exe -V %%f
for /R %shadersDir% %%f in (*.frag) do %vulkanBinDir%\glslangValidator.exe -V %%f

