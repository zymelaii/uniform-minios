
感谢 GOOD 的 minios，让我一天高血压 $\infty$ 次。

为了方便后人更容易接触到高血压的真相，我在这里提出强烈谴责的清单！

# 神乎其神的传参与硬编码实现

> from kernel/ktest.c:untar

```c
static void untar(const char *filename) {
    ...
    int fd = do_vopen(filename, O_RDWR);
    ...
    while (1) {
        ...
        char full_name[30] = "/orange/";
        strcat(full_name, phdr->name);
        int fdout = do_vopen(full_name, O_CREAT | O_RDWR);
        ...
    }
    ...
}
```

我就是说您老人家路径都传参了您就非得在实现里再硬编码一遍吗？您要是传参的是相对路径就算了，结果搁着传绝对路径呢？传绝对路径就算了，能不能在实现里尊重一下传参，就非得硬编码一下吗？传绝对路径作为参数的意义在哪？当作 BUG 的催化剂吗？

甚至还要在 Makefile 里加宏控制传入的参数来构造传参的绝对路径。你都知道用宏和传参来控制 untar 的文件了结果在实现里整这么一手，是觉得好玩吗？是觉得把天上宫阙和地上的屎混在一起很有艺术感吗？

# 只读？又读又写，暗度陈仓！

> from kernel/vfs.c:get_index

```c
static int get_index(char path[]) {
    int  pathlen = strlen(path);
    char fs_name[DEV_NAME_LEN];
    int  len = (pathlen < DEV_NAME_LEN) ? pathlen : DEV_NAME_LEN;
    int  i, a = 0;
    for (i = 0; i < len; i++) {
        if (path[i] == '/') {
            a = i;
            a++;
            break;
        } else {
            fs_name[i] = path[i];
        }
    }
    fs_name[i] = '\0';
    for (i = 0; i < pathlen - a; i++) path[i] = path[i + a];
    path[pathlen - a] = '\0';
    for (i = 0; i < NR_FS; i++) {
        if (vfs_table[i].fs_name == NULL) continue;
        if (!strcmp(fs_name, vfs_table[i].fs_name)) return i;
    }
    return -1;
}
```

哈，这个函数干了什么事呢？get_index，获取 path 对应的设备索引，非常正确！但是是不是代码太多且太乱了？

好，先看前一半，得了我知道你想拷贝第一个 "/" 之前的内容用来提取设备名，然后在 table 中逐个比较，得到 index。很好，很正常，虽然写的很烂但是可以容忍。

但是你这是什么玩意 `for (i = 0; i < pathlen - a; i++) path[i] = path[i + a];`，MD，改我实参是吧？你一个 get 系方法给我来一个 inout 型参数而且一点说明都没有？是觉得传参不加 const 已经是天大的暗示了吗？我真 \*\*\*\*\*。

来看看调用处是怎么写的：

```c
int pathlen = strlen(path);
char pathname[PATH_MAX];
strcpy(pathname,(char *)path);
pathname[pathlen] = 0;

int index = get_index(pathname);
if (index == -1) { return -1; }
int fd = vfs_table[index].op->open(pathname, flags);
```

乍一看，前面对 path 的拷贝显得很**安全**，看似多余但可以理解。然后再往后看你就会发现自己想错了——呵呵，这拷贝还真是一点都不多余，完善的让人想吐。

结合一下 get_index 的实现，get_index 不就是完成了**提取设备名称且匹配设备索引并将传参的绝对路径转换为相对于设备名的相对路径**吗？！很正常的实现，但是你有没有想过将这种操作描述为 get_index 堪称极致的误导啊！！

额外写一个转换相对路径的方法难道很难吗？明明就是甚至都不需要拷贝直接将绝对路径偏移 `strlen(devname)+1` 就行了啊！

# 小猫是猫，熊猫也是猫，我们都是好猫

> from kernel/vfs.c:do_vcreatedir

```c
int do_vcreatedir(char *path) {
    int  state;
    int  pathlen = strlen(path);
    char pathname[PATH_MAX];
    strcpy(pathname, path);
    pathname[pathlen] = 0;

    int index;
    index = (int)(pathname[1] - '0');
    for (int j = 0; j <= pathlen - 3; j++) { pathname[j] = pathname[j + 3]; }

    state = f_op_table[index].createdir(pathname);
    if (state != OK) { DisErrorInfo(state); }
    return state;
}
```

