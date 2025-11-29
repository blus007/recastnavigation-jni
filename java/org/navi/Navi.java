package org.navi;

import lombok.extern.slf4j.Slf4j;

import java.nio.file.Paths;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

@Slf4j
public class Navi {
    public static final int SYSTEM_NONE = 0;
    public static final int SYSTEM_WINDOWS = 1;
    public static final int SYSTEM_LINUX = 2;
    public static final int SYSTEM_MAC = 3;

    public static int getSystem() {
        String osName = System.getProperty("os.name").toLowerCase();
        if (osName.contains("win")) {
            return SYSTEM_WINDOWS;
        } else if (osName.contains("nix") || osName.contains("nux") || osName.contains("aix")) {
            return SYSTEM_LINUX;
        } else if (osName.contains("mac")) {
            return SYSTEM_MAC;
        } else {
            return SYSTEM_NONE;
        }
    }

    public static void init() {
        log.info("Start load navi library =============================");
        int system = getSystem();
        String pwd = System.getProperty("user.dir");
        String libPath;
        if (system == SYSTEM_WINDOWS) {
            libPath = Paths.get(pwd, "Lib", "Windows", "RecastJni.dll").toString();
        } else if (system == SYSTEM_LINUX) {
            libPath = Paths.get(pwd, "Lib", "Linux", "libRecastJni.so").toString();
        } else if (system == SYSTEM_MAC) {
            libPath = Paths.get(pwd, "Lib", "Mac", "libRecastJni.dylib").toString();
        } else {
            String osName = System.getProperty("os.name");
            throw new RuntimeException("Unsupported system: " + osName);
        }
        System.load(libPath);
        log.info("Load navi library success =============================");
    }

    // level:0-debug 1-warning 2-error
    public static void jniLog(int level, String message) {
         switch (level) {
             case 0: log.info(message);
             case 1: log.warn(message);
             case 2: log.error(message);
         }
    }

    public static final int FAILURE = 1 << 31; // Operation failed.
    public static final int SUCCESS = 1 << 30; // Operation succeed.
    public static final int MAX_QUERY_INIT_NODE = 65535;
    public static final int MAX_SEARCH_POLYS = 1024;

    private static final Map<Long, Long> createdNavis = new ConcurrentHashMap<>();
    public static final Map<Long, Long> getCreatedNavis() {
        return createdNavis;
    }

    public static boolean isFail(int status) {
        return (status & FAILURE) != 0;
    }

    public static boolean isSuccess(int status) {
        return (status & SUCCESS) != 0;
    }

    private static native int getMaxPosSizeNative();

    private long naviPtr = 0;
    private float[] posArray = new float[MAX_SEARCH_POLYS * 3];
    private int[] posSize = new int[1];

    public int getPosSize() {
        return posSize[0];
    }

    // 3 float(x, y, z) = 1 pos
    public float[] getPosArray() {
        return posArray;
    }

    public Navi() {
        create(MAX_SEARCH_POLYS);
    }

    public Navi(int maxPoly) {
        create(maxPoly);
    }

    public boolean isCreated() {
        return naviPtr != 0;
    }

    private native long createNative(int maxPoly, int maxObstacle);
    private void create(int maxPoly, int maxObstacle) {
        if (naviPtr != 0) {
            log.info("Create navi twice, ptr = {}", naviPtr);
            return;
        }
        naviPtr = createNative(maxPoly, maxObstacle);
        if (naviPtr != 0) {
            createdNavis.put(naviPtr, System.currentTimeMillis());
            log.info("Create navi result = {}", naviPtr);
        } else {
            log.info("Create a null navi");
        }
    }
    
    private native void destroyNative(long ptr);
    public void destroy() {
        if (naviPtr == 0) {
            log.error("Destroy a null navi");
            return;
        }
        if (createdNavis.containsKey(naviPtr)) {
            createdNavis.remove(naviPtr);
            log.info("Destroy navi = {}", naviPtr);
        } else {
            naviPtr = 0;
            log.error("Destroy but not find navi = {}", naviPtr);
            return;
        }
        destroyNative(naviPtr);
        naviPtr = 0;
    }
        
