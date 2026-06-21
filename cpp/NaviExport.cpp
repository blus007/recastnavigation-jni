#include <jni.h>
#include "stdlib.h"
#include "cstring"
#include "DetourCommon.h"
#include "DetourNavMesh.h"
#include "DetourTileCache.h"
#include "Util.h"
#include "Navi.h"

void JniClearPendingException(JNIEnv* env);
void JniDeleteLocalRef(JNIEnv* env, jobject ref);

COMPILE_TEST(JLONG_SIZE, sizeof(jlong) == 8);

char* Jstring2String(JNIEnv* env, jstring jstr)
{
	if (!env || !jstr)
		return nullptr;

	char* str = nullptr;
	jbyte* bytes = nullptr;
	jbyteArray byteArray = nullptr;

	jclass jstringClass = env->FindClass("java/lang/String");
	if (!jstringClass || env->ExceptionCheck())
	{
		JniDeleteLocalRef(env, jstringClass);
		JniClearPendingException(env);
		return nullptr;
	}

	jstring stringCode = env->NewStringUTF("utf-8");
	if (!stringCode || env->ExceptionCheck())
	{
		JniDeleteLocalRef(env, stringCode);
		JniDeleteLocalRef(env, jstringClass);
		JniClearPendingException(env);
		return nullptr;
	}

	jmethodID methodID = env->GetMethodID(jstringClass, "getBytes", "(Ljava/lang/String;)[B");
	if (!methodID || env->ExceptionCheck())
	{
		JniDeleteLocalRef(env, stringCode);
		JniDeleteLocalRef(env, jstringClass);
		JniClearPendingException(env);
		return nullptr;
	}

	byteArray = (jbyteArray)env->CallObjectMethod(jstr, methodID, stringCode);
	if (!byteArray || env->ExceptionCheck())
	{
		JniDeleteLocalRef(env, byteArray);
		JniDeleteLocalRef(env, stringCode);
		JniDeleteLocalRef(env, jstringClass);
		JniClearPendingException(env);
		return nullptr;
	}

	const jsize byteLength = env->GetArrayLength(byteArray);
	bytes = env->GetByteArrayElements(byteArray, JNI_FALSE);
	if (!bytes || env->ExceptionCheck())
	{
		JniDeleteLocalRef(env, byteArray);
		JniDeleteLocalRef(env, stringCode);
		JniDeleteLocalRef(env, jstringClass);
		JniClearPendingException(env);
		return nullptr;
	}

	str = (char*)malloc(byteLength + 1);
	if (str)
	{
		if (byteLength > 0)
			memcpy(str, bytes, byteLength);
		str[byteLength] = 0;
	}

	env->ReleaseByteArrayElements(byteArray, bytes, JNI_ABORT);
	JniDeleteLocalRef(env, byteArray);
	JniDeleteLocalRef(env, stringCode);
	JniDeleteLocalRef(env, jstringClass);
	JniClearPendingException(env);
	return str;
}

jstring String2Jstring(JNIEnv* env, const char* str)
{
	if (!env || !str)
		return nullptr;

	const int strSize = (int)strlen(str);

	jclass jstringClass = env->FindClass("java/lang/String");
	if (!jstringClass || env->ExceptionCheck())
	{
		JniDeleteLocalRef(env, jstringClass);
		JniClearPendingException(env);
		return nullptr;
	}

	jmethodID methodID = env->GetMethodID(jstringClass, "<init>", "([BLjava/lang/String;)V");
	if (!methodID || env->ExceptionCheck())
	{
		JniDeleteLocalRef(env, jstringClass);
		JniClearPendingException(env);
		return nullptr;
	}

	jbyteArray byteArray = env->NewByteArray(strSize);
	if (!byteArray || env->ExceptionCheck())
	{
		JniDeleteLocalRef(env, byteArray);
		JniDeleteLocalRef(env, jstringClass);
		JniClearPendingException(env);
		return nullptr;
	}

	env->SetByteArrayRegion(byteArray, 0, strSize, (const jbyte*)str);
	if (env->ExceptionCheck())
	{
		JniDeleteLocalRef(env, byteArray);
		JniDeleteLocalRef(env, jstringClass);
		JniClearPendingException(env);
		return nullptr;
	}

	jstring stringCode = env->NewStringUTF("utf-8");
	if (!stringCode || env->ExceptionCheck())
	{
		JniDeleteLocalRef(env, stringCode);
		JniDeleteLocalRef(env, byteArray);
		JniDeleteLocalRef(env, jstringClass);
		JniClearPendingException(env);
		return nullptr;
	}

	jstring result = (jstring)env->NewObject(jstringClass, methodID, byteArray, stringCode);
	if (env->ExceptionCheck())
	{
		JniDeleteLocalRef(env, result);
		JniDeleteLocalRef(env, stringCode);
		JniDeleteLocalRef(env, byteArray);
		JniDeleteLocalRef(env, jstringClass);
		JniClearPendingException(env);
		return nullptr;
	}

	JniDeleteLocalRef(env, stringCode);
	JniDeleteLocalRef(env, byteArray);
	JniDeleteLocalRef(env, jstringClass);
	JniClearPendingException(env);
	return result;
}

