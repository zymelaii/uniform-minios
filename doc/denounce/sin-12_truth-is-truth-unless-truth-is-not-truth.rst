众所周知，true == true，除非 true == false
===========================================

**from kernel/hd.c:hd_rdwt,hd_rdwt_real**

.. code-block:: c

    500 Internal Server Error

在一段真正串行的逻辑中，是不存在并发问题的。原味 minios 中，“使用”单独的 hd_service 进程进行有关磁盘 IO 的所有操作。hd_service 使用一个队列对收到的 IO 请求串行地、阻塞地进行处理即响应，并在内部与不可重入的硬盘中断协同完整 IO 请求是否完成的确认。

看上去很值得信赖，一眼就令人信服如此磁盘 IO 的设计根本就不可能出现并发问题对吧？

我当初也是这样认为的。

直到它真的出现了并发问题。

直到我打开 Understand 查看了 hd_service 和 fs io 的蝶形依赖图。

直到我发现这逼养的 fs io 通通直接怼上了 hd_rdwt 而 hd_service 唯独通过 hd_rdwt_real 进行磁盘 IO 的处理……

嘿，您猜怎么着？这 hd_rdwt 还就是把 hd_rdwt_real 原封不动地抄了一份，一来二去整个 runtime hd_service 屁都没被鸟一下！

我玩尼玛呢？！

持宝刀而不用还是他妈的吃饱了撑着呢？！

这 TM fs io 一并发，连带着 hd_rdwt 也一起并发，连带着硬件端口读写也一起并发！我 TM 能跑这么一段时间才爆出莫名其妙的概率性玄学问题真是积了八辈子阴德！

呼哧呼哧——

如今一想，要真是依赖 hd_service 实现那还真不简单——请求信息你得维护吧？进程通讯你得实现吧？睡眠唤醒你得处理吧？IO 调度你得优化吧？

哈哈，就这么一个路一平坦就大兴爆破的 minishit，怎么可能会写出这种东西嘛！要真实现了，岂不就是鹤立鸡群，独树一帜？循着 mini\*\* 这秉性，来一手木秀于林风必摧之不过分吧？

我真是 \[鸟语花香.flac\]（以下省略三千字）
