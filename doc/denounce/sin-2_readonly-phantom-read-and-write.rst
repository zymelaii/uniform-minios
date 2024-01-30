只读？又读又写，暗度陈仓！
==========================

**from kernel/vfs.c:get_index**

.. code-block:: c

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

哈，这个函数干了什么事呢？get_index，获取 path 对应的设备索引，非常正确！但是是不是代码太多且太乱了？

好，先看前一半，得了我知道你想拷贝第一个 "/" 之前的内容用来提取设备名，然后在 table 中逐个比较，得到 index。很好，很正常，虽然写的很烂但是可以容忍。

但是你这是什么玩意 ``for (i = 0; i < pathlen - a; i++) path[i] = path[i + a];``，MD，改我实参是吧？你一个 get 系方法给我来一个 inout 型参数而且一点说明都没有？是觉得传参不加 const 已经是天大的暗示了吗？我真 \*\*\*\*\*。

来看看调用处是怎么写的：

.. code-block:: c

    int pathlen = strlen(path);
    char pathname[PATH_MAX];
    strcpy(pathname,(char *)path);
    pathname[pathlen] = 0;

    int index = get_index(pathname);
    if (index == -1) { return -1; }
    int fd = vfs_table[index].op->open(pathname, flags);

乍一看，前面对 path 的拷贝显得很 **安全**，看似多余但可以理解。然后再往后看你就会发现自己想错了——呵呵，这拷贝还真是一点都不多余，完善的让人想吐。

结合一下 get_index 的实现，get_index 不就是完成了 **提取设备名称且匹配设备索引并将传参的绝对路径转换为相对于设备名的相对路径** 吗？！很正常的实现，但是你有没有想过将这种操作描述为 get_index 堪称极致的误导啊！！

额外写一个转换相对路径的方法难道很难吗？明明就是甚至都不需要拷贝直接将绝对路径偏移 ``strlen(devname)+1`` 就行了啊！
