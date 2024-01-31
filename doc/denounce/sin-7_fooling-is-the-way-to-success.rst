你不懂，糊弄同事有助于涨业绩！
==============================

**from kernel/pagetbl.c:vmalloc**

.. code-block:: c

    uint32_t vmalloc(uint32_t size) {
        uint32_t temp;
        if (p_proc_current->task.info.type == TYPE_PROCESS) { // 进程直接就是标识
            temp = p_proc_current->task.memmap.heap_lin_limit;
            p_proc_current->task.memmap.heap_lin_limit += size;
        } else { // 线程需要取父进程的标识
            temp = *((uint32_t *)p_proc_current->task.memmap.heap_lin_limit);
            (*((uint32_t *)p_proc_current->task.memmap.heap_lin_limit)) += size;
        }
        return temp;
    }

已知 heap_lin_limit 的类型是 uint32_t，并且 thread 的 head 是指向父进程/主线程的 head ptr。

.. note::

   子线程与主线程共用堆，但是变量独立。为了维护堆，在此实现中子线程通过指向主线程 heap limit 的指针完成同步。

你自己瞅瞅，你写的是人话吗？

加这么点破烂添头你有工资拿吗？

除了恶心自己恶心别人还有什么屁用吗？

我不苛求你把结构改良，你直接把注释删了不行吗？还 ``需要获取父进程的标识``……

你这获取的是父进程的标识吗？！难道你自己都不清楚自个儿是怎么实现父子线程的堆共享的吗？你需要注释的是\ ``这个 limit 其实是指向主线程 heap limit 的指针``\ 而不是\ ``获取父进程的标识``\ 啊！
