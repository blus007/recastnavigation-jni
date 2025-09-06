package org.navi;

public class Navi {
    static {
        System.loadLibrary("RecastJni");
        //System.out.println(String.format("getMaxPosSizeNative=%d", getMaxPosSizeNative()));
    }

    // level:0-debug 1-warning 2-error
    public static void jniLog(int level, String message) {
        // switch (level) {
        //     case 0: System.out.println(message); return;
        //     case 1: System.out.println(message); return;
        //     case 2: System.out.println(message); return;
        // }
    }

    public static final int FAILURE = 1 << 31; // Operation failed.
    public static final int SUCCESS = 1 << 30; // Operation succeed.

    public static boolean isFail(int status) {
        return (status & FAILURE) != 0;
    }

    public static boolean isSuccess(int status) {
        return (status & SUCCESS) != 0;
    }

    private static native int getMaxPosSizeNative();

    private long naviPtr = 0;
    private static final int MAX_POS_SIZE = getMaxPosSizeNative();
    private float[] posArray = new float[MAX_POS_SIZE * 3];
    private int[] posSize = new int[1];

    public boolean isCreated() {
        return naviPtr != 0;
    }

    public int getPosSize() {
        return posSize[0];
    }

    // 3 float(x, y, z) = 1 pos
    public float[] getPosArray() {
        return posArray;
    }

    private native long createNative();
    public void init() {
        if (naviPtr != 0)
            destroyNative(naviPtr);
        naviPtr = createNative();
    }
    
    private native void destroyNative(long ptr);
    public void destroy() {
        if (naviPtr == 0)
            return;
        destroyNative(naviPtr);
    }
        
    private native void setDefaultPolySizeNative(long ptr, float x, float y, float z);
    public void setDefaultPolySize(float x, float y, float z) {
        if (naviPtr == 0)
            return;
        setDefaultPolySizeNative(naviPtr, x, y, z);
    }

    private native boolean loadMeshNative(long ptr, String filePath);
    public boolean loadMesh(String filePath) {
        if (naviPtr == 0)
            return false;
        return loadMeshNative(naviPtr, filePath);
    }
        
    private native boolean loadDoorsNative(long ptr, String filePath);
    public boolean loadDoors(String filePath) {
        if (naviPtr == 0)
            return false;
        return loadDoorsNative(naviPtr, filePath);
    }

    private native boolean loadRegionsNative(long ptr, String filePath);
    public boolean loadRegions(String filePath) {
        if (naviPtr == 0)
            return false;
        return loadRegionsNative(naviPtr, filePath);
    }
    
    private native int getRegionIdNative(long ptr, float x, float z);
    public int getRegionId(float x, float z) {
        if (naviPtr == 0)
            return 0;
        return getRegionIdNative(naviPtr, x, z);
    }

    private native void initDoorsPolyNative(long ptr);
    public void initDoorsPoly() {
        if (naviPtr == 0)
            return;
        initDoorsPolyNative(naviPtr);
    }

    private native boolean isDoorExistNative(long ptr, int doorId);
    public boolean isDoorExist(int doorId) {
        if (naviPtr == 0)
            return false;
        return isDoorExistNative(naviPtr, doorId);
    }
        
    private native boolean isDoorOpenNative(long ptr, int doorId);
    public boolean isDoorOpen(int doorId) {
        if (naviPtr == 0)
            return false;
        return isDoorOpenNative(naviPtr, doorId);
    }

    private native void openDoorNative(long ptr, int doorId, boolean open);
    public void openDoor(int doorId, boolean open) {
        if (naviPtr == 0)
            return;
        openDoorNative(naviPtr, doorId, open);
    }
        
    private native void openAllDoorsNative(long ptr, boolean open);
    public void openAllDoors(long ptr, boolean open) {
        if (naviPtr == 0)
            return;
        openAllDoorsNative(naviPtr, open);
    }

    private native void closeAllDoorsPolyNative(long ptr);
    public void closeAllDoorsPoly() {
        if (naviPtr == 0)
            return;
        closeAllDoorsPolyNative(naviPtr);
    }

    private native void recoverAllDoorsPolyNative(long ptr);
    public void recoverAllDoorsPoly() {
        if (naviPtr == 0)
            return;
        recoverAllDoorsPolyNative(naviPtr);
    }
        
    private native int addObstacleNative(long ptr,
         float posX, float posY, float posZ,
         float radius, float height);
    public int addObstacle(float posX, float posY, float posZ,
         float radius, float height) {
        if (naviPtr == 0)
            return 0;
        return addObstacleNative(naviPtr, posX, posY, posZ, radius, height);
    }
    public int addObstacleOffset(float posX, float posY, float posZ,
         float radius, float height) {
        if (naviPtr == 0)
            return 0;
        return addObstacleNative(naviPtr, posX, posY - 1.0f, posZ, radius, height + 1.0f);
    }
        
    private native boolean removeObstacleNative(long ptr, int obstacleRef);
    public boolean removeObstacle(int obstacleRef) {
        if (naviPtr == 0)
            return false;
        return removeObstacleNative(naviPtr, obstacleRef);
    }
        
    private native boolean refreshObstacleNative(long ptr);
    public boolean refreshObstacle() {
        if (naviPtr == 0)
            return false;
        return refreshObstacleNative(naviPtr);
    }

    private native int getMaxObstacleReqCountNative(long ptr);
    public int getMaxObstacleReqCount() {
    	if (naviPtr == 0)
            return 0;
        return getMaxObstacleReqCountNative(naviPtr);
    }

    private native int getAddedObstacleReqCountNative(long ptr);
    public int getAddedObstacleReqCount() {
    	if (naviPtr == 0)
            return 0;
        return getAddedObstacleReqCountNative(naviPtr);
    }

    private native int getObstacleReqRemainCountNative(long ptr);
    public int getObstacleReqRemainCount() {
    	if (naviPtr == 0)
            return 0;
        return getObstacleReqRemainCountNative(naviPtr);
    }

    private native int findPathNative(long ptr, float[] posArray, int[] posSize,
         float startX, float startY, float startZ,
         float endX, float endY, float endZ,
         float sizeX, float sizeY, float sizeZ);
    public int findPath(float startX, float startY, float startZ,
         float endX, float endY, float endZ,
         float sizeX, float sizeY, float sizeZ) {
        if (naviPtr == 0)
            return FAILURE;
        return findPathNative(naviPtr, posArray, posSize, startX, startY, startZ, 
            endX, endY, endZ, sizeX, sizeY, sizeZ);
    }
        
    private native int findPathDefaultNative(long ptr, float[] posArray, int[] posSize,
         float startX, float startY, float startZ,
         float endX, float endY, float endZ);
    public int findPath(float startX, float startY, float startZ,
         float endX, float endY, float endZ) {
        if (naviPtr == 0)
            return FAILURE;
        return findPathDefaultNative(naviPtr, posArray, posSize, startX, startY, startZ, 
            endX, endY, endZ);
    }
}