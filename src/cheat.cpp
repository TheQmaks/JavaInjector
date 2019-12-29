#include <windows.h>
#include "jni.h"
#include "utils.h"

jstring getZipCommentFromBuffer(JNIEnv *env, jbyteArray buffer) {
	byte endOfDirectoryFlag[] = { 0x50, 0x4b, 0x05, 0x06 };
	int endLength = sizeof(endOfDirectoryFlag) / sizeof(byte);
	int bufferLength = env->GetArrayLength(buffer);
	jbyte *byteBuffer = env->GetByteArrayElements(buffer, false);

	for (int i = bufferLength - endLength - 22; i >= 0; i--) {
		boolean isEndOfDirectoryFlag = true;
		for (int k = 0; k < endLength; k++) {
			if (byteBuffer[i + k] != endOfDirectoryFlag[k]) {
				isEndOfDirectoryFlag = false;
				break;
			}
		}
		if (isEndOfDirectoryFlag) {
			int commentLen = byteBuffer[i + 20] + byteBuffer[i + 22] * 256;
			int realLen = bufferLength - i - 22;
			jclass String = env->FindClass("java/lang/String");
			jmethodID Init = env->GetMethodID(String, "<init>", "([BII)V");
			return (jstring)env->NewObject(String, Init, buffer, i + 22, min(commentLen, realLen));
		}
	}

	return NULL;
}

typedef jobjectArray(JNICALL *JVM_GetAllThreads)(JNIEnv *env, jclass dummy);