inline long long Ptr2Long(void* ptr)
{
    long long l = 0;
    const int copySize = sizeof(void*);
    memcpy(&l, &ptr, copySize);
    return l;
}

inline void* Long2Ptr(long long l)
{
    void* ptr = 0;
    const int copySize = sizeof(void*);
    memcpy(&ptr, &l, copySize);
    return ptr;
}

#ifdef __cplusplus
extern "C" {
#endif
    
JNIEXPORT jint JNICALL Java_org_navi_Navi_getMaxPosSizeNative
    (JNIEnv *env, jobject obj)
{
    JAVA_ENV_INIT(env);
    //printf("Java_org_navi_Navi_getMaxPosSizeNative:env=%p obj=%p\n", env, obj);
    return MAX_SEARCH_POLYS;
}
    
JNIEXPORT jlong JNICALL Java_org_navi_Navi_createNative
    (JNIEnv *env, jobject obj, jint maxPoly, jint maxObstacle)
{
    JAVA_ENV_INIT(env);
    //printf("Java_org_navi_Navi_createNative:env=%p obj=%p\n", env, obj);
    Navi* navi = new Navi(maxPoly, maxObstacle);
    jlong ptr = Ptr2Long(navi);
    return ptr;
}
    
JNIEXPORT void JNICALL Java_org_navi_Navi_destroyNative
    (JNIEnv *env, jobject obj, jlong ptr)
{
    JAVA_ENV_INIT(env);
    //printf("Java_org_navi_Navi_destroyNative:env=%p obj=%p\n", env, obj);
    if (!ptr)
        return;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    delete navi;
}
    
JNIEXPORT void JNICALL Java_org_navi_Navi_setDefaultPolySizeNative
    (JNIEnv *env, jobject obj, jlong ptr, jfloat x, jfloat y, jfloat z)
{
    JAVA_ENV_INIT(env);
    //printf("Java_org_navi_Navi_destroyNative:env=%p obj=%p\n", env, obj);
    if (!ptr)
        return;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    navi->SetDefaultPolySize(x, y, z);
}

JNIEXPORT jboolean JNICALL Java_org_navi_Navi_loadMeshNative
  (JNIEnv *env, jobject obj, jlong ptr, jstring filePath, jint maxSearchNodes)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return false;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    if (!filePath)
        return false;
	char* path = Jstring2String(env, filePath);
    if (!path)
        return false;
    printf("load mesh native:%s\n", path);
    jboolean success = navi->LoadMesh(path, maxSearchNodes);
	free(path);
	return success;
}
    
JNIEXPORT jboolean JNICALL Java_org_navi_Navi_loadDoorsNative
    (JNIEnv *env, jobject obj, jlong ptr, jstring filePath)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return false;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    if (!filePath)
        return false;
    char* path = Jstring2String(env, filePath);
    if (!path)
        return false;
    printf("load doors native:%s\n", path);
    jboolean success = navi->LoadDoors(path);
    free(path);
    return success;
}
    
JNIEXPORT jboolean JNICALL Java_org_navi_Navi_loadRegionsNative
    (JNIEnv *env, jobject obj, jlong ptr, jstring filePath)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return false;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    if (!filePath)
        return false;
    char* path = Jstring2String(env, filePath);
    if (!path)
        return false;
    printf("load regions native:%s\n", path);
    jboolean success = navi->LoadRegions(path);
    free(path);
    return success;
}
    
JNIEXPORT jint JNICALL Java_org_navi_Navi_getRegionIdNative
    (JNIEnv *env, jobject obj, jlong ptr, jfloat x, jfloat z)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return 0;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    Vector3 pos(x, 0, z);
    return navi->GetRegionId(pos);
}
    
