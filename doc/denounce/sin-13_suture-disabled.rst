縫合された幻の悪霊です
=======================

**from kernel/syscallc.c:sys_malloc,sys_free**

.. code-block:: c

    void *sys_malloc(int size) {
        int vir_addr, AddrLin;
        vir_addr = vmalloc(size);
        // 一个字节一个字节处理
        for (AddrLin = vir_addr; AddrLin < vir_addr + size; AddrLin += num_4B) {
            lin_mapping_phy(
                AddrLin,                  // 线性地址
                MAX_UNSIGNED_INT,         // 物理地址
                p_proc_current->task.pid, // 进程pid
                PG_P | PG_USU | PG_RWW,   // 页目录的属性位
                PG_P | PG_USU | PG_RWW);  // 页表的属性位
        }
        return (void *)vir_addr;
    }

    int sys_free(void *arg) {
        memarg = (struct memfree *)arg;
        return do_free(memarg->addr, memarg->size);
    }

看出来什么问题了吗？想必应当是不容易看出来的。

让我们再来看看 vmalloc 是怎么实现的：

.. code-block:: c

    u32 vmalloc(u32 size) {
        u32 temp;
        // 进程直接就是标识
        if (p_proc_current->task.info.type == TYPE_PROCESS) {
            temp = p_proc_current->task.memmap.heap_lin_limit;
            p_proc_current->task.memmap.heap_lin_limit += size;
        } else { // 线程需要取父进程的标识
            temp = *((u32 *)p_proc_current->task.memmap.heap_lin_limit);
            (*((u32 *)p_proc_current->task.memmap.heap_lin_limit)) += size;
        }
        return temp;
    }

啊哈，不叫朵拉的小朋友应该已经被心机之蛙一直摸肚子了。但是保险起见我们或许还需要再看看 do_free 的实现——啊对了还有它的同伴 do_malloc：

.. code-block:: c

    u32 do_malloc(u32 size) {
	    return memman_alloc(memman,size);
    }

    u32 do_free(u32 addr,u32 size) {
        return memman_free(memman,addr,size);
    }

*好了就到此为止吧，前面的区域以后再来探索吧。*

从以上实现中分明可以看到这样的关系：

- do_malloc 与 do_free 借助 memman 实现内存的管理
- vmalloc 通过进程的堆地址实现内存的分配
- sys_malloc 与 sys_free 构成了把从 **堆地址分配** 的地址空间交给 **memman 释放** 的处理过程

6（一个象征着吉祥的数字）。

你要不要看看你自己在干什么？！

顶级缝合大师是吧？页表申请释放的地方 malloc 和 free 乱搭没玩够这边用户态内存管理也一样放飞自我了是吧？！

那 TM 用来回收 do_malloc 内存的 memman 是你 vmalloc 该去的地方吗？

都 21 世纪了怎么还有玩意连垃圾分类都搞不明白？

您到底是不会分类呢，还是懒得分类呢，还是摆明了跟政策对着干存心乱扔垃圾？！

这就不说了，我不在用户态分配内存就不会出任何事，但是你能不能至少把函数命名整好？！一个 memman 支持 ``do_kmalloc`` ``do_kmalloc_4k`` ``do_malloc`` ``do_malloc_4k``，脑子机灵点的都知道你仅靠 memman 一个结构就把内核态分配、内核页面分配、用户态分配、用户页面分配四大类内存管理全部承包了。

但你 TM 光拉一坨就是死活不用是吧？！你看看那 do_malloc 是能用的样子嘛！半路直接拱手送 vmalloc，这直接用哈，页面没映射用户态访问不了；这不用吧，哈，就真还是僵尸代码一坨。

有这种设计你就老老实实把 do\_\* 改回 memman_alloc\* 不行吗？怎么还偏要原模原样封成一个 do\_\*？！为了少传一个 memman 的参数连脑子都不要了是吧？！

你说你为了少传参数我也认了，那你 TM 把 do_free 也一起省了行不行？！

妈的，一个 ``void free(void*, size_t)`` 的函数签名，你是想给谁用？真当我是全知全能神直接在当下精准预测遥远的未来是吧！这种玩意你除了在小范围 malloc 一下或者上一整套贯穿始终的分配规则还能怎么用？我说你还不如直接全部栈上开个 1 KB，怎么潇洒怎么来。
