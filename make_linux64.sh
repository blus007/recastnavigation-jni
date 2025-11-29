BUILD_PATH=build/Linux64
mkdir -p $BUILD_PATH
mkdir -p $BUILD_PATH/RecastDemo/Debug
mkdir -p $BUILD_PATH/RecastDemo/Release
cd $BUILD_PATH
cmake -G "Unix Makefiles" ../..
cd ../..
cmake --build $BUILD_PATH --config Release