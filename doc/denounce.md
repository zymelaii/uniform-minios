
感谢 GOOD 的 minios，让我一天高血压 $\infty$ 次。

为了方便后人更容易接触到高血压的真相，我在这里提出强烈谴责的清单！

# 神乎其神的传参与硬编码实现

> kernel/ktest.c:untar

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
