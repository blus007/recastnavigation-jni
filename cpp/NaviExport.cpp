#include <jni.h>
#include "stdlib.h"
#include "cstring"
#include "DetourCommon.h"
#include "DetourNavMesh.h"
#include "Navi.h"

static char* jstring2String(JNIEnv* env, jstring jstr)
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

static jstring char2Jstring(JNIEnv* env, const char* str)
{
	int strSize = strlen(str);
	jclass jstringClass = env->FindClass("Ljava/lang/String;");
	jmethodID methodID = env->GetMethodID(jstringClass, "<init>", "([BLjava/lang/String;)V");
	jbyteArray byteArray = env->NewByteArray(strSize);
	env->SetByteArrayRegion(byteArray, 0, strSize, (jbyte*)str);
	jstring stringCode = env->NewStringUTF("utf-8");
	return (jstring)env->NewObject(jstringClass, methodID, byteArray, stringCode);
}

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jstring JNICALL Java_org_navi_Navi_loadMesh
  (JNIEnv *env, jobject self, jstring filePath)
{
    printf("begin\n");
//    Navi navi;
	char* str = jstring2String(env, filePath);
	printf("%s\n", str);
	free(str);
	const char* test = "this is test";
	jstring jstr = char2Jstring(env, test);
	return jstr;
}

#ifdef __cplusplus
}
#endif