`index = (int)(pathname[1] - '0')`？你礼貌吗？你的 path 是金屋吗？是藏了娇吗这么牛逼？上来不管三七二十一反正第二个字符偏移 ASCII 0 就是设备的索引了？那你这 fs 的设计是得多严格，多牛逼啊？

甚至还不忘像 get_index 里那样拿了索引之后把路径转换为相对路径，你是想告诉我你是塔罗牌大师早在设计之时就已经预言到 path 的前缀一定是 {"/0/", "/1/", "/2/", "/3/", "/4/", "/5/", "/6/", "/7/", "/8/", "/9/"} 之一吗？

这么牛逼啊？那你能帮我算算我要是传入一个 "/orange/" 这个方法会怎么样吗？

哦对了提醒一下，f_op_table 的大小不过也就 3 哦，所以会发生什么呢？

是数组越界，是大概率的 Segment Fault 啊！！

倒是看看同胞方法都是怎么写的：

```c
int do_vdelete(char *path) {
    int pathlen = strlen(path);
    char pathname[PATH_MAX];
    strcpy(pathname,path);
    pathname[pathlen] = 0;
    int index;
    index = get_index(pathname);
    if (index == -1){ return -1; }
    return vfs_table[index].op->delete(pathname);
}
```

人家都是老老实实地通过 get_index 从 vfs_table 匹配索引，怎么到你这里就这么粗暴了呢？是谁给你的勇气？梁静茹吗？

就是说就算这是你神乎其神的设计能不能来个有点用的注释或文档而不是代码里 `added by` `modified by` `deleted by` 满天飞？！

# 过程错又怎么样？我结果不是对了吗？你凭什么说我！

> from *.h *.c

我能理解在 VSCode 里给这种古老的 Makefile 项目配置 linter 不是一件手到擒来的事。

我也能理解代码经手一伯万个人风格难免不统一。

但是请问您能不能稍微注意一下引用的依赖关系问题？

你说 make 能直接编译通过，确实，是真的。

但是就好比你这 proc.h 里根本就没包含 protect.h 却正常地使用了只在 protect.h 里声明的 typedef，稍微认识到编译器不是万能的应该就能发现它不可能给你虚空引入符号吧？

所以你说你这编译为什么能通过？是 TM 因为你每个包含 proc.h 的源文件里在包含 proc.h 之前都直接或间接地包含了 protect.h 啊！你说它既然都兜了山路十八弯好不容易引入了它能不通过吗？拜托，您能通过编译大概率是靠运气啊，是靠上天眷顾啊！

我为了消除 warning 可是把所有 *.c *.h 都给改了啊！这意思是没有一个文件能独立地无伤过 linter 啊！

再者就是小提一嘴那诸天神佛都畏惧的混沌依赖关系。尽管把最多的引用都放到了源文件，但还是不可避免地出现了循环依赖，最终还是不得不用前置声明来解决问题。

岂可修！

# 猴子掰苞谷

> from kernel/exec.c:sys_exec

```c
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
```

一个 elf 执行的时候加载三四个 LOAD 段，你记录加载段的地址上下界限的时候加载一个 LOAD 改一次，等所有 LOAD 加载完了发现第一个 LOAD 段的地址上下限已经被最后一个 LOAD 段给改完了，好家伙好家伙，怪不得每次 fork 都会丢 data 段，原来父进程只保证了自己是对的，自己的 pcb 压根都没有记录全部的 LOAD 段信息，这让释放的时候怎么释放？子进程如何 fork，我的评价是 fork 不了一点。

假设我们的 ph 段只有两个 LOAD，好，很完美，都加载了，fork 也正常。但如果有三个，第三次循环的时候要么把 text_lin_base 覆盖掉，要么把 data_lin_base 和 data_lin_limit 覆盖掉。

这不就猴子掰苞谷，改一点丢一。。。。。。。。。。。。。十三点。

# 这段要反转一下！你说的对，但是

> from kernel/fork.c:fork_mem_copy

```c
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
```

