Plt Hook
===
# 概述
因为都是Native Hook，Plt Hook和inline、虚函数的hook就放在一个仓库里面了，featute_plt分支是plthook的demo，而master是虚函数的hook。
本分支主要使用字节跳动的bHook实现了对于so内存的分配检测，用于定位分析异常内存分配，协助解决问题。
# 实现
借助bHook使用plt hook来hook住malloc()方法来检测内存的分配行为，排查异常的内存分配，观察到了异常分配后，使用dladdr和addr2line工具还原so库名称、行号和具体的函数名，从而定位问题。
# 参考文章
[内存优化：so库申请内存优化](https://juejin.cn/post/7204142890791026725)


