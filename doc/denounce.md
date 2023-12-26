
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
char pathname[MAX_PATH];
strcpy(pathname,(char *)path);
pathname[pathlen] = 0;

int index = get_index(pathname);
if (index == -1) { return -1; }
int fd = vfs_table[index].op->open(pathname, flags);
```

乍一看，前面对 path 的拷贝显得很**安全**，看似多余但可以理解。然后再往后看你就会发现自己想错了——呵呵，这拷贝还真是一点都不多余，完善的让人想吐。

结合一下 get_index 的实现，get_index 不就是完成了**提取设备名称且匹配设备索引并将传参的绝对路径转换为相对于设备名的相对路径**吗？！很正常的实现，但是你有没有想过将这种操作描述为 get_index 堪称极致的误导啊！！

额外写一个转换相对路径的方法难道很难吗？明明就是甚至都不需要拷贝直接将绝对路径偏移 `strlen(devname)+1` 的就行了啊！

# 小猫是猫，熊猫也是猫，我们都是好猫

> from kernel/vfs.c:do_vcreatedir

```c
int do_vcreatedir(char *path) {
    int  state;
    int  pathlen = strlen(path);
    char pathname[MAX_PATH];
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
    char pathname[MAX_PATH];
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

我也能理解代码经手一伯万个人风格难免不统一的问题。

但是请问您能不能稍微注意一下引用的依赖关系问题？

你说 make 能直接编译通过，确实，是真的。

但是就好比你这 proc.h 里根本就没包含 protect.h 却正常地使用了只在 protect.h 里声明的 typedef？稍微认识到编译器不是万能的应该就能发现它不可能给你虚空引入符号吧？

所以你说你这编译为什么能通过？是 TM 因为你每个包含 proc.h 的源文件里在包含 proc.h 之前都直接或间接地包含了 protect.h 啊！你说它既然都兜了山路十八弯好不容易引入了它能不通过吗？拜托，您能通过编译大概率是靠运气啊，是靠上天眷顾啊！

我为了消除 warning 可是把所有 *.c *.h 都给改了啊！这意思是没有一个文件能独立地无伤过 linter 啊！

再者就是小提一嘴那诸天神佛都畏惧的混沌依赖关系。尽管把最多的引用都放到了源文件，但还是不可避免地出现了循环引用，最终还是不得不用前置声明来解决问题。

岂可修！
