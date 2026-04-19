#include <jni.h>

#include <atomic>

#include "hooks.h"
#include "menu.h"
#include "mod_features.h"
#include "config.h"
#include "core/platform/android_bridge.hpp"

namespace {

std::atomic<bool> g_coreInitialized{false};

void initializeCore() {
    if (!g_coreInitialized.exchange(true)) {
        Config::ConfigManager::getInstance().load("/data/local/tmp/subway_mod_config.json");
        Features::FeatureManager::getInstance().initialize();
    }
    Hooks::HookManager::getInstance().initialize();
}

}

extern "C" JNIEXPORT void JNICALL
Java_com_mod_loader_ModLoader_initMod(JNIEnv* env, jobject thiz) {
    (void)env;
    (void)thiz;
    initializeCore();
}

extern "C" JNIEXPORT void JNICALL
Java_com_mod_loader_ModLoader_toggleMenu(JNIEnv* env, jobject thiz) {
    (void)env;
    (void)thiz;
    Menu::MenuRenderer::getInstance().toggleVisibility();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_mod_loader_ModLoader_isMenuVisible(JNIEnv* env, jobject thiz) {
    (void)env;
    (void)thiz;
    return static_cast<jboolean>(Menu::MenuRenderer::getInstance().isVisible());
}

extern "C" JNIEXPORT void JNICALL
Java_com_google_firebase_MessagingUnityPlayerActivity_initSubwayMod(JNIEnv* env, jclass clazz) {
    (void)env;
    (void)clazz;
    initializeCore();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_google_firebase_MessagingUnityPlayerActivity_nativeTouchFromDispatchTouchEvent(
    JNIEnv* env,
    jclass clazz,
    jint action,
    jfloat x,
    jfloat y,
    jint pointerCount,
    jint viewWidth,
    jint viewHeight) {
    (void)env;
    (void)clazz;
    Menu::MenuRenderer::getInstance().pushTouchEvent(action, x, y, pointerCount, viewWidth, viewHeight);
    return static_cast<jboolean>(Menu::MenuRenderer::getInstance().wantsCaptureInput());
}

extern "C" JNIEXPORT void JNICALL
Java_com_sybogames_subway_surfers_game_MessagingUnityPlayerActivity_initSubwayMod(JNIEnv* env, jobject thiz) {
    (void)env;
    (void)thiz;
    initializeCore();
}

__attribute__((constructor))
void library_init() {
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    (void)reserved;
    platform::android::set_java_vm(vm);
    initializeCore();
    return JNI_VERSION_1_6;
}