JNIEXPORT void JNICALL Java_org_navi_Navi_initDoorsPolyNative
    (JNIEnv *env, jobject obj, jlong ptr)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    navi->InitDoorsPoly();
}

JNIEXPORT jboolean JNICALL Java_org_navi_Navi_isDoorExistNative
    (JNIEnv *env, jobject obj, jlong ptr, jint doorId)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return false;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    return navi->IsDoorExist(doorId);
}
    
JNIEXPORT jboolean JNICALL Java_org_navi_Navi_isDoorOpenNative
    (JNIEnv *env, jobject obj, jlong ptr, jint doorId)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return false;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    return navi->IsDoorOpen(doorId);
}

JNIEXPORT void JNICALL Java_org_navi_Navi_openDoorNative
    (JNIEnv *env, jobject obj, jlong ptr, jint doorId, jboolean open)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    navi->OpenDoor(doorId, open);
}
    
JNIEXPORT void JNICALL Java_org_navi_Navi_openAllDoorsNative
    (JNIEnv *env, jobject obj, jlong ptr, jboolean open)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    navi->OpenAllDoors(open);
}
    
JNIEXPORT void JNICALL Java_org_navi_Navi_closeAllDoorsPolyNative
    (JNIEnv *env, jobject obj, jlong ptr)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    navi->CloseAllDoorsPoly();
}
    
JNIEXPORT void JNICALL Java_org_navi_Navi_recoverAllDoorsPolyNative
    (JNIEnv *env, jobject obj, jlong ptr)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    navi->RecoverAllDoorsPoly();
}
    
JNIEXPORT jint JNICALL Java_org_navi_Navi_addObstacleNative
    (JNIEnv *env, jobject obj, jlong ptr,
     jfloat posX, jfloat posY, jfloat posZ,
     jfloat radius, jfloat height)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return 0;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    Vector3 pos(posX, posY, posZ);
    dtObstacleRef ref = 0;
    int result = navi->AddObstacle(pos, radius, height, &ref);
    if (!dtStatusSucceed(result))
        return 0;
    return ref;
}
    
JNIEXPORT jboolean JNICALL Java_org_navi_Navi_removeObstacleNative
    (JNIEnv *env, jobject obj, jlong ptr, jint obstacleRef)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return false;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    int result = navi->RemoveObstacle(obstacleRef);
    if (!dtStatusSucceed(result))
        return false;
    return true;
}
    
JNIEXPORT jboolean JNICALL Java_org_navi_Navi_refreshObstacleNative
    (JNIEnv *env, jobject obj, jlong ptr)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return false;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    int result = navi->RefreshObstacle();
    if (!dtStatusSucceed(result))
        return false;
    return true;
}
    
JNIEXPORT jint JNICALL Java_org_navi_Navi_getMaxObstacleReqCountNative
    (JNIEnv *env, jobject obj, jlong ptr)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return 0;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    return navi->GetMaxObstacleReqCount();
}

JNIEXPORT jint JNICALL Java_org_navi_Navi_getAddedObstacleReqCountNative
    (JNIEnv *env, jobject obj, jlong ptr)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return 0;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    return navi->GetAddedObstacleReqCount();
}

JNIEXPORT jint JNICALL Java_org_navi_Navi_getObstacleReqRemainCountNative
    (JNIEnv *env, jobject obj, jlong ptr)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return 0;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    return navi->GetObstacleReqRemainCount();
}

JNIEXPORT jint JNICALL Java_org_navi_Navi_findPathNative
    (JNIEnv *env, jobject obj, jlong ptr, jfloatArray posArray, jint arraySize,
     jintArray posSize, jfloat startX, jfloat startY, jfloat startZ,
     jfloat endX, jfloat endY, jfloat endZ, jfloat sizeX, jfloat sizeY, jfloat sizeZ)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return DT_FAILURE;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    Vector3 start(startX, startY, startZ);
    Vector3 end(endX, endY, endZ);
    Vector3 size(sizeX, sizeY, sizeZ);
    int result = navi->FindPath(start, end, size);
    if (!dtStatusSucceed(result))
        return result;
    
    const int pathCount = navi->GetPathCount();
    const Vector3* path = navi->GetPath();
    const int maxCount = pathCount < arraySize ? pathCount : arraySize;
    env->SetFloatArrayRegion(posArray, 0, maxCount * 3, (const jfloat*)path);
    env->SetIntArrayRegion(posSize, 0, 1, (const jint*)&maxCount);
    
    return result;
}
    
