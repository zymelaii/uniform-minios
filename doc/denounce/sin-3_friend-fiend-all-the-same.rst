小猫是猫，熊猫也是猫，我们都是好猫
==================================

**from kernel/vfs.c:do_vcreatedir**

.. code-block:: c

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

``index = (int)(pathname[1] - '0')``？你礼貌吗？你的 path 是金屋吗？是藏了娇吗这么牛逼？上来不管三七二十一反正第二个字符偏移 ASCII 0 就是设备的索引了？那你这 fs 的设计是得多严格，多牛逼啊？

甚至还不忘像 get_index 里那样拿了索引之后把路径转换为相对路径，你是想告诉我你是塔罗牌大师早在设计之时就已经预言到 path 的前缀一定是 ``{"/0/", "/1/", "/2/", "/3/", "/4/", "/5/", "/6/", "/7/", "/8/", "/9/"}`` 之一吗？

这么牛逼啊？那你能帮我算算我要是传入一个 "/orange/" 这个方法会怎么样吗？

哦对了提醒一下，f_op_table 的大小不过也就 3 哦，所以会发生什么呢？

是数组越界，是大概率的 Segment Fault 啊！！

倒是看看同胞方法都是怎么写的：

.. code-block:: c

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

人家都是老老实实地通过 get_index 从 vfs_table 匹配索引，怎么到你这里就这么粗暴了呢？是谁给你的勇气？梁静茹吗？

就是说就算这是你神乎其神的设计能不能来个有点用的注释或文档而不是代码里 ``added by`` ``modified by`` ``deleted by`` 满天飞？！
