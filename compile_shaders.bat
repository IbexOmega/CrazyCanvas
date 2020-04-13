:: Misc
"tools/glslc.exe" -O -fshader-stage=compute 	Assets/Shaders/blur.glsl -o 					Assets/Shaders/blur.spv

:: Geometry Pass
"tools/glslc.exe" -O -fshader-stage=vertex 		Assets/Shaders/geometryVertex.glsl -o 			Assets/Shaders/geometryVertex.spv
"tools/glslc.exe" -O -fshader-stage=fragment 	Assets/Shaders/geometryPixel.glsl -o 			Assets/Shaders/geometryPixel.spv

:: Light Pass
"tools/glslc.exe" -O -fshader-stage=vertex 		Assets/Shaders/lightVertex.glsl -o 				Assets/Shaders/lightVertex.spv
"tools/glslc.exe" -O -fshader-stage=fragment 	Assets/Shaders/lightPixel.glsl -o 				Assets/Shaders/lightPixel.spv

:: Raytracing
"tools/glslc.exe" -O -fshader-stage=rgen 		Assets/Shaders/raygenRadiance.glsl -o 			Assets/Shaders/raygenRadiance.spv
"tools/glslc.exe" -O -fshader-stage=rmiss 		Assets/Shaders/missRadiance.glsl -o 			Assets/Shaders/missRadiance.spv
"tools/glslc.exe" -O -fshader-stage=rchit 		Assets/Shaders/closesthitRadiance.glsl -o 		Assets/Shaders/closesthitRadiance.spv
"tools/glslc.exe" -O -fshader-stage=rmiss 		Assets/Shaders/missShadow.glsl -o 				Assets/Shaders/missShadow.spv
"tools/glslc.exe" -O -fshader-stage=rchit 		Assets/Shaders/closesthitShadow.glsl -o 		Assets/Shaders/closesthitShadow.spv

:: Particles
"tools/glslc.exe" -O -fshader-stage=compute 	Assets/Shaders/particleUpdate.glsl -o 			Assets/Shaders/particleUpdate.spv

pause
