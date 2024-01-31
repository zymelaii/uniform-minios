提交规范
=========

为什么需要提交规范？
----------------------

1. 为了逼格：规范的 commit 信息能够让你的开发显得更专业。
2. 为了避免强迫症患者暴起刀人：规范的 commit 信息能或许能够安抚它们的情绪。
3. 为了规避无效劳动：敲一段规范清晰的 commit 信息能够让你知道你在这一次提交中究竟干了什么，你总不想花了力气敲了几个火星文上去，回过头来却还是要对着这段外星文明留下的通讯“若有所思”地摩挲下巴吧？
4. 为了能够更敏捷地梳理项目结构并进行项目管理：规范清晰的 commit 新能够在绝大部分场合直截了当地告诉你“这里有什么”“什么事情曾在这里发生过”，而不需要你再去累死累活地大规模手动翻阅源码，理清思路——小小的也很可爱。
5. 为了甩锅：哪一天项目突然罢工了，就去翻一翻提交历史记录，你可以像霸总眉头一皱，道一句“自己出来”，然后就会 commit 们就会站出来指认它们之中最可疑的那些家伙。到这一步你只需要眨巴眨巴眼睛，大底上就该知道哪个倒霉蛋需要背上“干崩项目”这口又大又黑的锅了。
6. 为了便于回溯与定位及修复错误：参照上一条，不会真有人甩了锅就想删库跑路吧？早知如此当初何必还要往程序开发这大坑跳呢？
7. 为了便于自动化管理：没毛病，只要 commit 信息或者其“规范/约定”本身足够规范清晰，自然而然地就能够衍生出自动化工具来强迫你规范你的提交记录或轻松地对 commit 数据进行进一步的处理，如生成版本号、导出 CHANGELOG.md 等等。
8. 为了减少开发误操作，提高容错率与整体开发效率：看了前面几条这还需要说？

一般格式
---------

.. code-block:: text

    <type>[(<scope>)][!]: <subject>

    [<body>]

    [<footer>]

提交的信息应至多由六部分组成，它们分别是：

- ``type`` 提交类型
- ``!`` 重要提交标识 \[可选]
- ``scope`` 提交的影响范围 \[可选]
- ``subject`` 提交内容简述
- ``body`` 提交内容补充说明 \[可选]
- ``footer`` 脚注 \[可选]

每一次提交应该至少包含 ``<type>: <subject>`` 的结构，如 ``fix: memory leak``、\ ``feat: buddy malloc``。

提交请依据一般格式严格控制提交信息的样式，如 **subject 前的单个空格**、\ **body 前的单个空行**。

在任何情况下，提交信息中都\ **不应该出现连续空行**；如果条件允许，应尽可能去除行末空白与末尾空行。

对于同一主题的附加信息，应当仅使用\ **换行处理 word wrap 或起新的段落**。

对于不同的主题——包括 body 与 footer 内部的不同主题——应当\ **使用单独的空行进行分隔**。

可选类型
---------

.. _commit_type_cheatsheet:

速查表
^^^^^^^

- :ref:`新增特性 | 新增功能 | 功能增强 <commit_type_feat>`
- :ref:`修复代码 BUG | 修复其它已知问题 <commit_type_fix>`
- :ref:`文档创建 | 文档删改 | 文档更新 <commit_type_doc>`
- :ref:`局部代码重构 | 接口调整 | 项目结构调整 <commit_type_refactor>`
- :ref:`代码样式变动 | 注释风格变动 | 其它格式变动 <commit_type_style>`
- :ref:`新增测试 | 测试修复 | 测试框架变动 | 测试配置变动 | 其它测试相关内容 <commit_type_test>`
- :ref:`构建工具变动 | 构建流程变动 | 项目配置变动 | 依赖项变动 | 版本变动 | 协议变动 | 其它杂项变动 <commit_type_chore>`
- :ref:`针对特定主体的预备变动 <commit_type_to>`
- :ref:`撤销提交 <commit_type_revert>`