    private native void setDefaultPolySizeNative(long ptr, float x, float y, float z);
    public void setDefaultPolySize(float x, float y, float z) {
        if (naviPtr == 0) {
            log.error("setDefaultPolySize but navi is null");
            return;
        }
        setDefaultPolySizeNative(naviPtr, x, y, z);
    }

    private native boolean loadMeshNative(long ptr, String filePath, int maxSearchNodes);
    public boolean loadMesh(String filePath) {
        return loadMesh(filePath, MAX_QUERY_INIT_NODE);
    }
    public boolean loadMesh(String filePath, int maxSearchNodes) {
        if (naviPtr == 0) {
            log.error("loadMesh but navi is null");
            return false;
        }
        return loadMeshNative(naviPtr, filePath, maxSearchNodes);
    }
        
    private native boolean loadDoorsNative(long ptr, String filePath);
    public boolean loadDoors(String filePath) {
        if (naviPtr == 0) {
            log.error("loadDoors but navi is null");
            return false;
        }
        return loadDoorsNative(naviPtr, filePath);
    }

    private native boolean loadRegionsNative(long ptr, String filePath);
    public boolean loadRegions(String filePath) {
        if (naviPtr == 0) {
            log.error("loadRegions but navi is null");
            return false;
        }
        return loadRegionsNative(naviPtr, filePath);
    }
    
    private native int getRegionIdNative(long ptr, float x, float z);
    public int getRegionId(float x, float z) {
        if (naviPtr == 0) {
            log.error("getRegionId but navi is null");
            return 0;
        }
        return getRegionIdNative(naviPtr, x, z);
    }

    private native void initDoorsPolyNative(long ptr);
    public void initDoorsPoly() {
        if (naviPtr == 0) {
            log.error("initDoorsPoly but navi is null");
            return;
        }
        initDoorsPolyNative(naviPtr);
    }

    private native boolean isDoorExistNative(long ptr, int doorId);
    public boolean isDoorExist(int doorId) {
        if (naviPtr == 0) {
            log.error("isDoorExist but navi is null");
            return false;
        }
        return isDoorExistNative(naviPtr, doorId);
    }
        
    private native boolean isDoorOpenNative(long ptr, int doorId);
    public boolean isDoorOpen(int doorId) {
        if (naviPtr == 0) {
            log.error("isDoorOpen but navi is null");
            return false;
        }
        return isDoorOpenNative(naviPtr, doorId);
    }

    private native void openDoorNative(long ptr, int doorId, boolean open);
    public void openDoor(int doorId, boolean open) {
        if (naviPtr == 0) {
            log.error("openDoor but navi is null");
            return;
        }
        openDoorNative(naviPtr, doorId, open);
    }
        
    private native void openAllDoorsNative(long ptr, boolean open);
    public void openAllDoors(boolean open) {
        if (naviPtr == 0) {
            log.error("openAllDoors but navi is null");
            return;
        }
        openAllDoorsNative(naviPtr, open);
    }

    private native void closeAllDoorsPolyNative(long ptr);
    public void closeAllDoorsPoly() {
        if (naviPtr == 0) {
            log.error("closeAllDoorsPoly but navi is null");
            return;
        }
        closeAllDoorsPolyNative(naviPtr);
    }

    private native void recoverAllDoorsPolyNative(long ptr);
    public void recoverAllDoorsPoly() {
        if (naviPtr == 0) {
            log.error("recoverAllDoorsPoly but navi is null");
            return;
        }
        recoverAllDoorsPolyNative(naviPtr);
    }
        
