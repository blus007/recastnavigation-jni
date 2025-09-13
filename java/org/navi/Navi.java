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

    public static boolean isFail(int status) {
        return (status & FAILURE) != 0;
    }

    public static boolean isSuccess(int status) {
        return (status & SUCCESS) != 0;
    }

    private static native int getMaxPosSizeNative();

    private float[] posArray = new float[MAX_SEARCH_POLYS * 3];
    private int[] posSize = new int[1];

    public int getPosSize() {
        return posSize[0];
    }

    // 3 float(x, y, z) = 1 pos
    public float[] getPosArray() {
        return posArray;
    }

    private native long createNative(int maxPoly);
    public long create() {
        return create(MAX_SEARCH_POLYS);
    }
    public long create(int maxPoly) {
        long naviPtr = createNative(maxPoly);
        if (naviPtr != 0) {
            createdNavis.put(naviPtr, System.currentTimeMillis());
            log.info("Create navi from create(int maxPoly), result = {}", naviPtr);
        } else {
            log.info("Create a null navi from create(int maxPoly)");
        }
        return naviPtr;
    }
    
    private native void destroyNative(long ptr);
    public void destroy(long naviPtr) {
        if (naviPtr == 0) {
            log.error("Destroy but navi is null");
            return;
        }
        if (createdNavis.containsKey(naviPtr)) {
            createdNavis.remove(naviPtr);
            log.info("Destroy navi = {}", naviPtr);
        } else {
            log.error("Destroy but not find navi = {}", naviPtr);
            return;
        }
        destroyNative(naviPtr);
    }
        
    private native void setDefaultPolySizeNative(long ptr, float x, float y, float z);
    public void setDefaultPolySize(long naviPtr, float x, float y, float z) {
        if (naviPtr == 0) {
            log.error("setDefaultPolySize but navi is null");
            return;
        }
        if (!createdNavis.containsKey(naviPtr)) {
            log.error("setDefaultPolySize but not find navi = {}", naviPtr);
            return;
        }
        setDefaultPolySizeNative(naviPtr, x, y, z);
    }

    private native boolean loadMeshNative(long ptr, String filePath, int maxSearchNodes);
    public boolean loadMesh(long naviPtr, String filePath) {
        return loadMesh(naviPtr, filePath, MAX_QUERY_INIT_NODE);
    }
    public boolean loadMesh(long naviPtr, String filePath, int maxSearchNodes) {
        if (naviPtr == 0) {
            log.error("loadMesh but navi is null");
            return false;
        }
        if (!createdNavis.containsKey(naviPtr)) {
            log.error("loadMesh but not find navi = {}", naviPtr);
            return false;
        }
        return loadMeshNative(naviPtr, filePath, maxSearchNodes);
    }
        
    private native boolean loadDoorsNative(long ptr, String filePath);
    public boolean loadDoors(long naviPtr, String filePath) {
        if (naviPtr == 0) {
            log.error("loadDoors but navi is null");
            return false;
        }
        if (!createdNavis.containsKey(naviPtr)) {
            log.error("loadDoors but not find navi = {}", naviPtr);
            return false;
        }
        return loadDoorsNative(naviPtr, filePath);
    }

    private native boolean loadRegionsNative(long ptr, String filePath);
    public boolean loadRegions(long naviPtr, String filePath) {
        if (naviPtr == 0) {
            log.error("loadRegions but navi is null");
            return false;
        }
        if (!createdNavis.containsKey(naviPtr)) {
            log.error("loadRegions but not find navi = {}", naviPtr);
            return false;
        }
        return loadRegionsNative(naviPtr, filePath);
    }
    
    private native int getRegionIdNative(long ptr, float x, float z);
    public int getRegionId(long naviPtr, float x, float z) {
        if (naviPtr == 0) {
            log.error("getRegionId but navi is null");
            return 0;
        }
        if (!createdNavis.containsKey(naviPtr)) {
            log.error("getRegionId but not find navi = {}", naviPtr);
            return 0;
        }
        return getRegionIdNative(naviPtr, x, z);
    }

    private native void initDoorsPolyNative(long ptr);
    public void initDoorsPoly(long naviPtr) {
        if (naviPtr == 0) {
            log.error("initDoorsPoly but navi is null");
            return;
        }
        if (!createdNavis.containsKey(naviPtr)) {
            log.error("initDoorsPoly but not find navi = {}", naviPtr);
            return;
        }
        initDoorsPolyNative(naviPtr);
    }

    private native boolean isDoorExistNative(long ptr, int doorId);
    public boolean isDoorExist(long naviPtr, int doorId) {
        if (naviPtr == 0) {
            log.error("isDoorExist but navi is null");
            return false;
        }
        if (!createdNavis.containsKey(naviPtr)) {
            log.error("isDoorExist but not find navi = {}", naviPtr);
            return false;
        }
        return isDoorExistNative(naviPtr, doorId);
    }
        
    private native boolean isDoorOpenNative(long ptr, int doorId);
    public boolean isDoorOpen(long naviPtr, int doorId) {
        if (naviPtr == 0) {
            log.error("isDoorOpen but navi is null");
            return false;
        }
        if (!createdNavis.containsKey(naviPtr)) {
            log.error("isDoorOpen but not find navi = {}", naviPtr);
            return false;
        }
        return isDoorOpenNative(naviPtr, doorId);
    }

    private native void openDoorNative(long ptr, int doorId, boolean open);
    public void openDoor(long naviPtr, int doorId, boolean open) {
        if (naviPtr == 0) {
            log.error("openDoor but navi is null");
            return;
        }
        if (!createdNavis.containsKey(naviPtr)) {
            log.error("openDoor but not find navi = {}", naviPtr);
            return;
        }
        openDoorNative(naviPtr, doorId, open);
    }
        
    private native void openAllDoorsNative(long ptr, boolean open);
    public void openAllDoors(long naviPtr, boolean open) {
        if (naviPtr == 0) {
            log.error("openAllDoors but navi is null");
            return;
        }
        if (!createdNavis.containsKey(naviPtr)) {
            log.error("openAllDoors but not find navi = {}", naviPtr);
            return;
        }
        openAllDoorsNative(naviPtr, open);
    }

    private native void closeAllDoorsPolyNative(long ptr);
    public void closeAllDoorsPoly(long naviPtr) {
        if (naviPtr == 0) {
            log.error("closeAllDoorsPoly but navi is null");
            return;
        }
        if (!createdNavis.containsKey(naviPtr)) {
            log.error("closeAllDoorsPoly but not find navi = {}", naviPtr);
            return;
        }
        closeAllDoorsPolyNative(naviPtr);
    }

    private native void recoverAllDoorsPolyNative(long ptr);
    public void recoverAllDoorsPoly(long naviPtr) {
        if (naviPtr == 0) {
            log.error("recoverAllDoorsPoly but navi is null");
            return;
        }
        if (!createdNavis.containsKey(naviPtr)) {
            log.error("recoverAllDoorsPoly but not find navi = {}", naviPtr);
            return;
        }
        recoverAllDoorsPolyNative(naviPtr);
    }
        
    private native int addObstacleNative(long ptr,
         float posX, float posY, float posZ,
         float radius, float height);
    public int addObstacle(long naviPtr, float posX, float posY, float posZ,
         float radius, float height) {
        if (naviPtr == 0) {
            log.error("addObstacle but navi is null");
            return 0;
        }
        if (!createdNavis.containsKey(naviPtr)) {
            log.error("addObstacle but not find navi = {}", naviPtr);
            return 0;
        }
        return addObstacleNative(naviPtr, posX, posY, posZ, radius, height);
    }
    public int addObstacleOffset(long naviPtr, float posX, float posY, float posZ,
         float radius, float height) {
        if (naviPtr == 0) {
            log.error("addObstacleOffset but navi is null");
            return 0;
        }
        if (!createdNavis.containsKey(naviPtr)) {
            log.error("addObstacleOffset but not find navi = {}", naviPtr);
            return 0;
        }
        return addObstacleNative(naviPtr, posX, posY - 1.0f, posZ, radius, height + 1.0f);
    }
        
    private native boolean removeObstacleNative(long ptr, int obstacleRef);
    public boolean removeObstacle(long naviPtr, int obstacleRef) {
        if (naviPtr == 0) {
            log.error("removeObstacle but navi is null");
            return false;
        }
        if (!createdNavis.containsKey(naviPtr)) {
            log.error("removeObstacle but not find navi = {}", naviPtr);
            return false;
        }
        return removeObstacleNative(naviPtr, obstacleRef);
    }
        
    private native boolean refreshObstacleNative(long ptr);
    public boolean refreshObstacle(long naviPtr) {
        if (naviPtr == 0) {
            log.error("refreshObstacle but navi is null");
            return false;
        }
        if (!createdNavis.containsKey(naviPtr)) {
            log.error("refreshObstacle but not find navi = {}", naviPtr);
            return false;
        }
        return refreshObstacleNative(naviPtr);
    }

    private native int getMaxObstacleReqCountNative(long ptr);
    public int getMaxObstacleReqCount(long naviPtr) {
        if (naviPtr == 0) {
            log.error("getMaxObstacleReqCount but navi is null");
            return 0;
        }
        if (!createdNavis.containsKey(naviPtr)) {
            log.error("getMaxObstacleReqCount but not find navi = {}", naviPtr);
            return 0;
        }
        return getMaxObstacleReqCountNative(naviPtr);
    }

    private native int getAddedObstacleReqCountNative(long ptr);
    public int getAddedObstacleReqCount(long naviPtr) {
        if (naviPtr == 0) {
            log.error("getAddedObstacleReqCount but navi is null");
            return 0;
        }
        if (!createdNavis.containsKey(naviPtr)) {
            log.error("getAddedObstacleReqCount but not find navi = {}", naviPtr);
            return 0;
        }
        return getAddedObstacleReqCountNative(naviPtr);
    }

    private native int getObstacleReqRemainCountNative(long ptr);
    public int getObstacleReqRemainCount(long naviPtr) {
        if (naviPtr == 0) {
            log.error("getObstacleReqRemainCount but navi is null");
            return 0;
        }
        if (!createdNavis.containsKey(naviPtr)) {
            log.error("getObstacleReqRemainCount but not find navi = {}", naviPtr);
            return 0;
        }
        return getObstacleReqRemainCountNative(naviPtr);
    }

    private native int findPathNative(long ptr, float[] posArray, int arraySize, int[] posSize,
         float startX, float startY, float startZ,
         float endX, float endY, float endZ,
         float sizeX, float sizeY, float sizeZ);
    public int findPath(long naviPtr, float startX, float startY, float startZ,
         float endX, float endY, float endZ,
         float sizeX, float sizeY, float sizeZ) {
        if (naviPtr == 0) {
            log.error("findPath but navi is null");
            return FAILURE;
        }
        if (!createdNavis.containsKey(naviPtr)) {
            log.error("findPath but not find navi = {}", naviPtr);
            return FAILURE;
        }
        return findPathNative(naviPtr, posArray, MAX_SEARCH_POLYS, posSize, startX, startY, startZ,
            endX, endY, endZ, sizeX, sizeY, sizeZ);
    }
        
    private native int findPathDefaultNative(long ptr, float[] posArray, int arraySize, int[] posSize,
         float startX, float startY, float startZ,
         float endX, float endY, float endZ);
    public int findPath(long naviPtr, float startX, float startY, float startZ,
         float endX, float endY, float endZ) {
        if (naviPtr == 0) {
            log.error("findPath default but navi is null");
            return FAILURE;
        }
        if (!createdNavis.containsKey(naviPtr)) {
            log.error("findPath  default but not find navi = {}", naviPtr);
            return FAILURE;
        }
        return findPathDefaultNative(naviPtr, posArray, MAX_SEARCH_POLYS, posSize, startX, startY, startZ,
            endX, endY, endZ);
    }
}