#include <string.h>
#include <jni.h>


jint add(JNIEnv *env, jobject thiz, jint x, jint y){
	return x + y;
}

jint substraction(JNIEnv *env, jobject thiz, jint x, jint y){

	return x - y;
}


//C++层 native函数
jstring native_hello(JNIEnv* env, jobject thiz) {
	return env->NewStringUTF("Hello jni");
}

/**
* JNINativeMethod由三部分组成:(1)Java中的函数名;
(2)函数签名,格式为(输入参数类型)返回值类型;
(3)native函数名
*/
static JNINativeMethod gMethods[] = { { "getStringByJni", "()Ljava/lang/String;", (void *)native_hello } };

//JNI_OnLoad默认会在System.loadLibrary过程中自动调用到，因而可利用此函数，进行动态注册
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) 
{
	JNIEnv* env = NULL;
	jint result = JNI_FALSE;

	//获取env指针
	if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
		return result;
	}
	if (env == NULL) {
		return result;
	}
	//获取类引用
	jclass clazz = env->FindClass("com/example/dynamicjni/MainActivity");
	if (clazz == NULL) {
		return result;
	}
	//注册方法
	if (env->RegisterNatives(clazz, gMethods,
		sizeof(gMethods) / sizeof(gMethods[0])) < 0) {
		return result;
	}
	//成功
	result = JNI_VERSION_1_6;
	return result;
}