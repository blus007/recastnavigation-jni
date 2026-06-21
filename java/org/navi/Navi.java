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

    // static class Log {
    //     void info(String s) {}
    //     void info(String s, Object o1) {}
    //     void warn(String s) {}
    //     void warn(String s, Object o1) {}
    //     void error(String s) {}
    //     void error(String s, Object o1) {}
    // }
    // public static final Log log = new Log();

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
             case 0:
                 log.info(message);
                 break;
             case 1:
                 log.warn(message);
                 break;
             case 2:
                 log.error(message);
                 break;
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
    private int bindCount = 0;
    private Thread activeThread = null;
    private float[] posArray = new float[MAX_SEARCH_POLYS * 3];
    private int[] posSize = new int[1];

    private synchronized void bindCurrentThread() {
        Thread current = Thread.currentThread();
        if (bindCount == 0) {
            activeThread = current;
        } else if (activeThread != current) {
            throwWrongThread(activeThread, current);
        }
        ++bindCount;
    }

    private synchronized void releaseCurrentThread() {
        if (bindCount <= 0) {
            return;
        }
        --bindCount;
        if (bindCount == 0) {
            activeThread = null;
        }
    }

    private void throwWrongThread(Thread owner, Thread current) {
        throw new IllegalStateException(String.format(
            "Navi instance is in use by thread [%s] (id=%d), cannot be used from thread [%s] (id=%d)",
            owner.getName(), owner.getId(), current.getName(), current.getId()));
    }

    public int getPosSize() {
        bindCurrentThread();
        try {
            return posSize[0];
        } finally {
            releaseCurrentThread();
        }
    }

    // 3 float(x, y, z) = 1 pos
    public float[] getPosArray() {
        bindCurrentThread();
        try {
            return posArray;
        } finally {
            releaseCurrentThread();
        }
    }

    public Navi() {
        create(MAX_SEARCH_POLYS, 0);
    }

    public Navi(int maxPoly, int maxObstacle) {
        create(maxPoly, maxObstacle);
    }

    public boolean isCreated() {
        bindCurrentThread();
        try {
            return naviPtr != 0;
        } finally {
            releaseCurrentThread();
        }
    }

    private native long createNative(int maxPoly, int maxObstacle);
    private void create(int maxPoly, int maxObstacle) {
        if (naviPtr != 0) {
            log.info("Create navi twice, ptr = {}", naviPtr);
            return;
        }
        bindCurrentThread();
        try {
            naviPtr = createNative(maxPoly, maxObstacle);
            if (naviPtr != 0) {
                createdNavis.put(naviPtr, System.currentTimeMillis());
                log.info("Create navi result = {}", naviPtr);
            } else {
                log.info("Create a null navi");
            }
        } finally {
            releaseCurrentThread();
        }
    }
    
    private native void destroyNative(long ptr);
    public void destroy() {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("Destroy a null navi");
                return;
            }
            long ptr = naviPtr;
            if (createdNavis.containsKey(ptr)) {
                createdNavis.remove(ptr);
                log.info("Destroy navi = {}", ptr);
                destroyNative(ptr);
                naviPtr = 0;
            } else {
                naviPtr = 0;
                log.error("Destroy but not find navi = {}", ptr);
            }
        } finally {
            releaseCurrentThread();
        }
    }
        
    private native void setDefaultPolySizeNative(long ptr, float x, float y, float z);
    public void setDefaultPolySize(float x, float y, float z) {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("setDefaultPolySize but navi is null");
                return;
            }
            setDefaultPolySizeNative(naviPtr, x, y, z);
        } finally {
            releaseCurrentThread();
        }
    }

    private native boolean loadMeshNative(long ptr, String filePath, int maxSearchNodes);
    public boolean loadMesh(String filePath) {
        return loadMesh(filePath, MAX_QUERY_INIT_NODE);
    }
    public boolean loadMesh(String filePath, int maxSearchNodes) {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("loadMesh but navi is null");
                return false;
            }
            return loadMeshNative(naviPtr, filePath, maxSearchNodes);
        } finally {
            releaseCurrentThread();
        }
    }
        
    private native boolean loadDoorsNative(long ptr, String filePath);
    public boolean loadDoors(String filePath) {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("loadDoors but navi is null");
                return false;
            }
            return loadDoorsNative(naviPtr, filePath);
        } finally {
            releaseCurrentThread();
        }
    }

    private native boolean loadRegionsNative(long ptr, String filePath);
    public boolean loadRegions(String filePath) {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("loadRegions but navi is null");
                return false;
            }
            return loadRegionsNative(naviPtr, filePath);
        } finally {
            releaseCurrentThread();
        }
    }
    
    private native int getRegionIdNative(long ptr, float x, float z);
    public int getRegionId(float x, float z) {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("getRegionId but navi is null");
                return 0;
            }
            return getRegionIdNative(naviPtr, x, z);
        } finally {
            releaseCurrentThread();
        }
    }

    private native void initDoorsPolyNative(long ptr);
    public void initDoorsPoly() {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("initDoorsPoly but navi is null");
                return;
            }
            initDoorsPolyNative(naviPtr);
        } finally {
            releaseCurrentThread();
        }
    }

    private native boolean isDoorExistNative(long ptr, int doorId);
    public boolean isDoorExist(int doorId) {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("isDoorExist but navi is null");
                return false;
            }
            return isDoorExistNative(naviPtr, doorId);
        } finally {
            releaseCurrentThread();
        }
    }
        
    private native boolean isDoorOpenNative(long ptr, int doorId);
    public boolean isDoorOpen(int doorId) {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("isDoorOpen but navi is null");
                return false;
            }
            return isDoorOpenNative(naviPtr, doorId);
        } finally {
            releaseCurrentThread();
        }
    }

    private native void openDoorNative(long ptr, int doorId, boolean open);
    public void openDoor(int doorId, boolean open) {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("openDoor but navi is null");
                return;
            }
            openDoorNative(naviPtr, doorId, open);
        } finally {
            releaseCurrentThread();
        }
    }
        
    private native void openAllDoorsNative(long ptr, boolean open);
    public void openAllDoors(boolean open) {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("openAllDoors but navi is null");
                return;
            }
            openAllDoorsNative(naviPtr, open);
        } finally {
            releaseCurrentThread();
        }
    }

    private native void closeAllDoorsPolyNative(long ptr);
    public void closeAllDoorsPoly() {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("closeAllDoorsPoly but navi is null");
                return;
            }
            closeAllDoorsPolyNative(naviPtr);
        } finally {
            releaseCurrentThread();
        }
    }

    private native void recoverAllDoorsPolyNative(long ptr);
    public void recoverAllDoorsPoly() {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("recoverAllDoorsPoly but navi is null");
                return;
            }
            recoverAllDoorsPolyNative(naviPtr);
        } finally {
            releaseCurrentThread();
        }
    }
        
    private native int addObstacleNative(long ptr,
         float posX, float posY, float posZ,
         float radius, float height);
    public int addObstacle(float posX, float posY, float posZ,
         float radius, float height) {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("addObstacle but navi is null");
                return 0;
            }
            return addObstacleNative(naviPtr, posX, posY, posZ, radius, height);
        } finally {
            releaseCurrentThread();
        }
    }
    public int addObstacleOffset(float posX, float posY, float posZ,
         float radius, float height) {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("addObstacleOffset but navi is null");
                return 0;
            }
            return addObstacleNative(naviPtr, posX, posY - 1.0f, posZ, radius, height + 1.0f);
        } finally {
            releaseCurrentThread();
        }
    }
        
    private native boolean removeObstacleNative(long ptr, int obstacleRef);
    public boolean removeObstacle(int obstacleRef) {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("removeObstacle but navi is null");
                return false;
            }
            return removeObstacleNative(naviPtr, obstacleRef);
        } finally {
            releaseCurrentThread();
        }
    }
        
    private native boolean refreshObstacleNative(long ptr);
    public boolean refreshObstacle() {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("refreshObstacle but navi is null");
                return false;
            }
            return refreshObstacleNative(naviPtr);
        } finally {
            releaseCurrentThread();
        }
    }

    private native int getMaxObstacleReqCountNative(long ptr);
    public int getMaxObstacleReqCount() {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("getMaxObstacleReqCount but navi is null");
                return 0;
            }
            return getMaxObstacleReqCountNative(naviPtr);
        } finally {
            releaseCurrentThread();
        }
    }

    private native int getAddedObstacleReqCountNative(long ptr);
    public int getAddedObstacleReqCount() {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("getAddedObstacleReqCount but navi is null");
                return 0;
            }
            return getAddedObstacleReqCountNative(naviPtr);
        } finally {
            releaseCurrentThread();
        }
    }

    private native int getObstacleReqRemainCountNative(long ptr);
    public int getObstacleReqRemainCount() {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("getObstacleReqRemainCount but navi is null");
                return 0;
            }
            return getObstacleReqRemainCountNative(naviPtr);
        } finally {
            releaseCurrentThread();
        }
    }

    private native int findPathNative(long ptr, float[] posArray, int arraySize, int[] posSize,
         float startX, float startY, float startZ,
         float endX, float endY, float endZ,
         float sizeX, float sizeY, float sizeZ);
    public int findPath(float startX, float startY, float startZ,
         float endX, float endY, float endZ,
         float sizeX, float sizeY, float sizeZ) {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("findPath but navi is null");
                return FAILURE;
            }
            return findPathNative(naviPtr, posArray, MAX_SEARCH_POLYS, posSize, startX, startY, startZ,
                endX, endY, endZ, sizeX, sizeY, sizeZ);
        } finally {
            releaseCurrentThread();
        }
    }
        
    private native int findPathDefaultNative(long ptr, float[] posArray, int arraySize, int[] posSize,
         float startX, float startY, float startZ,
         float endX, float endY, float endZ);
    public int findPath(float startX, float startY, float startZ,
         float endX, float endY, float endZ) {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("findPath default but navi is null");
                return FAILURE;
            }
            return findPathDefaultNative(naviPtr, posArray, MAX_SEARCH_POLYS, posSize, startX, startY, startZ,
                endX, endY, endZ);
        } finally {
            releaseCurrentThread();
        }
    }

    private native int makePathStraightNative(long ptr, float[] posArray, int arraySize,
         float sizeX, float sizeY, float sizeZ);
    public int makePathStraight(float[] posArray, int arraySize, float sizeX, float sizeY, float sizeZ) {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("makePathStraight but navi is null");
                return arraySize;
            }
            return makePathStraightNative(naviPtr, posArray, arraySize, sizeX, sizeY, sizeZ);
        } finally {
            releaseCurrentThread();
        }
    }

    private native int makePathStraightDefaultNative(long ptr, float[] posArray, int arraySize);
    public int makePathStraightDefault(float[] posArray, int arraySize) {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("makePathStraightDefault but navi is null");
                return arraySize;
            }
            return makePathStraightDefaultNative(naviPtr, posArray, arraySize);
        } finally {
            releaseCurrentThread();
        }
    }

    private native float pathRaycastNative(long ptr,
         float startX, float startY, float startZ,
         float endX, float endY, float endZ,
         float sizeX, float sizeY, float sizeZ);
    public float pathRaycast(float startX, float startY, float startZ,
         float endX, float endY, float endZ,
         float sizeX, float sizeY, float sizeZ) {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("pathRaycast but navi is null");
                return -1.0f;
            }
            return pathRaycastNative(naviPtr, startX, startY, startZ,
                endX, endY, endZ, sizeX, sizeY, sizeZ);
        } finally {
            releaseCurrentThread();
        }
    }

    private native float pathRaycastDefaultNative(long ptr,
         float startX, float startY, float startZ,
         float endX, float endY, float endZ);
    public float pathRaycastDefault(float startX, float startY, float startZ,
         float endX, float endY, float endZ) {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("pathRaycastDefault but navi is null");
                return -1.0f;
            }
            return pathRaycastDefaultNative(naviPtr, startX, startY, startZ,
                endX, endY, endZ);
        } finally {
            releaseCurrentThread();
        }
    }

    private native boolean canPathForwardNative(long ptr,
         float startX, float startY, float startZ,
         float endX, float endY, float endZ,
         float sizeX, float sizeY, float sizeZ);
    public boolean canPathForward(float startX, float startY, float startZ,
         float endX, float endY, float endZ,
         float sizeX, float sizeY, float sizeZ) {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("canPathForward but navi is null");
                return false;
            }
            return canPathForwardNative(naviPtr, startX, startY, startZ,
                endX, endY, endZ, sizeX, sizeY, sizeZ);
        } finally {
            releaseCurrentThread();
        }
    }

    private native boolean canPathForwardDefaultNative(long ptr,
         float startX, float startY, float startZ,
         float endX, float endY, float endZ);
    public boolean canPathForwardDefault(float startX, float startY, float startZ,
         float endX, float endY, float endZ) {
        bindCurrentThread();
        try {
            if (naviPtr == 0) {
                log.error("canPathForwardDefault but navi is null");
                return false;
            }
            return canPathForwardDefaultNative(naviPtr, startX, startY, startZ,
                endX, endY, endZ);
        } finally {
            releaseCurrentThread();
        }
    }
}