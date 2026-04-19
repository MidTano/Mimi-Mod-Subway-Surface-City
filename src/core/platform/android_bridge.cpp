#include "android_bridge.hpp"

#include <jni.h>

#include <atomic>
#include <mutex>

namespace platform::android {

namespace {

std::atomic<JavaVM*> g_jvm{nullptr};
std::mutex g_lock;
jclass    g_mod_loader_class = nullptr;
jmethodID g_prompt_seed_mid  = nullptr;
jmethodID g_poll_seed_mid    = nullptr;
std::atomic<bool> g_bindings_failed{false};

bool ensure_bindings(JNIEnv* env) {
    if (g_bindings_failed.load()) return false;
    if (g_mod_loader_class != nullptr &&
        g_prompt_seed_mid != nullptr &&
        g_poll_seed_mid != nullptr) {
        return true;
    }
    jclass local = env->FindClass("com/mod/loader/ModLoader");
    if (local == nullptr) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        g_bindings_failed.store(true);
        return false;
    }
    jclass global = reinterpret_cast<jclass>(env->NewGlobalRef(local));
    env->DeleteLocalRef(local);
    if (global == nullptr) {
        g_bindings_failed.store(true);
        return false;
    }
    jmethodID prompt = env->GetStaticMethodID(global,
        "promptSeedInput", "(Ljava/lang/String;)V");
    if (prompt == nullptr) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        env->DeleteGlobalRef(global);
        g_bindings_failed.store(true);
        return false;
    }
    jmethodID poll = env->GetStaticMethodID(global,
        "pollSeedInput", "()Ljava/lang/String;");
    if (poll == nullptr) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        env->DeleteGlobalRef(global);
        g_bindings_failed.store(true);
        return false;
    }
    g_mod_loader_class = global;
    g_prompt_seed_mid = prompt;
    g_poll_seed_mid = poll;
    return true;
}

struct JniScope {
    JavaVM* vm = nullptr;
    JNIEnv* env = nullptr;
    bool attached = false;

    explicit JniScope(JavaVM* v) : vm(v) {
        if (vm == nullptr) return;
        if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK) {
            return;
        }
        if (vm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
            attached = true;
        } else {
            env = nullptr;
        }
    }
    ~JniScope() {
        if (attached && vm != nullptr) vm->DetachCurrentThread();
    }
    JNIEnv* operator->() const { return env; }
};

}

void set_java_vm(void* vm) {
    g_jvm.store(reinterpret_cast<JavaVM*>(vm));
}

bool has_java_vm() {
    return g_jvm.load() != nullptr && !g_bindings_failed.load();
}

void request_seed_input(const std::string& current) {
    JavaVM* vm = g_jvm.load();
    if (vm == nullptr) {
        return;
    }
    JniScope scope(vm);
    if (scope.env == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> guard(g_lock);
    if (!ensure_bindings(scope.env)) return;

    jstring arg = scope.env->NewStringUTF(current.c_str());
    scope.env->CallStaticVoidMethod(g_mod_loader_class, g_prompt_seed_mid, arg);
    if (scope.env->ExceptionCheck()) {
        scope.env->ExceptionClear();
    }
    if (arg != nullptr) scope.env->DeleteLocalRef(arg);
}

bool poll_seed_input(std::string& out_value) {
    JavaVM* vm = g_jvm.load();
    if (vm == nullptr) return false;
    JniScope scope(vm);
    if (scope.env == nullptr) return false;

    std::lock_guard<std::mutex> guard(g_lock);
    if (!ensure_bindings(scope.env)) return false;

    jobject result = scope.env->CallStaticObjectMethod(g_mod_loader_class, g_poll_seed_mid);
    if (scope.env->ExceptionCheck()) {
        scope.env->ExceptionClear();
        return false;
    }
    if (result == nullptr) return false;

    const char* chars = scope.env->GetStringUTFChars(static_cast<jstring>(result), nullptr);
    if (chars != nullptr) {
        out_value.assign(chars);
        scope.env->ReleaseStringUTFChars(static_cast<jstring>(result), chars);
    } else {
        out_value.clear();
    }
    scope.env->DeleteLocalRef(result);
    return true;
}

}
