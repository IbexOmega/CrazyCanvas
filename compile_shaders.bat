:: Misc
"tools/glslc.exe" -O -fshader-stage=compute 	Assets/Shaders/Blur.glsl -o 					Assets/Shaders/Blur.spv
"tools/glslc.exe" -O -fshader-stage=vertex 		Assets/Shaders/FullscreenQuad.glsl -o 			Assets/Shaders/FullscreenQuad.spv

:: Geometry Pass
"tools/glslc.exe" -O -fshader-stage=vertex 		Assets/Shaders/GeometryDefVertex.glsl -o 		Assets/Shaders/GeometryDefVertex.spv
"tools/glslc.exe" -O -fshader-stage=fragment 	Assets/Shaders/GeometryDefPixel.glsl -o 		Assets/Shaders/GeometryDefPixel.spv

"tools/glslc.exe" -O -fshader-stage=vertex 		Assets/Shaders/GeometryVisVertex.glsl -o 		Assets/Shaders/GeometryVisVertex.spv
"tools/glslc.exe" -O -fshader-stage=fragment 	Assets/Shaders/GeometryVisPixel.glsl -o 		Assets/Shaders/GeometryVisPixel.spv

:: Light Pass
"tools/glslc.exe" -O -fshader-stage=vertex 		Assets/Shaders/LightVertex.glsl -o 				Assets/Shaders/LightVertex.spv
"tools/glslc.exe" -O -fshader-stage=fragment 	Assets/Shaders/LightPixel.glsl -o 				Assets/Shaders/LightPixel.spv

"tools/glslc.exe" -O -fshader-stage=fragment 	Assets/Shaders/ShadingVisPixel.glsl -o 			Assets/Shaders/ShadingVisPixel.spv
"tools/glslc.exe" -O -fshader-stage=fragment 	Assets/Shaders/ShadingDefPixel.glsl -o 			Assets/Shaders/ShadingDefPixel.spv

:: Raytracing

pause
