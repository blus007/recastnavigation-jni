#pragma once

extern void JniLog(int level, const char* format, ...);
#ifdef _WIN32
#define LOG_INFO(format, ...) {printf(format"\n", ## __VA_ARGS__); JniLog(0, format, ## __VA_ARGS__);}
#define LOG_WARN(format, ...) {printf(format"\n", ## __VA_ARGS__); JniLog(1, format, ## __VA_ARGS__);}
#define LOG_ERROR(format, ...) {printf(format"\n", ## __VA_ARGS__); JniLog(2, format, ## __VA_ARGS__);}
#else
#define LOG_INFO(format, ...) {JniLog(0, format, ## __VA_ARGS__);}
#define LOG_WARN(format, ...) {JniLog(1, format, ## __VA_ARGS__);}
#define LOG_ERROR(format, ...) {JniLog(2, format, ## __VA_ARGS__);}
#endif

#ifndef COMPILE_TEST
#define COMPILE_TEST(Name, Check) int __COMPILE_TEST_##Name##__[(Check) ? 1 : -1];
#endif

class JavaEnvIniter
{
public:
	JavaEnvIniter(void* env);
	~JavaEnvIniter();
};
#define JAVA_ENV_INIT(env) JavaEnvIniter __initer__(env)