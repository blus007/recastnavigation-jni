#include <jni.h>
#include "stdlib.h"
#include "cstring"
#include "DetourCommon.h"
#include "DetourNavMesh.h"
#include "DetourTileCache.h"
#include "Navi.h"

char* Jstring2String(JNIEnv* env, jstring jstr)
{
	char* str = nullptr;
	jclass jstringClass = env->FindClass("java/lang/String");
	jstring stringCode = env->NewStringUTF("utf-8");
	jmethodID methodID = env->GetMethodID(jstringClass, "getBytes", "(Ljava/lang/String;)[B");
	jbyteArray byteArray = (jbyteArray)env->CallObjectMethod(jstr, methodID, stringCode);
	jsize byteLength = env->GetArrayLength(byteArray);
	jbyte* bytes = env->GetByteArrayElements(byteArray, JNI_FALSE);

	if (byteLength > 0)
	{
		str = (char*)malloc(byteLength + 1);
		memcpy(str, bytes, byteLength);
		str[byteLength] = 0;
	}
	env->ReleaseByteArrayElements(byteArray, bytes, 0);
	return str;
}

jstring String2Jstring(JNIEnv* env, const char* str)
{
	int strSize = strlen(str);
	jclass jstringClass = env->FindClass("Ljava/lang/String;");
	jmethodID methodID = env->GetMethodID(jstringClass, "<init>", "([BLjava/lang/String;)V");
	jbyteArray byteArray = env->NewByteArray(strSize);
	env->SetByteArrayRegion(byteArray, 0, strSize, (jbyte*)str);
	jstring stringCode = env->NewStringUTF("utf-8");
	return (jstring)env->NewObject(jstringClass, methodID, byteArray, stringCode);
}

inline long Ptr2Long(void* ptr)
{
    long l = 0;
    memcpy(&l, &ptr, sizeof(void*));
    return l;
}

inline void* Long2Ptr(long l)
{
    void* ptr = 0;
    memcpy(&ptr, &l, sizeof(void*));
    return ptr;
}

