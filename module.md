# Linux内核模块
    <<Linux设备驱动开发详解：基于最新的Linux 4.0内核>>


### 内核模块简介
    Linux模块机制：
    1. 模块本身不被编译入内核映像，从而控制了内核的大小；
    2. 模块一旦被加载，它就和内核中的其它部分完全一样；

    lsmod命令可以获得系统中已加载的所有模块以及模块间的依赖关系；
    lsmod命令实际上是读取并分析“/proc/modules”文件；
    内核中已加载模块的信息也存在于/sys/module目录下，对应于一个目录，
    eg. /sys/module/hello，该目录下又有refcnt文件和sections目录；

    加载/卸载模块：
    insmod xxx.ko / rmmod xxx
    modprobe xxx / modprobe -r xxx

    modprobe在加载某模块时，会同时加载该模块所依赖的其它模块；卸载时将同时卸载其依赖的模块；
    模块之间的依赖关系存放在根文件系统的/lib/modules/<kernel-version>/modules.dep文件中，
    实际上是在整体编译内核的时候由depmod工具生成的；

    modinfo命令可以获得模块的信息，包括模块作者、模块的说明、模块所支持的参数以及vermagic，
    eg. modinfo xxx.ko；


### 内核模块程序结构
    1. 模块加载函数
    当通过insmod或modprobe命令加载内核模块时，模块的加载函数会自动被内核执行，完成模块的相关初始化工作；
    2. 模块卸载函数
    当通过rmmod命令卸载模块时，模块的卸载函数会自动被内核执行，完成与模块卸载函数相反的功能；
    3. 模块许可证声明
    许可证（LICENSE）声明描述内核模块的许可权限，
    如果不声明LICENSE，模块被加载时，将收到内核被污染（Kernel Tainted）的警告；
    ”GPL“、”GPL v2“、”GPL and additional rights“、”Dual BSD/GPL“、”Dual MPL/GPL、”Proprietary“；
    4. 模块参数（可选）
    模块参数是模块被加载的时候可以传递给它的值，它本身对应模块内部的全局变量；
    5. 模块导出符号（可选）
    内核模块可以导出的符号（symbol，对应于函数或变量），若导出，其它模块则可以使用该模块中的函数或变量；
    6. 模块作者等信息声明（可选）


### 模块加载函数
    内核模块加载函数一般以 __init标识声明：
    static int __init initialization_function(void)
    {
        /* codes */
        return xxx;
    }
    module_init(initialization_function);

    在Linux内核中，可以使用 request_module(const char *fmt, ...)函数加载内核模块；

    在Linux中，所有标识为 __init的函数如果直接编译进入内核，成为内核镜像的一部分，
    在连接的时候会放在 .init.text这个区段内；
    #define __init      __attribute__ ((__section__(".init.text")))

    所有的 __init函数在区段 .initcall.init中还保存了一份函数指针，
    在初始化时内核会通过这些函数指针调用这些 __init函数，
    并在初始化完成后，释放init区段（包括.init.text、.initcall.init等）的内存；

    数据可以被定义为 __initdata，对于只是初始化阶段需要的数据，内核在初始化完后，也可以释放它们占用的内存；
    eg. static int hello_data __initdata = 1;


### 模块卸载函数
    内核模块卸载函数一般 __exit标识声明：
    static void __exit cleanup_function(void)
    {
        /* codes */
    }

    用 __exit来修饰模块卸载函数，可以告诉内核如果相关的模块被直接编译进内核，
    则 cleanup_function()函数会被忽略，直接不链进最后的镜像；
    只是退出阶段采用的数据也可以用 __exitdata来修饰；


### 模块参数
    1. 为模块定义一个参数：
    module_param(参数名, 参数类型, 参数读/写权限);
    2. 为模块定义一个参数数组：
    module_param_array(数组名, 数组类型, 参数读/写权限);

    在装载内核模块时，用户可以向模块传递参数：
    insmod/modprobe 模块名 参数名=值
    如果不传递，参数将使用模块内定义的缺省值；
    运行insmod或modprobe命令时，应使用逗号分隔输入的数组元素；

    如果模块被内置，就无法insmod，但是bootloader可以通过在bootargs里
    设置“模块名.参数名=值”的形式给该内置的模块传递参数；

    参数的类型可以时byte、short、ushort、int、uint、long、ulong、charp（字符指针）、
    bool、invbool（布尔的反）；
    在模块被编译时会将module_param中声明的类型与变量定义的类型进行比较，判断是否一致；

    eg.
    static char *book_name = "dissecting Linux Device Driver";
    module_param(book_name, charp, S_IRUGO);
    static int book_num = 4000;
    module_parm(book_num, int, S_IRUGO);

    模块被加载后，在/sys/module/目录下将出现以此模块名命令的目录；
    若“参数读/写权限”为0时，表示此参数不存在sysfs文件系统下对应的文件节点；
    若“参数读/写权限”不为0时，在此模块的目录下还将出现parameters目录，其中包含一系列以参数名命令的文件节点，
    这些文件的权限值就是传入module_param()的“参数读/写权限”，而文件的内容为参数的值；


### 导出符号
    “/proc/kallsyms”文件对应着内核符号表，它记录了符号以及符号所在的内存地址；

    模块可以使用如下宏导出符号到内核符号表中：
    EXPORT_SYMBOL(符号名);
    EXPORT_SYMBOL_GPL(符号名);     // 只适用于包含GPL许可权的模块


### 模块声明与描述
    MODULE_AUTHOR(author);              // 作者
    MODULE_DESCRIPTION(description);    // 描述
    MODULE_VERSION(version_string);     // 版本
    MODULE_DEVICE_TABLE(table_info);    // 设备表
    MODULE_ALIAS(alternate_name);       // 别名

