这段要反转一下！你说的对，但是
==============================

**from kernel/fork.c:fork_mem_copy**

.. code-block:: c

    // 复制栈，栈不共享，子进程需要申请物理地址，并复制过来(注意栈的复制方向)
    for (addr_lin = p_proc_current->task.memmap.stack_lin_base;
            addr_lin > p_proc_current->task.memmap.stack_lin_limit;
            addr_lin -= num_4K) {
        lin_mapping_phy(
            SharePageBase,
            0,
            ppid,
            PG_P | PG_USU | PG_RWW,
            0); // 使用前必须清除这个物理页映射
        lin_mapping_phy(
            SharePageBase,
            MAX_UNSIGNED_INT,
            ppid,
            PG_P | PG_USU | PG_RWW,
            PG_P | PG_USU | PG_RWW); // 利用父进程的共享页申请物理页
        memcpy(
            (void*)SharePageBase,
            (void*)(addr_lin & 0xFFFFF000),
            num_4K); // 将数据复制到物理页上,注意这个地方是强制一页一页复制的
        lin_mapping_phy(
            addr_lin, // 线性地址
            get_page_phy_addr(
                ppid,
                SharePageBase), // 物理地址，获取共享页的物理地址，填进子进程页表
            pid, // 要挂载的进程的pid，子进程的pid
            PG_P | PG_USU | PG_RWW,  // 页目录属性，一般都为可读写
            PG_P | PG_USU | PG_RWW); // 页表属性，栈是可读写的
    }

行行行我知道栈是向低地址生长的也看到你的 ``addr_lin -= num_4K`` 了。

但是，就是说，你晓不晓得 memcpy 是从低地址向高地址拷贝的？

你这是 clone 栈区 [stack_lin_limit, stack_lin_base) 吗？你这不分明在 clone (stack_lin_limit+4K, stack_lin_base+4K] 吗？！又缺斤短两又酒里掺水了啊喂！