行行行我知道栈是向低地址生长的也看到你的 `addr_lin -= num_4K` 了。

但是，就是说，你晓不晓得 memcpy 是从低地址向高地址拷贝的？

你这是 clone 栈区 \[stack_lin_limit, stack_lin_base) 吗？你这不分明在 clone (stack_lin_limit+4K, stack_lin_base+4K] 吗？！又缺斤短两又酒里掺水了啊喂！

# 你不懂，糊弄同事有助于涨业绩！

> from kernel/pagetbl.c:vmalloc

```c
u32 vmalloc(u32 size) {
    u32 temp;
    if (p_proc_current->task.info.type == TYPE_PROCESS) { // 进程直接就是标识
        temp = p_proc_current->task.memmap.heap_lin_limit;
        p_proc_current->task.memmap.heap_lin_limit += size;
    } else { // 线程需要取父进程的标识
        temp = *((u32 *)p_proc_current->task.memmap.heap_lin_limit);
        (*((u32 *)p_proc_current->task.memmap.heap_lin_limit)) += size;
    }
    return temp;
}
```

已知 heap_lin_limit 的类型是 u32，并且 thread 的 head 是指向父进程/主线程的 head ptr。

> 子线程与主线程共用堆，但是变量独立。为了维护堆，在此实现中子线程通过指向主线程 heap limit 的指针完成同步。

你自己瞅瞅，你写的是人话吗？

加这么点破烂添头你有工资拿吗？

除了恶心自己恶心别人还有什么屁用吗？

我不苛求你把结构改良，你直接把注释删了不行吗？还`需要获取父进程的标识`……

你这获取的是父进程的标识吗？！难道你自己都不清楚自个儿是怎么实现父子线程的堆共享的吗？你需要注释的是`这个 limit 其实是指向主线程 heap limit 的指针`而不是`获取父进程的标识`啊！

# 页面是 4K 对齐的！所以整页整页拷贝就行了

> from kernel/fork.c:fork_mem_copy

```c
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
```

承接上文**这段要反转一下！你说的对，但是**，fork_mem_copy 在经历千锤百炼之后再次在希望的终点给予我们背刺！

简单修复前文中所提的方向的问题，可以将代码修改如下：

```c
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
```

很对吧！

看上去确实很对，栈的问题修复了，可能会错的地方也注意到了。

但是好了嘛！？

大抵确实是好了吧，毕竟整个代码里很多其它类似的地方也是这么干的，不也是正常的跑了吗？再说了，既然原来偏移了 0x1000 的代码能正常跑，那我只要把所有跟 stack addr 有关的地方全部把 0x1000 偏移的问题修复了，~~根据平移不变性~~那不就是很显然地也能正常运行吗？！

原神，启动！！

> 叮！你收获了一个 page fault！

不可能，这绝对不可能！！！简单查下 stack limit 的 pte——没错啊，这不是存在吗？！属性值不都对着吗？！

……

……

……

屮，stack base 的 pte 怎么没有？！往前 4B，也没有！往前 0x1000，好，存在！！——MD，少拷贝一页！

再回过来看代码——不对，是先看看 stack 的 base 和 limit。曾经很偶然地瞥到了 `#define StackLinBase (ArgLinBase - num_4B)`，当时只道是莫名其妙，但又道无伤大雅，并不会影响到代码的执行。可现今一看，如何不能！

当万千错误集结到一起汇聚出一个**恰巧**能够正常运行的代码，那修复他都是一种莫大的罪过！

在其它情况下，我重写了 range mapping 的方法，并充分处理了页对齐的问题，故而 elf LOAD 倒是十分顺利地运行了。但在 vpage、heap、arg、stack 等 laddr 手动编排的内存空间上，则是在 fork_mem_copy 中利用共享页便利化拷贝并将物理页转移给子进程。按理说这些玩意应该是同生共死的，可细细一查，怎么会只有 stack 出问题了呢？

一瞅——就健壮性而言，fork_mem_copy 理所应当要寄！laddr 整页递增，那要是抵达 limit 时 limit 没有与页面尺寸对齐怎么办？页内偏移 `0 ~ limit & 0xfff` 的这一部分难道就抛弃嘛！那必然还是要再申请一页的！如此情形不处理，stack 缺页不是理所应当！

