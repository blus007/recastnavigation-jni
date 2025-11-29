BUILD_PATH=build/Mac
mkdir -p $BUILD_PATH
mkdir -p $BUILD_PATH/RecastDemo/Debug
mkdir -p $BUILD_PATH/RecastDemo/Release
cd $BUILD_PATH
cmake -GXcode ../..
cd ../..
cmake --build $BUILD_PATH --config Release