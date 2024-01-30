线性地址？物理地址？能跑就行！
===============================

**from \*.c**

.. code-block:: c

    buf = (PBYTE)K_PHY2LIN(do_kmalloc(Bytes_Per_Sector * sizeof(BYTE)));
    ...
    do_free(buf);

以及队友被 miniOS 带偏后，按照其写法增加的新功能实现片段：

.. code-block:: c

    Elf32_Ehdr* elf_header = NULL;
    Elf32_Phdr* elf_proghs = NULL;
    elf_header             = do_kmalloc(sizeof(Elf32_Ehdr));
    assert(elf_header != NULL);
    read_Ehdr(fd, elf_header, 0);
    elf_proghs = do_kmalloc(sizeof(Elf32_Phdr) * elf_header->e_phnum);
    assert(elf_proghs != NULL);
    for (int i = 0; i < elf_header->e_phnum; i++) {
        uint32_t offset = elf_header->e_phoff + i * sizeof(Elf32_Phdr);
        read_Phdr(fd, elf_proghs + i, offset);
    }

    if (exec_load(fd, elf_header, elf_proghs) == -1) {
        do_close(fd);
        do_free(elf_header);
        do_free(elf_proghs);
        return -1;
    }

.. tip::

    以上关乎原汁原味 miniOS malloc 和 free 由于过于古早，回溯较为麻烦，此处使用 uniform-minios 的 syscall、malloc 修正版作为参考代码，但保证谴责目标依旧原汁原味！

众所周知，``void \*ptr = malloc(size); ... free(ptr);`` 是一个内存管理的范式。

但某个 miniOS 显然不是这么想的。

首先的首先，miniOS 用户态的 malloc 是没用的，只在内核态下使用系列方法进行内存管理。

.. note::

    关于用户态的 malloc 为什么用不了，简单点来说是 heap 没有映射页面，但这之中涉及到一些更需要谴责的恶劣操作，故此处先不深入展开。

然而说是内存管理，其实此系列方法实现的是对物理地址区间的管理。只不过仰赖于内核页面总是可用的，故可以通过 K_PHY2LIN 从分配得到的物理地址直接访问对应的内核空间线性地址，继而完成访存操作。

好戏就是这么开场的。

第一，此 malloc 系列返回的是物理地址，在页表机制的作用下，物理地址是不能被访问的！除非页表中有与该物理地址等值的线性地址映射。

第二，malloc 得到的是什么就应该去释放什么。难不成你打白条借了三千元还能把货币单位一改，打扮成“萬元冥幣”再还回去？您说您这礼貌吗？

关于第二点无需多言，直接参照样例代码即可悉知。

重点是第一个——天佑我也！！天佑我也？！

**请看第二段看上去非常正确的代码。**

回顾前文，在旧的充满未知与挑战的稀烂的栈配置与细细品尝便可发现漏洞百出的页表实现的共同作用下——这其中或许还有从 loader 到 kernel_main 时清除 loader 低端页表页与 pcb 初始化页表的助力——这对物理地址堂而皇之地访问竟然大摇大摆地“正常”运行了好长一段时间！

或许有人会说，这也不是不可以啊，有线性地址映射不就行了？

但这正是问题所在！在现在的内存模型中，这一段地址就不应该能够被进程在内核态正常访问！那一些映射了地址 0 的 pde、pte 压根就不能且不应该存在！而更过分的是，此处的线性地址甚至不是映射到等值的物理地址上的！这就压根连 loader 中页表映射的语义都不是！

所以是什么导致他们存活着的？

malloc 实现有误？stack 引发的蝴蝶效应？还是整个页表实现就有致命缺陷？亦或者是那 kernel main 中所谓的初始化与清理工作压根就是表面功夫？

又或者——谁都掺上了一脚？

我想不清，究不明。杂乱的实现，四伏的隐患，让这所谓 **“只有很少的问题”** 的 miniOS 愈发恶心。

*“《重构是能带来光明，但只有重写才能真正拯救世界》”*

*“把页表重写了，重写才是正道妈的！”*

删除 pagetbl.c，创建 page.c，尘埃落定，所有莫名奇妙的错误顿时一扫而空。

至于是什么有问题，这还需要更进一步的证明吗？