JNIEXPORT jint JNICALL Java_org_navi_Navi_findPathDefaultNative
    (JNIEnv *env, jobject obj, jlong ptr, jfloatArray posArray, jint arraySize,
     jintArray posSize, jfloat startX, jfloat startY, jfloat startZ,
     jfloat endX, jfloat endY, jfloat endZ)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return DT_FAILURE;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    Vector3 start(startX, startY, startZ);
    Vector3 end(endX, endY, endZ);
    int result = navi->FindPath(start, end);
    if (!dtStatusSucceed(result))
        return result;
    
    const int pathCount = navi->GetPathCount();
    const Vector3* path = navi->GetPath();
    const int maxCount = pathCount < arraySize ? pathCount : arraySize;
    env->SetFloatArrayRegion(posArray, 0, maxCount * 3, (const jfloat*)path);
    env->SetIntArrayRegion(posSize, 0, 1, (const jint*)&maxCount);
    
    return result;
}

JNIEXPORT jint JNICALL Java_org_navi_Navi_makePathStraightNative
(JNIEnv* env, jobject obj, jlong ptr, jfloatArray posArray, jint arraySize,
    jfloat sizeX, jfloat sizeY, jfloat sizeZ)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return arraySize;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    Vector3 size(sizeX, sizeY, sizeZ);
    Vector3* path = (Vector3*)navi->GetPath();
    env->GetFloatArrayRegion(posArray, 0, arraySize * 3, (jfloat*)path);
    int pathCount = arraySize;
    navi->MakePathStraight(pathCount, (float*)path, size);
    if (pathCount < arraySize)
        env->SetFloatArrayRegion(posArray, 0, pathCount * 3, (const jfloat*)path);

    return pathCount;
}

JNIEXPORT jint JNICALL Java_org_navi_Navi_makePathStraightDefaultNative
(JNIEnv* env, jobject obj, jlong ptr, jfloatArray posArray, jint arraySize)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return arraySize;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    Vector3* path = (Vector3*)navi->GetPath();
    env->GetFloatArrayRegion(posArray, 0, arraySize * 3, (jfloat*)path);
    int pathCount = arraySize;
    navi->MakePathStraight(pathCount, (float*)path);
    if (pathCount < arraySize)
        env->SetFloatArrayRegion(posArray, 0, pathCount * 3, (const jfloat*)path);

    return pathCount;
}

JNIEXPORT jfloat JNICALL Java_org_navi_Navi_pathRaycastNative
(JNIEnv* env, jobject obj, jlong ptr, jfloat startX, jfloat startY, jfloat startZ,
    jfloat endX, jfloat endY, jfloat endZ, jfloat sizeX, jfloat sizeY, jfloat sizeZ)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return -1.0f;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    Vector3 start(startX, startY, startZ);
    Vector3 end(endX, endY, endZ);
    Vector3 size(sizeX, sizeY, sizeZ);
    return navi->PathRaycast(start, end, size);
}

JNIEXPORT jfloat JNICALL Java_org_navi_Navi_pathRaycastDefaultNative
(JNIEnv* env, jobject obj, jlong ptr, jfloat startX, jfloat startY, jfloat startZ,
    jfloat endX, jfloat endY, jfloat endZ)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return -1.0f;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    Vector3 start(startX, startY, startZ);
    Vector3 end(endX, endY, endZ);
    return navi->PathRaycast(start, end);
}

JNIEXPORT jboolean JNICALL Java_org_navi_Navi_canPathForwardNative
(JNIEnv* env, jobject obj, jlong ptr, jfloat startX, jfloat startY, jfloat startZ,
    jfloat endX, jfloat endY, jfloat endZ, jfloat sizeX, jfloat sizeY, jfloat sizeZ)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return false;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    Vector3 start(startX, startY, startZ);
    Vector3 end(endX, endY, endZ);
    Vector3 size(sizeX, sizeY, sizeZ);
    return navi->CanPathForward(start, end, size);
}

JNIEXPORT jboolean JNICALL Java_org_navi_Navi_canPathForwardDefaultNative
(JNIEnv* env, jobject obj, jlong ptr, jfloat startX, jfloat startY, jfloat startZ,
    jfloat endX, jfloat endY, jfloat endZ)
{
    JAVA_ENV_INIT(env);
    if (!ptr)
        return false;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    Vector3 start(startX, startY, startZ);
    Vector3 end(endX, endY, endZ);
    return navi->CanPathForward(start, end);
}

#ifdef __cplusplus
}
#endif