但为什么 vpage、heap、arg 这些没寄？（一方面是因为这时候压根就没 pte，所以怎么想都寄不了啊~）看看人家，他们的 base 和 limit 可都是 4K 对齐的啊！base 即是页边界，limit 即是最后一页的边界，这一来二去，边界对边界，自然不会有缺页的问题。所以他们寄了吗？**运气好没寄罢了！**

倘若不是一手 `StackLinBase = ArgLinBase - num_4B` 把 4K 对齐的特性打得稀烂，估计这等差错还要再延后或者到底还是让人察觉不了——有多少人会斟酌一段实在令人作呕又刚好能够正常运行的代码呢？

再去看看偏移了 0x1000 的旧实现，那果真没问题吗？

自然是巧合下的又一个巧合！

**巧合在 base 处必然会有 pte！**

**巧合在 stack 的大小足够大，简单的用户程序难以令 esp 接近 limit 而触发 limit 处的缺页！**

**巧合在 esp 并非是在真正的栈顶，而是在距真正的栈顶 0x1000 处的低地址位置！**

从一开始就没有正确的算法实现！

**“页面是 4K 对齐的！所以整页整页拷贝就行了”**，这对吗？这自然可以是对的，但没有满足边界地址页对齐，又谈何正确！终究不过是在一环套一环的谬误中苟活的虚伪 **PASS** 罢了！

一朝败露，便忘恩负义地杀得人片甲不留，又金蝉脱壳了去！

# 线性地址？物理地址？能跑就行！

> from *.c

```c
buf = (PBYTE)K_PHY2LIN(do_kmalloc(Bytes_Per_Sector * sizeof(BYTE)));
...
do_free(buf);
```

以及队友被 miniOS 带偏后，按照其写法增加的新功能实现片段：

```c
Elf32_Ehdr* elf_header = NULL;
Elf32_Phdr* elf_proghs = NULL;
elf_header             = do_kmalloc(sizeof(Elf32_Ehdr));
assert(elf_header != NULL);
read_Ehdr(fd, elf_header, 0);
elf_proghs = do_kmalloc(sizeof(Elf32_Phdr) * elf_header->e_phnum);
assert(elf_proghs != NULL);
for (int i = 0; i < elf_header->e_phnum; i++) {
    u32 offset = elf_header->e_phoff + i * sizeof(Elf32_Phdr);
    read_Phdr(fd, elf_proghs + i, offset);
}

if (exec_load(fd, elf_header, elf_proghs) == -1) {
    do_close(fd);
    do_free(elf_header);
    do_free(elf_proghs);
    return -1;
}
```

> PS: 以上关乎原汁原味 miniOS malloc 和 free 由于过于古早，回溯较为麻烦，此处使用 uniform-minios 的 syscall、malloc 修正版作为参考代码，但保证谴责目标依旧原汁原味！

众所周知，`void *ptr = malloc(size); ... free(ptr);` 是一个内存管理的范式。

但某个 miniOS 显然不是这么想的。

首先的首先，miniOS 用户态的 malloc 是没用的，只在内核态下使用系列方法进行内存管理。

> 关于用户态的 malloc 为什么用不了，简单点来说是 heap 没有映射页面，但这之中涉及到一些更需要谴责的恶劣操作，故此处先不深入展开。

然而说是内存管理，其实此系列方法实现的是对物理地址区间的管理。只不过仰赖于内核页面总是可用的，故可以通过 K_PHY2LIN 从分配得到的物理地址直接访问对应的内核空间线性地址，继而完成访存操作。

好戏就是这么开场的。

第一，此 malloc 系列返回的是物理地址，**在页表机制的作用下，物理地址是不能被访问的！**除非页表中有该与该物理地址等值的线性地址映射。

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

我想不清，究不明。杂乱的实现，四伏的隐患，让这所谓**“只有很少的问题”**的 miniOS 愈发恶心。

> “《重构是能带来光明，但只有重写才能真正拯救世界》”
>
> “把页表重写了，重写才是正道妈的！”

删除 pagetbl.c，创建 page.c，尘埃落定，所有莫名奇妙的错误顿时一扫而空。

至于是什么有问题，这还需要更进一步的证明吗？

