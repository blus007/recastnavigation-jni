#include <jni.h>
#include "Util.h"

thread_local JNIEnv* tlEnv = nullptr;
thread_local int tlEnterCount = 0;

void JniClearPendingException(JNIEnv* env)
{
	if (env && env->ExceptionCheck())
		env->ExceptionClear();
}

void JniDeleteLocalRef(JNIEnv* env, jobject ref)
{
	if (env && ref)
		env->DeleteLocalRef(ref);
}

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
	if (!tlEnv || !format)
		return;

	JNIEnv* env = tlEnv;

	jclass naviClass = env->FindClass("org/navi/Navi");
	if (!naviClass || env->ExceptionCheck())
	{
		JniDeleteLocalRef(env, naviClass);
		JniClearPendingException(env);
		return;
	}

	jmethodID methodID = env->GetStaticMethodID(naviClass, "jniLog", "(ILjava/lang/String;)V");
	if (!methodID || env->ExceptionCheck())
	{
		JniDeleteLocalRef(env, naviClass);
		JniClearPendingException(env);
		return;
	}

	const int bufferSize = 2048;
	char buffer[bufferSize];
	va_list vl;
	va_start(vl, format);
	vsnprintf(buffer, bufferSize, format, vl);
	va_end(vl);

	jstring message = env->NewStringUTF(buffer);
	if (!message || env->ExceptionCheck())
	{
		JniDeleteLocalRef(env, message);
		JniDeleteLocalRef(env, naviClass);
		JniClearPendingException(env);
		return;
	}

	env->CallStaticVoidMethod(naviClass, methodID, level, message);

	JniDeleteLocalRef(env, message);
	JniDeleteLocalRef(env, naviClass);
	JniClearPendingException(env);
}