.. _commit_type_feat:

:ref:`feat <commit_type_cheatsheet>`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

引入了新功能、新特性或对现有功能进行了增强，通常可以提升项目的 minor 版本号。

**subject 使用新功能、新特性的名词性描述，或简要描述变动的动作。**

*样例*

.. code-block:: text

    feat: buddy malloc
    feat: advanced bootloader
    feat: add tracing logging

.. _commit_type_fix:

:ref:`fix <commit_type_cheatsheet>`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

修补了一个已知代码漏洞或其它方面的问题，通常可以提升项目的 patch 版本号。

**subject 使用问题的名词性描述，或简要描述变动的动作。**

*样例*

.. code-block:: text

    fix: remove dirty flags
    fix: memory access violation
    fix: some bugs in mem free
    fix: logic of kill process

.. _commit_type_doc:

:ref:`doc <commit_type_cheatsheet>`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

对项目的文档进行了修正或更新。

**subject 简要描述文档变动的动作。**

*样例*

.. code-block:: text

    doc: add README.md
    doc: update design
    doc: fix mispelling

.. _commit_type_refactor:

:ref:`refactor <commit_type_cheatsheet>`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

对代码进行了重构性的改动，如局部代码重构、接口调整、项目结构调整等，\ **这些改动不应该涉及代码特性、功能的变更及漏洞的修复等**。

**subject 简要描述重构的动作。**

*样例*

.. code-block:: text

    refactor: move lock impl to stdlib
    refactor: rewrite untar
    refactor: remove dead code

.. _commit_type_style:

:ref:`style <commit_type_cheatsheet>`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

改变了现有内容的风格，如代码样式变动、注释风格变动及其它格式变动，\ **这些改动不应该影响原内容的功能、行为、特性、含义等等**。

**subject 简要描述风格变动的动作。**

*样例*

.. code-block:: text

    style: update format conf
    style: format impl.c

.. _commit_type_test:

:ref:`test <commit_type_cheatsheet>`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

进行了测试相关的变动，如新增单元测试，增强了单元测试，修复了单元测试中的问题，进行了测试框架的改动，调整了测试相关的配置或其它测试相关内容。

**subject 简要描述风格变动的动作。**

*样例*

.. code-block:: text

    test: use googletest framework
    test: add boundary tests
    test: fix dirty assertion
    test: enhance fs tests

.. _commit_type_chore:

:ref:`chore <commit_type_cheatsheet>`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

杂项变动，包括但不限于对项目构建及辅助工具相关的内容进行了更改，如构建系统的调整、外部依赖的调整等等，或版本变动、协议引入等等。

**subject 使用杂项的名词性描述，或简要描述杂项变动的动作。**

*样例*

.. code-block:: text

    chore: add source deps
    chore: import minios
    chore: update ws conf
    chore: intelligent mkfiles

特别地，当 git 仓库首次创建时，建议提交第一个 commit 为 ``chore: first commit``。在该次提交中通常包括项目目录的大致结构、README.md、.gitignore 等仓库配置文件及协议等内容。

.. _commit_type_to:

:ref:`to <commit_type_cheatsheet>`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

用于过渡未完成的变动，当更改未完成但是出于同步或其它目的需要临时提交时可以使用该类型。实际开发过程应当避免使用该类型，如若确实需要，应当确保更改已经完成的部分是重要的，且未完成的部分具有一定的复杂度。应用时，应当尽量剥离未完成的部分进行提交或确保未完成的部分不会对分支产生恶劣影响。

**subject 描述变动的主体。**

*样例*

.. code-block:: text

    to: prerelease v1.0.0
    to: patch for fs

.. _commit_type_revert:

:ref:`revert <commit_type_cheatsheet>`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

进行提交的回滚。