# 页表：梅开三度！我就是全场最靓的宰

> from kernel/console.c:w_copy

```c
static void w_copy(unsigned int dst, const unsigned int src, int size) {
    memcpy(
        (void*)(V_MEM_BASE + (dst << 1)),
        (void*)(V_MEM_BASE + (src << 1)),
        size << 1);
}
```

简而言之，言而简之，这件事我只能说懂得都懂，不懂的呢也别趟这趟混水了，乐呵乐呵就完事了。

**前情提要：逆天页表独领风骚**

在一些名叫**如能跑**的神秘学代码的作用下，旧页表成功地实现了为线性地址 0 映射了物理页面。

由之前的谴责可以得知，咱的 kernel 是压根没有访问线性地址 0 的去处的，这 0 的映射要么是旧页表的错误映射，要么是 loader 阶段建立的映射没有清除——或者说，**如清除**。

于是到了 console 的实现里，就会出现这么一个匪夷所思的 BUG：在看上去十分正确的功能实现下，当 console 需要翻页时，实现分明正确拷贝翻页了，可屏幕上却偏偏会兜兜转转绕一圈将最初的第 0 行所在的那一屏重新打印出来。

莫非是在这实现的某个角落里存在极难察觉的坐标设置问题，使得它周期性翻转了？又或者干脆是没有清空显存？

不晓得，时间有限，在前期粗浅修了 tty 便没再去管这 console 了，毕竟是无伤大雅的事情。

可是在最新的页表全面重写后，一些新的蜜汁问题却又蹦了出来。关于此前文已有样例，但此处且就单单盯着这 console，来一次旧活新整。

**请，再看它一眼！**

```c
static void w_copy(unsigned int dst, const unsigned int src, int size) {
    memcpy(
        (void*)(V_MEM_BASE + (dst << 1)),
        (void*)(V_MEM_BASE + (src << 1)),
        size << 1);
}
```

**再来瞅瞅别个去处？**

```c
static void clear_screen(int pos, int len) {
    u8* pch = (u8*)K_PHY2LIN(V_MEM_BASE + pos * 2);
    while (--len >= 0) {
        *pch++ = ' ';
        *pch++ = (u8)DEFAULT_CHAR_COLOR;
    }
}
```

> 问题是什么？

MLGB 的，这显存基址分明就不是个线性地址啊！

> 那他为什么能访问呢？

旧页表有大恐怖啊，他搁那线性地址 0 附近的地方有页表项，**恰巧**访问不会缺页呀！

> 那岂不就是说？——

是极是极！这玩意压根写的内存就不对，只不过恰好没原地爆炸升天罢了！再看那功能实现——操作都操作到别人家里去了，没被人家打死已是踩了狗屎运，哪里还敢苛求个富贵！

w_copy？呵！**如调用**罢了！

不过话又说回来，将这地址转换了去，内核处的线性地址便是对吗？

确实，访存一致了，页表页正确了，功能等诸多方面也着实是无误了。

可莫要忘了这 VGA 显存的基址为 0x000b8000，如今却要教这诸多外因转换去了 0xc00b8000，如此依旧是对吗？

大抵是众口难一。

我是觉得，既然已经将这规矩打破，要么便破碎个彻底，让着基址随波逐流，莫要再赖皮着个 0x000b8000；要么就把规矩重新树立好，将这 0x000b8000 给好好映射到那一般模样的线性地址，别再整个**吃着碗里，看着锅里**的笑话了！

# 噩梦小插曲，内存恐慌

> from ???

```c
404 Not Found
```

按照 denounce 的评判标准来说，这一段其实不应该被包含进来，但是实在是憋不住了，不顺带提一嘴心里堵得慌。

miniOS 的基准代码中贴心地附上了内存模型，但殊不知这正是噩梦的始作俑者。

先贴几个补充事实：

1. 可以为链接器指定各个段的基地址
2. 静态可重定向文件各个符号的相对定位是固定的，无法或者说难以手动指定
3. 默认情况下，链接器按给出的目标文件列表的顺序进行链接操作
4. 对于每一个目标文件，链接器在对应段上顺序定位，先链接的在段的低地址处，后链接的在段的高地址处
5. 可以强制指定符号的地址，但这不会影响其余符号的重定位，若符号对应的数据区间存在重叠，可能引发不可预估的错误

