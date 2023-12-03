#include <jni.h>
#include <android/log.h>
#include <unistd.h>
#include <asm-generic/mman-common.h>
#include <sys/mman.h>
#include "dlopen.h"
#include "xdllibs/include/xdl.h"

#define LOG_TAG            "art-hook"
#define LOGI(fmt, ...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##__VA_ARGS__)

typedef void (*TaskRunType)(void *, void *);
// GC
void **mSlot = nullptr;
void *originFun = nullptr;
// Jit
void **mSlotJit = nullptr;
void *originFunJit = nullptr;
void *taskAddressJit = nullptr;
void *runAddressJit = nullptr;


bool replaceFunc(void **slot, void *func) {
    // 计算指定地址所在的内存页的起始地址
    void *page = (void *)((size_t) slot & (~(size_t)(PAGE_SIZE - 1)));
    // 将内存页设置为可读写
    if (mprotect(page, PAGE_SIZE, PROT_READ | PROT_WRITE) != 0) return false;
    // 替换函数指针
    *slot = func;
    // 对于ARM架构, 执行 cacheflush操作, 刷新指令和数据结构
#ifdef __arm__
    cacheflush((long)page, (long)page + PAGE_SIZE, 0);
#endif
    // 将内存页设置为只读
    mprotect(page, PAGE_SIZE, PROT_READ);
    // 操作成功, 返回 true
    return true;
}

void hookRun(void *task, void *thread) {
    LOGE("start sleep 5s!");
    sleep(5);
    LOGE("wake up!");
    replaceFunc(mSlot, originFun);
    ((TaskRunType) originFun)(task, thread);
    LOGE("hook End!");
}

void hookRunJit(void *task, void *thread) {
    LOGE("start sleep 5s!");
    sleep(5);
    LOGE("wake up!");
    replaceFunc(mSlotJit, originFunJit);
    ((TaskRunType) originFunJit)(task, thread);
    LOGE("hook End!");
}
/**
 * 使用开源库 https://github.com/Rprop/ndk_dlopen实现的GC抑制
 *
 * 缺点: 只能在Android 7.0之前的系统上使用
 * 原因: Android7.0之后ConcurrentGCTask在.dynsym段, 而Run方法在.symtab段, 而ndk_dlsym只能搜索.dynsym段, 无法搜索
 * .symtab段，导致无法查找到Run函数的地址
 *
 * @param env
 */
void delayGCx7(JNIEnv *env) {
    int androidApi = android_get_device_api_level();
    ndk_init(env);
    // 以RTLD_NOW模式打开动态库libart.so, 拿到句柄, RTLD_NOW即解析出每个未定义变量的地址
    void *handle = ndk_dlopen("libart.so", RTLD_NOW);
    void *runAddress = nullptr;

    LOGE("api %d", androidApi);

    // 通过符号拿到run方法
    if (androidApi < __ANDROID_API_M__) { // Android 5.x 版本
        runAddress = ndk_dlsym(handle, "_ZN3art2gc4Heap12ConcurrentGCEPNS_6ThreadE");
    } else if (androidApi < 34) { // Android 6 - 13
        runAddress = ndk_dlsym(handle, "_ZN3art2gc4Heap16ConcurrentGCTask3RunEPNS_6ThreadE");
        LOGE("get address!");
    } else {
        LOGE("return!");
        return;
    }

    // 通过符号拿到ConcurrentGCTask类首地址
    void *taskAddress = ndk_dlsym(handle, "_ZTVN3art2gc4Heap16ConcurrentGCTaskE");

    if (taskAddress == nullptr || runAddress == nullptr) {
        LOGE("null address return!");
        return;
    }

    for (size_t i = 0; i < 5; i++) {
        void *vfunc = ((void **) taskAddress)[i];
        if (vfunc == runAddress) {
            LOGE("find address");
            mSlot = (void **)taskAddress + i;
        }
    }
    // 保存原有函数
    originFun = *mSlot;
    // 将虚函数表中值替换成hook函数的地址
    replaceFunc(mSlot, (void *) &hookRun);
}

/**
 * 使用开源库 https://github.com/hexhacking/xDL 实现
 * 这个库可以绕过Android 7.0 + Linker namespace限制
 * 查询.dynsym中的动态链接符号和.symtab以及.gnu_debugdata里的.symtab中的调试符号
 * 支持Android 4.1 - 13 (API 16 -33)
 *
 */
void delayGC() {
    int androidApi = android_get_device_api_level();
    void *handle = xdl_open("libart.so", RTLD_NOW);
    void *runAddress = nullptr;

    LOGE("api %d", androidApi);

    // 通过符号拿到run方法
    if (androidApi < __ANDROID_API_M__) { // Android 5.x 版本
        runAddress = xdl_dsym(handle, "_ZN3art2gc4Heap12ConcurrentGCEPNS_6ThreadE", nullptr);
    } else if (androidApi < 34) { // Android 6 - 13
        runAddress = xdl_dsym(handle, "_ZN3art2gc4Heap16ConcurrentGCTask3RunEPNS_6ThreadE", nullptr);
        LOGE("get address!");
    } else {
        LOGE("return!");
        return;
    }

    // 通过符号拿到ConcurrentGCTask类首地址
    void *taskAddress = xdl_dsym(handle, "_ZTVN3art2gc4Heap16ConcurrentGCTaskE", nullptr);


    if (taskAddress == nullptr || runAddress == nullptr) {
        LOGE("null address return!");
        return;
    }

    for (size_t i = 0; i < 5; i++) {
        void *vfunc = ((void **) taskAddress)[i];
        if (vfunc == runAddress) {
            LOGE("find address");
            mSlot = (void **)taskAddress + i;
        }
    }

    if (mSlot == nullptr) {
        LOGE("can not find address");
        return;
    }

    // 保存原有函数
    originFun = *mSlot;
    replaceFunc(mSlot, (void *) &hookRun);
}

/**
 * 延迟5s执行Jit
 *
 */
void delayJit() {
    // 加载库
    void *handle = xdl_open("libart.so", RTLD_NOW);

    // 通过符号拿到JitCompileTaskE类首地址
    taskAddressJit = xdl_dsym(handle, "_ZTVN3art3jit14JitCompileTaskE", nullptr);
    // 拿到jit的执行函数Run
    runAddressJit = xdl_dsym(handle, "_ZN3art3jit14JitCompileTask3RunEPNS_6ThreadE", nullptr);

    if (taskAddressJit == nullptr || runAddressJit == nullptr) {
        LOGE("null address return!");
        return;
    }

    for (size_t i = 0; i < 5; i++) {
        void *vfunc = ((void **) taskAddressJit)[i];
        if (vfunc == runAddressJit) {
            LOGE("find address");
            mSlotJit = (void **)taskAddressJit + i;
        }
    }

    if (mSlotJit == nullptr) {
        LOGE("can not find address");
        return;
    }

    // 保存原有函数
    originFunJit = *mSlotJit;
    replaceFunc(mSlotJit, (void *) &hookRunJit);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ptrain_artimple_ARTHook_init(JNIEnv *env, jobject thiz) {
    delayJit();
}




