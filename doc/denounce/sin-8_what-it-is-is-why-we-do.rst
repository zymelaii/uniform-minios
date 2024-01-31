页面是 4K 对齐的！所以整页整页拷贝就行了
=========================================

**from kernel/fork.c:fork_mem_copy**

.. code-block:: c

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

承接上文\ **这段要反转一下！你说的对，但是**，fork_mem_copy 在经历千锤百炼之后再次在希望的终点给予我们背刺！

简单修复前文中所提的方向的问题，可以将代码修改如下：

.. code-block:: c

    for (addr_lin = p_proc_current->task.memmap.stack_lin_limit;
        addr_lin < p_proc_current->task.memmap.stack_lin_base;
        addr_lin += num_4K) {
        lin_mapping_phy(SharePageBase, 0, ppid, PG_P | PG_USU | PG_RWW, 0);
        lin_mapping_phy(
            SharePageBase,
            MAX_UNSIGNED_INT,
            ppid,
            PG_P | PG_USU | PG_RWW,
            PG_P | PG_USU | PG_RWW);
        memcpy((void*)SharePageBase, (void*)(addr_lin & 0xFFFFF000), num_4K);
        lin_mapping_phy(
            addr_lin,
            get_page_phy_addr(ppid, SharePageBase),
            pid,
            PG_P | PG_USU | PG_RWW,
            PG_P | PG_USU | PG_RWW);
    }

很对吧！

看上去确实很对，栈的问题修复了，可能会错的地方也注意到了。

但是好了嘛！？

大抵确实是好了吧，毕竟整个代码里很多其它类似的地方也是这么干的，不也是正常的跑了吗？再说了，既然原来偏移了 0x1000 的代码能正常跑，那我只要把所有跟 stack addr 有关的地方全部把 0x1000 偏移的问题修复了，根据平移不变性那不就是很显然地也能正常运行吗？！

原神，启动！！

*叮！你收获了一个 page fault！*

不可能，这绝对不可能！！！简单查下 stack limit 的 pte——没错啊，这不是存在吗？！属性值不都对着吗？！

……

……

……

屮，stack base 的 pte 怎么没有？！往前 4B，也没有！往前 0x1000，好，存在！！——MD，少拷贝一页！

再回过来看代码——不对，是先看看 stack 的 base 和 limit。曾经很偶然地瞥到了 ``#define StackLinBase (ArgLinBase - num_4B)``，当时只道是莫名其妙，但又道无伤大雅，并不会影响到代码的执行。可现今一看，如何不能！

当万千错误集结到一起汇聚出一个\ **恰巧**\ 能够正常运行的代码，那修复他都是一种莫大的罪过！

在其它情况下，我重写了 range mapping 的方法，并充分处理了页对齐的问题，故而 elf LOAD 倒是十分顺利地运行了。但在 vpage、heap、arg、stack 等 laddr 手动编排的内存空间上，则是在 fork_mem_copy 中利用共享页便利化拷贝并将物理页转移给子进程。按理说这些玩意应该是同生共死的，可细细一查，怎么会只有 stack 出问题了呢？

一瞅——就健壮性而言，fork_mem_copy 理所应当要寄！laddr 整页递增，那要是抵达 limit 时 limit 没有与页面尺寸对齐怎么办？页内偏移 ``0 ~ limit & 0xfff`` 的这一部分难道就抛弃嘛！那必然还是要再申请一页的！如此情形不处理，stack 缺页不是理所应当！

但为什么 vpage、heap、arg 这些没寄？（一方面是因为这时候压根就没 pte，所以怎么想都寄不了啊~）看看人家，他们的 base 和 limit 可都是 4K 对齐的啊！base 即是页边界，limit 即是最后一页的边界，这一来二去，边界对边界，自然不会有缺页的问题。所以他们寄了吗？\ **运气好没寄罢了！**

倘若不是一手 ``StackLinBase = ArgLinBase - num_4B`` 把 4K 对齐的特性打得稀烂，估计这等差错还要再延后或者到底还是让人察觉不了——有多少人会斟酌一段实在令人作呕又刚好能够正常运行的代码呢？

再去看看偏移了 0x1000 的旧实现，那果真没问题吗？

自然是巧合下的又一个巧合！

**巧合在 base 处必然会有 pte！**

**巧合在 stack 的大小足够大，简单的用户程序难以令 esp 接近 limit 而触发 limit 处的缺页！**

**巧合在 esp 并非是在真正的栈顶，而是在距真正的栈顶 0x1000 处的低地址位置！**

从一开始就没有正确的算法实现！

**“页面是 4K 对齐的！所以整页整页拷贝就行了”**，这对吗？这自然可以是对的，但没有满足边界地址页对齐，又谈何正确！终究不过是在一环套一环的谬误中苟活的虚伪 **PASS** 罢了！

一朝败露，便忘恩负义地杀得人片甲不留，又金蝉脱壳了去！