#ifdef __cplusplus
extern "C" {
#endif
    
JNIEXPORT jlong JNICALL Java_org_navi_Navi_createNative
    (JNIEnv *env, jobject self)
{
    Navi* navi = new Navi;
    jlong ptr = Ptr2Long(navi);
    return ptr;
}
    
JNIEXPORT void JNICALL Java_org_navi_Navi_destroyNative
    (JNIEnv *env, jobject self, jlong ptr)
{
    if (!ptr)
        return;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    delete navi;
}
    
JNIEXPORT void JNICALL Java_org_navi_Navi_setDefaultPolySizeNative
    (JNIEnv *env, jobject self, jlong ptr, jfloat x, jfloat y, jfloat z)
{
    if (!ptr)
        return;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    navi->SetDefaultPolySize(x, y, z);
}

JNIEXPORT jboolean JNICALL Java_org_navi_Navi_loadMeshNative
  (JNIEnv *env, jobject self, jlong ptr, jstring filePath)
{
    if (!ptr)
        return false;
    Navi* navi = (Navi*)Long2Ptr(ptr);
	char* path = Jstring2String(env, filePath);
    printf("load mesh native:%s\n", path);
    jboolean success = navi->LoadMesh(path);
	free(path);
	return success;
}
    
JNIEXPORT jboolean JNICALL Java_org_navi_Navi_loadDoorsNative
    (JNIEnv *env, jobject self, jlong ptr, jstring filePath)
{
    if (!ptr)
        return false;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    char* path = Jstring2String(env, filePath);
    printf("load doors native:%s\n", path);
    jboolean success = navi->LoadDoors(path);
    free(path);
    return success;
}
    
JNIEXPORT void JNICALL Java_org_navi_Navi_initDoorsPolyNative
    (JNIEnv *env, jobject self, jlong ptr)
{
    if (!ptr)
        return;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    navi->InitDoorsPoly();
}

JNIEXPORT jboolean JNICALL Java_org_navi_Navi_isDoorExistNative
    (JNIEnv *env, jobject self, jlong ptr, jint doorId)
{
    if (!ptr)
        return false;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    return navi->IsDoorExist(doorId);
}
    
JNIEXPORT jboolean JNICALL Java_org_navi_Navi_isDoorOpenNative
    (JNIEnv *env, jobject self, jlong ptr, jint doorId)
{
    if (!ptr)
        return false;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    return navi->IsDoorOpen(doorId);
}

JNIEXPORT void JNICALL Java_org_navi_Navi_openDoorNative
    (JNIEnv *env, jobject self, jlong ptr, jint doorId, jboolean open)
{
    if (!ptr)
        return;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    navi->OpenDoor(doorId, open);
}
    
JNIEXPORT void JNICALL Java_org_navi_Navi_openAllDoorsNative
    (JNIEnv *env, jobject self, jlong ptr, jboolean open)
{
    if (!ptr)
        return;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    navi->OpenAllDoors(open);
}
    
JNIEXPORT void JNICALL Java_org_navi_Navi_closeAllDoorsPolyNative
    (JNIEnv *env, jobject self, jlong ptr)
{
    if (!ptr)
        return;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    navi->CloseAllDoorsPoly();
}
    
JNIEXPORT void JNICALL Java_org_navi_Navi_recoverAllDoorsPolyNative
    (JNIEnv *env, jobject self, jlong ptr)
{
    if (!ptr)
        return;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    navi->RecoverAllDoorsPoly();
}
    
JNIEXPORT jint JNICALL Java_org_navi_Navi_addObstacleNative
    (JNIEnv *env, jobject self, jlong ptr,
     jfloat posX, jfloat posY, jfloat posZ,
     jfloat radius, jfloat height)
{
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
    (JNIEnv *env, jobject self, jlong ptr, jint obstacleRef)
{
    if (!ptr)
        return false;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    int result = navi->RemoveObstacle(obstacleRef);
    if (!dtStatusSucceed(result))
        return false;
    return true;
}
    
JNIEXPORT jboolean JNICALL Java_org_navi_Navi_refreshObstacleNative
    (JNIEnv *env, jobject self, jlong ptr)
{
    if (!ptr)
        return false;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    int result = navi->RefreshObstacle();
    if (!dtStatusSucceed(result))
        return false;
    return true;
}
    
JNIEXPORT jint JNICALL Java_org_navi_Navi_getMaxObstacleReqCountNative
    (JNIEnv *env, jobject self, jlong ptr)
{
    if (!ptr)
        return 0;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    return navi->GetMaxObstacleReqCount();
}

JNIEXPORT jint JNICALL Java_org_navi_Navi_getAddedObstacleReqCountNative
    (JNIEnv *env, jobject self, jlong ptr)
{
    if (!ptr)
        return 0;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    return navi->GetAddedObstacleReqCount();
}

JNIEXPORT jint JNICALL Java_org_navi_Navi_getObstacleReqRemainCountNative
    (JNIEnv *env, jobject self, jlong ptr)
{
    if (!ptr)
        return 0;
    Navi* navi = (Navi*)Long2Ptr(ptr);
    return navi->GetObstacleReqRemainCount();
}

JNIEXPORT jint JNICALL Java_org_navi_Navi_findPathNative
    (JNIEnv *env, jobject self, jlong ptr, jfloatArray posArray, jintArray posSize,
     jfloat startX, jfloat startY, jfloat startZ,
     jfloat endX, jfloat endY, jfloat endZ,
     jfloat sizeX, jfloat sizeY, jfloat sizeZ)
{
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
    env->SetFloatArrayRegion(posArray, 0, pathCount * 3, (const jfloat*)path);
    env->SetIntArrayRegion(posSize, 0, 1, (const jint*)&pathCount);
    
    return result;
}
    
JNIEXPORT jint JNICALL Java_org_navi_Navi_findPathDefaultNative
    (JNIEnv *env, jobject self, jlong ptr, jfloatArray posArray, jintArray posSize,
     jfloat startX, jfloat startY, jfloat startZ,
     jfloat endX, jfloat endY, jfloat endZ)
{
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
    env->SetFloatArrayRegion(posArray, 0, pathCount * 3, (const jfloat*)path);
    env->SetIntArrayRegion(posSize, 0, 1, (const jint*)&pathCount);
    
    return result;
}

#ifdef __cplusplus
}
#endif