    private native int addObstacleNative(long ptr,
         float posX, float posY, float posZ,
         float radius, float height);
    public int addObstacle(float posX, float posY, float posZ,
         float radius, float height) {
        if (naviPtr == 0) {
            log.error("addObstacle but navi is null");
            return 0;
        }
        return addObstacleNative(naviPtr, posX, posY, posZ, radius, height);
    }
    public int addObstacleOffset(float posX, float posY, float posZ,
         float radius, float height) {
        if (naviPtr == 0) {
            log.error("addObstacleOffset but navi is null");
            return 0;
        }
        return addObstacleNative(naviPtr, posX, posY - 1.0f, posZ, radius, height + 1.0f);
    }
        
    private native boolean removeObstacleNative(long ptr, int obstacleRef);
    public boolean removeObstacle(int obstacleRef) {
        if (naviPtr == 0) {
            log.error("removeObstacle but navi is null");
            return false;
        }
        return removeObstacleNative(naviPtr, obstacleRef);
    }
        
    private native boolean refreshObstacleNative(long ptr);
    public boolean refreshObstacle() {
        if (naviPtr == 0) {
            log.error("refreshObstacle but navi is null");
            return false;
        }
        return refreshObstacleNative(naviPtr);
    }

    private native int getMaxObstacleReqCountNative(long ptr);
    public int getMaxObstacleReqCount() {
        if (naviPtr == 0) {
            log.error("getMaxObstacleReqCount but navi is null");
            return 0;
        }
        return getMaxObstacleReqCountNative(naviPtr);
    }

    private native int getAddedObstacleReqCountNative(long ptr);
    public int getAddedObstacleReqCount() {
        if (naviPtr == 0) {
            log.error("getAddedObstacleReqCount but navi is null");
            return 0;
        }
        return getAddedObstacleReqCountNative(naviPtr);
    }

    private native int getObstacleReqRemainCountNative(long ptr);
    public int getObstacleReqRemainCount() {
        if (naviPtr == 0) {
            log.error("getObstacleReqRemainCount but navi is null");
            return 0;
        }
        return getObstacleReqRemainCountNative(naviPtr);
    }

    private native int findPathNative(long ptr, float[] posArray, int arraySize, int[] posSize,
         float startX, float startY, float startZ,
         float endX, float endY, float endZ,
         float sizeX, float sizeY, float sizeZ);
    public int findPath(float startX, float startY, float startZ,
         float endX, float endY, float endZ,
         float sizeX, float sizeY, float sizeZ) {
        if (naviPtr == 0) {
            log.error("findPath but navi is null");
            return FAILURE;
        }
        return findPathNative(naviPtr, posArray, MAX_SEARCH_POLYS, posSize, startX, startY, startZ,
            endX, endY, endZ, sizeX, sizeY, sizeZ);
    }
        
    private native int findPathDefaultNative(long ptr, float[] posArray, int arraySize, int[] posSize,
         float startX, float startY, float startZ,
         float endX, float endY, float endZ);
    public int findPath(float startX, float startY, float startZ,
         float endX, float endY, float endZ) {
        if (naviPtr == 0) {
            log.error("findPath default but navi is null");
            return FAILURE;
        }
        return findPathDefaultNative(naviPtr, posArray, MAX_SEARCH_POLYS, posSize, startX, startY, startZ,
            endX, endY, endZ);
    }

    private native int makePathStraightNative(long ptr, float[] posArray, int arraySize,
         float sizeX, float sizeY, float sizeZ);
    public int makePathStraight(float[] posArray, int arraySize, float sizeX, float sizeY, float sizeZ) {
        if (naviPtr == 0) {
            log.error("makePathStraight but navi is null");
            return FAILURE;
        }
        return makePathStraightNative(naviPtr, posArray, arraySize, sizeX, sizeY, sizeZ);
    }

    private native int makePathStraightDefaultNative(long ptr, float[] posArray, int arraySize);
    public int makePathStraightDefault(float[] posArray, int arraySize) {
        if (naviPtr == 0) {
            log.error("makePathStraightDefault but navi is null");
            return FAILURE;
        }
        return makePathStraightDefaultNative(naviPtr, posArray, arraySize);
    }
}