猴子掰苞谷
===========

**from kernel/exec.c:sys_exec**

.. code-block:: c

    for (ph_num = 0; ph_num < Echo_Ehdr->e_phnum; ph_num++) {
        if (0 == Echo_Phdr[ph_num].p_memsz) { break; }
        if (Echo_Phdr[ph_num].p_flags
            & 0x1) // xx1，__E, executable seg must be code seg
        {          //.text
            exec_elfcpy(fd, Echo_Phdr[ph_num], PG_P | PG_USU | PG_RWR);
            p_proc_current->task.memmap.text_lin_base = Echo_Phdr[ph_num].p_vaddr;
            p_proc_current->task.memmap.text_lin_limit =
                Echo_Phdr[ph_num].p_vaddr + Echo_Phdr[ph_num].p_memsz;
        } else if (Echo_Phdr[ph_num].p_flags & 0x4) {
            exec_elfcpy(fd, Echo_Phdr[ph_num], PG_P | PG_USU | PG_RWW);
            p_proc_current->task.memmap.data_lin_base = Echo_Phdr[ph_num].p_vaddr;
            p_proc_current->task.memmap.data_lin_limit =
                Echo_Phdr[ph_num].p_vaddr + Echo_Phdr[ph_num].p_memsz;
        } else {
            vga_write_str_color("exec_load: unKnown elf'program!", 0x74);
            return -1;
        }
    }

一个 elf 执行的时候加载三四个 LOAD 段，你记录加载段的地址上下界限的时候加载一个 LOAD 改一次，等所有 LOAD 加载完了发现第一个 LOAD 段的地址上下限已经被最后一个 LOAD 段给改完了，好家伙好家伙，怪不得每次 fork 都会丢 data 段，原来父进程只保证了自己是对的，自己的 pcb 压根都没有记录全部的 LOAD 段信息，这让释放的时候怎么释放？子进程如何 fork，我的评价是 fork 不了一点。

假设我们的 ph 段只有两个 LOAD，好，很完美，都加载了，fork 也正常。但如果有三个，第三次循环的时候要么把 text_lin_base 覆盖掉，要么把 data_lin_base 和 data_lin_limit 覆盖掉。

这不就猴子掰苞谷，改一点丢一。。。。。。。。。。。。。十三点。