void cheat(JNIEnv *jniEnv) {
	jclass fileChooserCls = jniEnv->FindClass("javax/swing/JFileChooser");
	jmethodID fileChooserInit = jniEnv->GetMethodID(fileChooserCls, "<init>", "()V");
	jobject fileChooser = jniEnv->NewObject(fileChooserCls, fileChooserInit);

	jmethodID setDialogTitle = jniEnv->GetMethodID(fileChooserCls, "setDialogTitle", "(Ljava/lang/String;)V");
	jniEnv->CallVoidMethod(fileChooser, setDialogTitle, jniEnv->NewStringUTF("Select target file"));

	jmethodID setAcceptAllFileFilterUsed = jniEnv->GetMethodID(fileChooserCls, "setAcceptAllFileFilterUsed", "(Z)V");
	jniEnv->CallVoidMethod(fileChooser, setAcceptAllFileFilterUsed, false);

	jclass String = jniEnv->FindClass("java/lang/String");
	jobjectArray extensions = jniEnv->NewObjectArray(2, String, false);
	jniEnv->SetObjectArrayElement(extensions, 0, jniEnv->NewStringUTF("zip"));
	jniEnv->SetObjectArrayElement(extensions, 1, jniEnv->NewStringUTF("jar"));

	jclass extFilterCls = jniEnv->FindClass("javax/swing/filechooser/FileNameExtensionFilter");
	jmethodID extFilterInit = jniEnv->GetMethodID(extFilterCls, "<init>", "(Ljava/lang/String;[Ljava/lang/String;)V");
	jobject filter = jniEnv->NewObject(extFilterCls, extFilterInit, jniEnv->NewStringUTF("ZIP or JAR file"), extensions);

	jmethodID addChoosableFileFilter = jniEnv->GetMethodID(fileChooserCls, "addChoosableFileFilter", "(Ljavax/swing/filechooser/FileFilter;)V");
	jniEnv->CallVoidMethod(fileChooser, addChoosableFileFilter, filter);

	jclass fileCls = jniEnv->FindClass("java/io/File");
	jmethodID initFileWithString = jniEnv->GetMethodID(fileCls, "<init>", "(Ljava/lang/String;)V");
	jmethodID initFileWithTwoStrings = jniEnv->GetMethodID(fileCls, "<init>", "(Ljava/lang/String;Ljava/lang/String;)V");
	jmethodID getParent = jniEnv->GetMethodID(fileCls, "getParent", "()Ljava/lang/String;");
	jmethodID getAbsolutePath = jniEnv->GetMethodID(fileCls, "getAbsolutePath", "()Ljava/lang/String;");

	jmethodID setCurrentDirectory = jniEnv->GetMethodID(fileChooserCls, "setCurrentDirectory", "(Ljava/io/File;)V");
	jclass System = jniEnv->FindClass("java/lang/System");
	jmethodID getProperty = jniEnv->GetStaticMethodID(System, "getProperty", "(Ljava/lang/String;)Ljava/lang/String;");
	jobject Desktop = jniEnv->NewObject(fileCls, initFileWithTwoStrings, (jstring) jniEnv->CallStaticObjectMethod(System, getProperty, jniEnv->NewStringUTF("user.home")), jniEnv->NewStringUTF("Desktop"));
	jniEnv->CallVoidMethod(fileChooser, setCurrentDirectory, Desktop);

	jmethodID showDialog = jniEnv->GetMethodID(fileChooserCls, "showDialog", "(Ljava/awt/Component;Ljava/lang/String;)I");

	TCHAR tempPath[MAX_PATH];
	HMODULE hm = NULL;

	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPWSTR)&cheat, &hm);
	GetModuleFileName(hm, tempPath, sizeof(tempPath));

	char cPath[MAX_PATH];
	wcstombs(cPath, tempPath, wcslen(tempPath) + 1);

	jstring jPath = jniEnv->NewStringUTF(cPath);
	jobject dllFile = jniEnv->NewObject(fileCls, initFileWithString, jPath);
	jobject file = jniEnv->NewObject(fileCls, initFileWithTwoStrings, jniEnv->CallObjectMethod(dllFile, getParent), jniEnv->NewStringUTF("cheat.zip"));
	jstring comment = NULL;
	do {
		if (!file) {
			jint result = jniEnv->CallIntMethod(fileChooser, showDialog, NULL, jniEnv->NewStringUTF("Inject"));

			//result == JFileChooser.APPROVE_OPTION
			if (result == 0) {
				jmethodID getSelectedFile = jniEnv->GetMethodID(fileChooserCls, "getSelectedFile", "()Ljava/io/File;");
				file = jniEnv->CallObjectMethod(fileChooser, getSelectedFile);
			} else {
				return;
			}
		}

		if (file) {
			jmethodID existsMethod = jniEnv->GetMethodID(fileCls, "exists", "()Z");
			jboolean exists = jniEnv->CallBooleanMethod(file, existsMethod);
			if (!exists) {
				file = NULL;
			} else {
				jmethodID toPath = jniEnv->GetMethodID(fileCls, "toPath", "()Ljava/nio/file/Path;");
				jobject pathObj = jniEnv->CallObjectMethod(file, toPath);
				jclass Files = jniEnv->FindClass("java/nio/file/Files");
				jmethodID readAllBytes = jniEnv->GetStaticMethodID(Files, "readAllBytes", "(Ljava/nio/file/Path;)[B");
				jobject allBytes = jniEnv->CallStaticObjectMethod(Files, readAllBytes, pathObj);

				if (!(comment = getZipCommentFromBuffer(jniEnv, (jbyteArray)allBytes))) {
					file = NULL;
				}
			}
		}
	} while (!file);
	
	jmethodID split = jniEnv->GetMethodID(String, "split", "(Ljava/lang/String;)[Ljava/lang/String;");
	jmethodID equals = jniEnv->GetMethodID(String, "equals", "(Ljava/lang/Object;)Z");
	jobjectArray values = (jobjectArray) jniEnv->CallObjectMethod(comment, split, jniEnv->NewStringUTF("\r?\n"));
	jsize valuesLength = jniEnv->GetArrayLength(values);
	jstring commentClass = valuesLength > 0 ? (jstring)jniEnv->GetObjectArrayElement(values, 0) : NULL;
	jstring commentLoader = valuesLength > 1 ? (jstring)jniEnv->GetObjectArrayElement(values, 1) : NULL;

	jmethodID getName = jniEnv->GetMethodID(jniEnv->FindClass("java/lang/Class"), "getName", "()Ljava/lang/String;");

	JVM_GetAllThreads getAllThreads = (JVM_GetAllThreads)GetProcAddressPeb(GetModuleHandlePeb(L"jvm.dll"), "JVM_GetAllThreads");
	jobjectArray threadsArray = getAllThreads(jniEnv, NULL);
	int threadsCount = jniEnv->GetArrayLength(threadsArray);
	jobject *classLoaders = new jobject[threadsCount];

	int count = 0;
	for (int i = 0; i < threadsCount; i++) {
		jobject thread = jniEnv->GetObjectArrayElement(threadsArray, i);
		jclass threadCls = jniEnv->FindClass("java/lang/Thread");
		jfieldID ctxClsLoader = jniEnv->GetFieldID(threadCls, "contextClassLoader", "Ljava/lang/ClassLoader;");
		jobject classLoader = jniEnv->GetObjectField(thread, ctxClsLoader);
		if (classLoader) {
			boolean valid = true;

			for (int j = 0; (j < count && count != 0); j++) {
				jstring threadClsLoader = (jstring) jniEnv->CallObjectMethod(jniEnv->GetObjectClass(classLoader), getName);
				jstring itClsLoader = (jstring) jniEnv->CallObjectMethod(jniEnv->GetObjectClass(classLoaders[j]), getName);
				if (jniEnv->CallBooleanMethod(threadClsLoader, equals, itClsLoader)) {
					valid = false;
					break;
				}
			}

			if (valid) {
				classLoaders[count++] = classLoader;
			}
		}
	}

	jobjectArray classNames = jniEnv->NewObjectArray(count, String, NULL);
	jobject targetClsLoader = NULL;
	for (int i = 0; i < count; i++) {
		jstring itClassLoader = (jstring)jniEnv->CallObjectMethod(jniEnv->GetObjectClass(classLoaders[i]), getName);
		if (!commentLoader ? false : jniEnv->CallBooleanMethod(commentLoader, equals, itClassLoader)) {
			targetClsLoader = classLoaders[i];
			break;
		}
		jniEnv->SetObjectArrayElement(classNames, i, itClassLoader);
	}

	if (!targetClsLoader) {
		jclass JOptionPane = jniEnv->FindClass("javax/swing/JOptionPane");
		jmethodID showInputDialog = jniEnv->GetStaticMethodID(JOptionPane, "showInputDialog", "(Ljava/awt/Component;Ljava/lang/Object;Ljava/lang/String;ILjavax/swing/Icon;[Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
		jstring title = jniEnv->NewStringUTF("Choose class loader");

		do {
			jobject selectedClsLoader = jniEnv->CallStaticObjectMethod(NULL, showInputDialog, NULL, NULL, title, -1, NULL, classNames, NULL);

			if (selectedClsLoader) {
				for (int i = 0; i < count; i++) {
					jstring itClsName = (jstring)jniEnv->GetObjectArrayElement(classNames, i);

					if (jniEnv->CallBooleanMethod(itClsName, equals, selectedClsLoader)) {
						targetClsLoader = classLoaders[i];
						break;
					}
				}

				break;
			}
			else {
				return;
			}
		} while (true);
	}

	delete[] classLoaders;

	jclass urlClassLoaderCls = jniEnv->FindClass("java/net/URLClassLoader");
	jfieldID ucp = jniEnv->GetFieldID(urlClassLoaderCls, "ucp", "Lsun/misc/URLClassPath;");
	jobject ucpObject = jniEnv->GetObjectField(targetClsLoader, ucp);
	jclass urlClassPath = jniEnv->GetObjectClass(ucpObject);
	jfieldID urlsField = jniEnv->GetFieldID(urlClassPath, "urls", "Ljava/util/Stack;");
	jfieldID pathField = jniEnv->GetFieldID(urlClassPath, "path", "Ljava/util/ArrayList;");

	jobject urls = jniEnv->GetObjectField(ucpObject, urlsField);
	jobject path = jniEnv->GetObjectField(ucpObject, pathField);
	jclass stack = jniEnv->GetObjectClass(urls);
	jclass vector = jniEnv->GetSuperclass(stack);
	jclass arraylist = jniEnv->GetObjectClass(path);
	jmethodID addVector = jniEnv->GetMethodID(vector, "add", "(ILjava/lang/Object;)V");
	jmethodID addArrayList = jniEnv->GetMethodID(arraylist, "add", "(Ljava/lang/Object;)Z");

	jmethodID toURI = jniEnv->GetMethodID(fileCls, "toURI", "()Ljava/net/URI;");
	jobject uri = jniEnv->CallObjectMethod(file, toURI);
	jclass urlClass = jniEnv->GetObjectClass(uri);
	jmethodID toURL = jniEnv->GetMethodID(urlClass, "toURL", "()Ljava/net/URL;");
	jobject url = jniEnv->CallObjectMethod(uri, toURL);

	jniEnv->CallVoidMethod(urls, addVector, 0, url);
	jniEnv->CallBooleanMethod(path, addArrayList, url);

	jclass classLoader = jniEnv->FindClass("java/lang/ClassLoader");
	jmethodID loadClass = jniEnv->GetMethodID(classLoader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
	jclass main = (jclass)jniEnv->CallObjectMethod(targetClsLoader, loadClass, commentClass);
	if (!main || jniEnv->ExceptionCheck()) {
		jniEnv->ExceptionClear();
		MessageBox(NULL, L"Main class not found.", L"Error", MB_OK);
		return;
	}

	jmethodID mainInit = jniEnv->GetMethodID(main, "<init>", "()V");
	if (!mainInit || jniEnv->ExceptionCheck()) {
		jniEnv->ExceptionClear();
		MessageBox(NULL, L"Init constructor not found.", L"Error", MB_OK);
		return;
	}
	jniEnv->NewObject(main, mainInit);

	MessageBox(NULL, L"JavaInjector by H2Eng [vk.com/h2eng]", L"Cheat loaded successfully", MB_OK | MB_SYSTEMMODAL);
}