:: Misc
"tools/glslc.exe" -O -fshader-stage=compute 	Assets/Shaders/blur.glsl -o 					Assets/Shaders/blur.spv
"tools/glslc.exe" -O -fshader-stage=vertex 		Assets/Shaders/fullscreenQuad.glsl -o 			Assets/Shaders/fullscreenQuad.spv

:: Geometry Pass
"tools/glslc.exe" -O -fshader-stage=vertex 		Assets/Shaders/geometryDefVertex.glsl -o 		Assets/Shaders/geometryDefVertex.spv
"tools/glslc.exe" -O -fshader-stage=fragment 	Assets/Shaders/geometryDefPixel.glsl -o 		Assets/Shaders/geometryDefPixel.spv

"tools/glslc.exe" -O -fshader-stage=vertex 		Assets/Shaders/geometryVisVertex.glsl -o 		Assets/Shaders/geometryVisVertex.spv
"tools/glslc.exe" -O -fshader-stage=fragment 	Assets/Shaders/geometryVisPixel.glsl -o 		Assets/Shaders/geometryVisPixel.spv

:: Light Pass
"tools/glslc.exe" -O -fshader-stage=vertex 		Assets/Shaders/lightVertex.glsl -o 				Assets/Shaders/lightVertex.spv
"tools/glslc.exe" -O -fshader-stage=fragment 	Assets/Shaders/lightPixel.glsl -o 				Assets/Shaders/lightPixel.spv

"tools/glslc.exe" -O -fshader-stage=fragment 	Assets/Shaders/shadingVisPixel.glsl -o 			Assets/Shaders/shadingVisPixel.spv
"tools/glslc.exe" -O -fshader-stage=fragment 	Assets/Shaders/shadingDefPixel.glsl -o 			Assets/Shaders/shadingDefPixel.spv

:: Raytracing

pause
