DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"

./tools/glslc -fshader-stage=vertex assets/shaders/vertex.glsl -o assets/shaders/vertex.spv 
./tools/glslc -fshader-stage=fragment assets/shaders/fragment.glsl -o assets/shaders/fragment.spv

./tools/glslc -fshader-stage=vertex assets/shaders/geometryVertex.glsl -o assets/shaders/geometryVertex.spv 
./tools/glslc -fshader-stage=fragment assets/shaders/geometryFragment.glsl -o assets/shaders/geometryFragment.spv

./tools/glslc -fshader-stage=vertex assets/shaders/lightVertex.glsl -o assets/shaders/lightVertex.spv 
./tools/glslc -fshader-stage=fragment assets/shaders/lightFragment.glsl -o assets/shaders/lightFragment.spv

./tools/glslc -fshader-stage=vertex assets/shaders/skyboxVertex.glsl -o assets/shaders/skyboxVertex.spv 
./tools/glslc -fshader-stage=fragment assets/shaders/skyboxFragment.glsl -o assets/shaders/skyboxFragment.spv

./tools/glslc -fshader-stage=vertex assets/shaders/filterCubemap.glsl -o assets/shaders/filterCubemap.spv 
./tools/glslc -fshader-stage=fragment assets/shaders/genCubemapFragment.glsl -o assets/shaders/genCubemapFragment.spv
./tools/glslc -fshader-stage=fragment assets/shaders/genIrradianceFragment.glsl -o assets/shaders/genIrradianceFragment.spv