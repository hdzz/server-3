#include <string.h>
#include <jni.h>


jint add(JNIEnv *env, jobject thiz, jint x, jint y){
	return x + y;
}

jint substraction(JNIEnv *env, jobject thiz, jint x, jint y){

	return x - y;
}


//C++�� native����
jstring native_hello(JNIEnv* env, jobject thiz) {
	return env->NewStringUTF("Hello jni");
}

/**
* JNINativeMethod�����������:(1)Java�еĺ�����;
(2)����ǩ��,��ʽΪ(�����������)����ֵ����;
(3)native������
*/
static JNINativeMethod gMethods[] = { { "getStringByJni", "()Ljava/lang/String;", (void *)native_hello } };

//JNI_OnLoadĬ�ϻ���System.loadLibrary�������Զ����õ�����������ô˺��������ж�̬ע��
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) 
{
	JNIEnv* env = NULL;
	jint result = JNI_FALSE;

	//��ȡenvָ��
	if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
		return result;
	}
	if (env == NULL) {
		return result;
	}
	//��ȡ������
	jclass clazz = env->FindClass("com/example/dynamicjni/MainActivity");
	if (clazz == NULL) {
		return result;
	}
	//ע�᷽��
	if (env->RegisterNatives(clazz, gMethods,
		sizeof(gMethods) / sizeof(gMethods[0])) < 0) {
		return result;
	}
	//�ɹ�
	result = JNI_VERSION_1_6;
	return result;
}