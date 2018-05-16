set "vulkanBinDir=%~dp0..\..\Libraries\Vulkan\Bin\"
set "shadersDir=%~dp0..\shaders\"

@ Compile vertex shaders
for /R %shadersDir% %%f in (*.vert) do %vulkanBinDir%\glslangValidator.exe -V %%f

@ Compile fragment shaders
for /R %shadersDir% %%f in (*.frag) do %vulkanBinDir%\glslangValidator.exe -V %%f

@DEBUG
@echo %vulkanBinDir%
@echo %shadersDir%
