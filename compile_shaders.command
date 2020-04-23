DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"

./tools/glslc -O -fshader-stage=compute Assets/Shaders/blur.glsl -o assets/shaders/blur.spv 
./tools/glslc -O -fshader-stage=vertex Assets/Shaders/fullscreenQuad.glsl -o assets/shaders/fullscreenQuad.spv

./tools/glslc -O -fshader-stage=vertex Assets/Shaders/geometryDefVertex.glsl -o assets/shaders/geometryDefVertex.spv 
./tools/glslc -O -fshader-stage=fragment Assets/Shaders/geometryDefPixel.glsl -o assets/shaders/geometryDefPixel.spv

./tools/glslc -O -fshader-stage=vertex Assets/Shaders/geometryVisVertex.glsl -o assets/shaders/geometryVisVertex.spv 
./tools/glslc -O -fshader-stage=fragment Assets/Shaders/geometryVisPixel.glsl -o assets/shaders/geometryVisPixel.spv

./tools/glslc -O -fshader-stage=vertex Assets/Shaders/lightVertex.glsl -o assets/shaders/lightVertex.spv 
./tools/glslc -O -fshader-stage=fragment Assets/Shaders/lightPixel.glsl -o assets/shaders/lightPixel.spv

./tools/glslc -O -fshader-stage=fragment Assets/Shaders/shadingVisPixel.glsl -o assets/shaders/shadingVisPixel.spv 
./tools/glslc -O -fshader-stage=fragment Assets/Shaders/shadingDefPixel.glsl -o assets/shaders/shadingDefPixel.spv