**subject 描述被回滚的提交的简述或简要描述回滚的内容。**

提交 revert 时，应在 footer 部分以 ``Refs: <hash> [, <hash>]...`` 的形式标注被回滚的提交的 hash 编码。若有需要，可以在 body 部分补充说明回滚的范围、内容或回滚的原因。

*样例*

回滚提交 ``025cfed fix: title``

.. code-block:: text

    revert: fix: title

    Refs: 025cfed

回滚提交 ``025cfed fix: title``、\ ``12e342b doc: add toc``

.. code-block:: text

    revert: misintroduced toc & title

    Refs: 025cfed, 12e342b

回滚多个提交并添加附加信息

.. code-block:: text

    revert: misintroduced toc & title

    Updates to the toc and titles are made to deprecated
    entries, and we need to roll back to the original
    document that matches the source code

    Refs: 025cfed, 12e342b

如何编写 subject？
---------------------------

subject 的具体内容见相应类型的说明及参考样例。

填写 subject 时，应至少遵从以下规则：

1. 使用全英描述
2. 自由进行恰当的缩写、省略以保证描述的简洁性与精炼程度
3. 以小写为主，避免不必要的大写
4. 句末不使用标点，并避免使用句中标点
5. 使用名词性描述或简述动作的主题，禁止引入主语
6. 主题尽量单一，避免引入多个主题描述

特别地，当一次提交中涵盖多种类型的内容变动时，应遵从以下方式进行处理，并在 body 中罗列并陈述各项变动：

1. 当变动可被总结时，将该总结陈述为 subject 并选用契合该总结的提交类型。
2. 当变动难以总结时，选取最关键的变动作为该次提交的 subject 依据并选用对应提交类型。

如何编写 scope？
---------------------------

scope 用于指明当前提交的影响范围，影响范围的类型依据具体提交而定，包括但不限于模块、方法、目录、文件、类型、范围。

当 scope 存在多个时，应当避免引入该项以确保提交简述的精简性。其它情况自行斟酌是否应当添加 scope 说明。

如何编写 body？
---------------------------

body 用于对提交进行补充说明，可以是提交变动的详情，也可以是变动的原因及目的。

提交者可以在必要时在该部分自由地添加需要补充的内容，但请注意确保 body 的\ **精简性**\ 与\ **表意功能**，避免引入冗杂的描述。

**请注意主动对 body 进行 word wrap，不要引入过长的单行信息。**

当需要编写多个主题的补充时，可以用一个空行进行分隔。

如何编写 footer？
---------------------------

footer 作为脚注应当总是以 ``<title>: <content>`` 进行呈现。如 revert 提交中的 ``Refs: 025cfed`` 就是一项脚注。

每一个提交的描述中可以包含多个脚注，对于较短的 footer，推荐以不空行的形式集中换行编写。

footer 可以由提交者主动添加，也可能由 GitHub、git 及其它途径自动生成。

当一次提交包含\ **重大变更**\ 时，可以使用 ``BREAKING CHANGE: <content>`` 进行脚注，content 中陈述\ **重大变更**\ 的关键。

此时提交的描述应当是这样的：

.. code-block:: text

    <type>[(<scope>)]!: <subject>

    [<body>]

    BREAKING CHANGE: <content>

    [<other-footers>]

使用\ **重大变更**\ 脚注时，应当同时使用 ``!`` 项；反之当使用 ``!`` 项时，可以不添加\ **重大变更**\ 脚注。

当 ``!`` 项被应用时，通常可以提升项目的 major 版本号。

如何关联 issue？
----------------

当某次提交用于处理或解决某个或某些 issue 时，应当主动在 subject 末尾附加 issue 的编号。此时提交简述应当符合以下格式：

.. code-block:: text

    <type>[(<scope>)][!]: <subject> (#<id>[, #<id>]...)

如 ``fix: title (#1)``、\ ``fix: title (#1, #8)``。