好了，再回到基准代码。

在设计中，loader 被加载到物理地址 0x00090000 处，而 kernel elf 将被加载到物理地址 0x00080000 处。

内核的 text 段基址指定为 0xc0030400，在内存模型中，kernel runtime 的内存区间被编排在 0x00030000~0x00080000，也即 elf 段的安全加载区间应该在 0xc0000400~0xc0080000。

那么问题来了，kernel 的 elf 加载多少谁说了算？内存模型说了不算，elf 自己说了算。

初始的内核栈在 bss 段中定义，也即在 text 和 data 段之后。

若不做额外的处理，也即 text、data、bss 三个段是紧邻的，假定系统严格按照给出的内存模型操作，那么其稳定运行的充分条件应当是**内存模型正确**。

那么它正确吗？前面提到了，这事得由 elf 说了算。

loader 的起始地址附近存储了 gdt，该部分将在 kernel 中被拷贝至内核空间，到此 loader 的任务彻底结束，其内存空间可以自由用于其它用途。

而放置 kernel.bin 的区间，由于 load 段的加载地址紧密排列，所以 kernel elf 加载至内核空间时一定不存在差错。

可 loader 就不一样了，放置 kernel.bin 的内存空间被覆写也无所谓，因为加载一定是正确的，可 loader 的任务要约莫执行到 kernel_main 才算结束，在这之前对 loader 内存空间的任意非法内存写都是潜在的致命因素。

那么如果 elf 中 load 段的符号越过了 0xc0090000 会发生什么？将会与 loader 重叠！并且将直接重叠 loader 中待被拷贝的 gdt 数据段！

如果越过的是 text 或 data 段，那么 gdt 将被直接破坏。

但相对于 text 或 data 段，bss 段显然更容易越过 loader 的内存空间。并且由于 bss 段不清空的处理，覆写的问题将更加隐蔽。或许立即被写，或许延迟一段时间被写，又或许永远都不会被写。在后两种情形下，出现问题时调试的难度远远高于前者，等问题出现时，上下文大概往往是“貌似”与真正有问题的地方八竿子打不着。

> loader 中将 0xc0000000 映射至物理地址 0x00000000

这时候再回过头来看看**补充事实的第 4 点**：地址依据链接顺序顺序编排。

在原始的 Makefile 里，kernel.asm 被安排在第一个链接项，栈区被安排在此目标文件的 bss 段偏移 0 位置，也即在加载时会被加载到 kernel.bin bss 段的基址上。

现在的 kernel 并不那么大，bss 基址几乎总是可以落在 0xc0040000 附近的地方。

可如果 kernel.asm 的目标文件被最后链接呢？在 `uniform-minios >= 0.5.1` 的某一个版本中，内核栈顶直接干到了 0xc0090140 左右的位置，而 loader 中的 gdt 基址则是 0x90102——他们是在一个物理页面上！

这意味着什么？可能无事发生，也可能只不过是一个 call 指令把返回地址压栈，gdt 就被写了！

如果这件事在 gdt 拷贝完成之前发生，那么加载了错误的 gdt 会发生什么简直难以想象。

可能是直接反复干穿 bios，也可能只是无事发生，更可能是干穿一次 bios 后然后又无事发生地正常进入 kernel_main。

> 以下省略三千字……

这个问题好解决吗？意识到是 gdt 出问题了，那简直不要太简单，loader gdt 的地址是固定的，直接 gdb 打个 watch 分分钟就能定位覆写 gdt 的位置。可要是没有意识到，那可就真是怀疑天怀疑地怀疑空气，出错代码附近的玩意但凡沾亲带故的都得通通排查过去。

> 当然如果对整个系统具有极强掌控力和极高自信心，那就算一开始没意识到 gdt 的问题也应该能第一时间排除掉大部分可能。

截至 v0.6.0，uniform 依旧**暂时**没有优雅的办法处理越界的问题，而是简单地强制 kernel.asm 首先被链接来先缓和下问题，凑活过过日子。

总不能，每次 kernel 大小变了我都要人力瞅一眼再到处爆改常量吧？

那可真是——既怠惰又不优雅。
