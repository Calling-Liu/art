# art
Android native hook

# GC抑制和Jit抑制
## 概述
### 项目起因
之前在逛掘金时，发现了字节跳动大佬写的文章，是关于native hook的，大佬举例使用的就是GC抑制，通过抑制GC来提高应用的启动速度。当时没有时间，收藏文章之后，最近有了时间，便马上实际实现试试。
### 项目内容概述
该项目主要实现了，在Android系统上执行GC和Jit的抑制，实现的手段是，使用虚函数对GC和Jit的Run函数进行hook，将系统的Run函数虚函数地址，替换为自己实现的函数的地址，睡眠5s之后，再将虚函数地址替换回去，执行原本的任务。
## 项目引用的开源库
* [ndk_dlopen](https://github.com/Rprop/ndk_dlopen) 实际上实现的时候发现这个开源库只能查找.dynsym段，Android7.0之后ConcurrentGCTask在.dynsym段, 而Run方法在.symtab段, 而ndk_dlsym只能搜索.dynsym段。因为这个原因最后没用这个实现，姑且保留这部分的代码。
* [xdl](https://github.com/hexhacking/xDL) 最终使用的查找函数地址的开源库，这个开源库可以绕开link nameSpace的限制，也可以查找.symtab段。不过需要注意的是：我使用的时候无法下载到这个依赖，所以自己把源码下载下来依赖的。
## 参考的文章
* [GC抑制](https://juejin.cn/post/7280345543145127977) 基本按照这个流程学习的，需要的注意的是大佬的使用的是ndk_dlopen，需要换成xdl.
* [GC与Jit抑制](https://juejin.cn/post/7244947219161006136) 在这个文章中找到了xdl库，大佬自己是使用ShadowHook实现的，ShadowHook实现起来很简单，直接修改的是汇编语言，可能不太稳定？
* [启动优化](https://juejin.cn/post/7217828263259291708) 这篇文章实现比较详细。
## 核心实现
* art/src/main/cpp/art.cpp
* art/src/main/cpp/CMakeLists.txt
## 支持的环境
### Android API
理论上支持Android 4.1 - 13 (API 16 -33)
### 支持的架构
ABI: armeabi-v7a, arm64-v8a, x86, x86_64
## 注
### 实现过程踩坑记录
* ndk_dlopen Android 7.0以上无法使用。
* 虚函数hook睡眠之后，执行部分有坑，如下
```C++
typedef void (*TaskRunType)(void *, void *);
void hookRun(void *task, void *thread) {
   //...
    ((TaskRunType) originFun)(task, thread); // Run函数是Task的成员函数, 必须传入task
  // ...
}
```
* 搜索匹配run函数地址, 我写的都是5，实际线上这样不够稳妥，需要虚函数表size / sizeof(void *).
