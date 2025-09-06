#include <jni.h>
#include "Util.h"

thread_local JNIEnv* tlEnv = nullptr;
thread_local int tlEnterCount = 0;

JavaEnvIniter::JavaEnvIniter(void* env)
{
	if (tlEnterCount > 0 && tlEnv != env)
	{
		LOG_ERROR("different env in one thread [%p] <=> [%p]", tlEnv, env);
	}
	++tlEnterCount;
	if (tlEnv)
		return;
	tlEnv = (JNIEnv*)env;
}

JavaEnvIniter::~JavaEnvIniter()
{
	--tlEnterCount;
	if (tlEnterCount <= 0)
		tlEnv = nullptr;
}

void JniLog(int level, const char* format, ...)
{
	if (!tlEnv)
		return;

	JNIEnv* env = tlEnv;
	jclass naviClass = env->FindClass("org/navi/Navi");
	if (naviClass == nullptr)
		return;
	jmethodID methodID = env->GetStaticMethodID(naviClass, "jniLog", "(ILjava/lang/String;)V");
	if (!methodID)
		return;

	const int bufferSize = 2048;
	char buffer[bufferSize];
	va_list vl;
	va_start(vl, format);
	vsnprintf(buffer, bufferSize, format, vl);
	va_end(vl);

	jstring message = env->NewStringUTF(buffer);
	env->CallStaticVoidMethod(naviClass, methodID, level, message);
}