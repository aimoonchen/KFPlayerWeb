../../../Github/emsdk/emsdk activate latest
source ../../../Github/emsdk/emsdk_env.sh
rm -rf build_web
mkdir build_web
cd build_web
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
emmake make