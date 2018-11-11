# Linux文件系统与设备文件
    file system and device files
    <<Linux设备驱动开发详解：基于最新的Linux 4.0内核>>


### Linux文件系统

#### Linux文件系统目录结构
    /bin
    包含基本命令，如 ls、cp、mkdir等，这个目录中的文件都是可执行的；

    /sbin
    包含系统命令，如 modprobe、hwclock、ifconfig等，大多是涉及系统管理的命令，这个目录中文件都是可执行的；

    /dev
    设备文件存储目录，应用程序通过对这些文件的读写和控制以访问实际的设备；

    /etc
    系统配置文件的所在地，一些服务器的配置文件也在这里，如用户账号及密码配置文件；
    busybox的启动脚本也存放在该目录；

    /lib
    系统库文件存放目录；

    /mnt
    这个目录一般是用来存放挂载存储设备的目录；可以参看/etc/fstab的定义；
    有时可以让系统开机自动挂载文件系统，并挂载点放在这里；

    /opt
    opt是“可选”的意思，有些软件包会被安装在这里；

    /proc
    操作系统运行时，进程及内核信息（比如CPU、硬盘分区、内存信息）存放在这里；
    /proc目录为伪文件系统proc的挂载目录，proc并不是真正的文件系统，它存在与内存之中；

    /tmp
    用户运行程序的时后，有时会产生临时文件，/tmp用来存放临时文件；

    /usr
    这个是系统存放程序的目录，比如用户命令、用户库等；

    /var
    var表示的是变化的意思，这个目录的内容经常变动，如/var/log目录被用来存放系统日志；

    /sys
    Linux 2.6以后的内核所支持的sysfs文件系统被映射在此目录上；
    Linux设备驱动模型中的总线驱动和设备都可以在sysfs文件系统中找到对应的节点；
    当内核检测到在系统中出现了新设备后，内核会在sysfs文件系统中为该新设备生成一项新的记录；

#### Linux文件系统与设备驱动
    应用程序和VFS之间的接口是系统调用，
    而VFS与文件系统以及设备文件之间的接口是 file_operations 结构体成员函数，
    这个结构体包含对文件进行打开、关闭、读写、控制的一系列函数；

    由于字符设备的上层没有类似于磁盘的ext2等文件系统，所以字符设备的file_operations成员函数
    就直接由设备驱动提供，这是字符设备驱动的核心；

    块设备有两种访问方法：
    1. 不通过文件系统直接访问裸设备，在Linux内核实现了统一的 def_blk_fops这一file_operations，
    它的源代码位于fs/block_dev.c;
    eg. dd if=/dev/sdb1 of=sdb1.img
    2. 通过文件系统来访问块设备，file_operations的实现则位于文件系统内，
    文件系统会把针对文件的读写转换为对块设备原始扇区的读写；
    ext2、fat、Btrfs等文件系统中会实现针对VFS的file_operations成员函数，
    设备驱动层将看不到file_operations的存在；

    在设备驱动程序的设计中，一般而言，会关心 file 和 inode 这两个结构体；

##### file 结构体
    file结构体代表一个打开的文件，系统中每个打开的文件在内核空间都一个关联struct file；
    它由内核在打开文件时创建，并传递给文件上进行操作的任何函数；
    在文件的所有实例都关闭后，内核释放这个数据结构；
    在内核和驱动源代码中，struct file的指针通常被命名为file或filp（即 file pointer）；

    struct file {
        union {
            struct llist_node        fu_llist;
            struct rcu_head          fu_rcuhead;
        } f_u;
        struct path                  f_path;
    #define f_dentry    f_path.dentry
        struct inode                 *f_inode;  /* cached value */
        const struct file_operations *f_op;     /* 和文件关联的操作 */

        /*
         * Protects f_ep_links, f_flags.
         * Must not be taken from IRQ context.
         */
        spinlock_t                   f_lock;
        atomic_long_t                f_count;
        unsigned int                 f_flags;   /* 文件标志，如 O_RDONLY、O_NONBLOCK、O_SYNC */
        fmode_t                      f_mode;    /* 文件读/写模式，FMODE_READ和FMODE_WRITE */
        struct mutex                 f_pos_lock;
        loff_t                       f_pos;     /* 当前读写位置 */
        struct fown_struct           f_owner;
        const stuct cred             *f_cred;
        struct file_ra_state         f_ra;

        u64                          f_version;
    #ifdef CONFiG_SECURITY
        void                         *f_security;
    #endif
        /* needed for tty driver, and maybe others */
        void                         *private_data; /* 文件私有数据 */

    #ifdef CONFiG_EPOLL
        /* Used by fs/eventpoll.c to link all the hooks to this file */
        struct list_head             f_ep_links;
        struct list_head             f_tfile_llink;
    #endif
        struct address_space         *f_mapping;
    } __attribute__((aligned(4)));

    文件读/写模式mode、标志f_flags都是设备驱动关心的内容，而私有数据指针private_data在设备驱动中被广泛应用，
    大多被指向设备驱动自定义以用于描述设备的结构体；

##### inode 结构体
    VFS inode包含文件访问权限、属主、组、大小、生成时间、访问时间、最后修改时间等信息；
    它是Linux管理文件系统的最基本单位，也是文件系统连接任何子目录、文件的桥梁；

    struct inode {
        ...
        umode_t         i_mode;     /* inode的权限 */
        uid_t           i_uid;      /* inode拥有者的id */
        gid_t           i_gid;      /* inode所属的群组id */
        dev_t           i_rdev;     /* 若是设备文件，此字段将记录设备的设备号 */
        loff_t          i_size;     /* inode所代表的文件大小 */

        struct timespec i_atime;    /* inode最近一次的存取时间 */
        struct timespec i_mtime;    /* inode最近一次的修改时间 */
        struct timespec i_ctime;    /* inode的产生时间 */

        unsigned int    i_blkbits;
        blkcnt_t        i_blocks;   /* inode所使用的block数，一个block为512字节 */
        union {
            struct pipe_inode_info *i_pipe;
            struct block_device    *i_bdev; /* 若是块设备，为其对应的block_device结构体指针 */
            struct cdev            *i_cdev; /* 若是字符设备，为其对应的cdev结构体指针 */
        }
        ...
    };

    对于表示设备文件的inode结构，i_rdev字段包含设备编号；
    Linux内核设备编号分为主设备编号和次设备编号，前者为dev_t的高12位，后者为dev_t的低20位；
    从一个inode中获取主设备号和次设备号：
    unsigned int imajor(struct inode *inode);
    unsigned int iminor(struct inode *inode);

    查看/proc/devices文件可以获知系统中注册的设备，第1列为主设备号，第2列为设备名；
    查看/dev目录可以获知系统中包含的设备文件，日期的前两列给出了对应设备的主设备号和次设备号；

    主设备号是与驱动对应的概念，同一类设备一般使用相同的主设备号，不同类的设备一般使用不同的主设备号；
    但也不排除在同一主设备号下包含一定差异的设备；
    因为同一驱动可支持多个同类设备，因此用次设备号来描述使用该驱动的设备的序号，序号一般从0开始；
    内核Documents目录下的devices.txt文件描述了Linux设备号的分配情况它由LANANA组织维护；


### devfs
    devfs（设备文件系统）是由Linux 2.4内核引入的，它的出现使得设备驱动程序能自主地管理自己的设备文件；
    1. 可以通过程序在设备初始化时在/dev目录下创建设备文件，卸载设备时将它删除；
    2. 设备驱动程序可以指定设备名、所有者和权限位，用户空间程序仍可以修改所有者和权限位；
    3. 不再需要为设备驱动程序分配主设备号以及处理次设备号，在程序中可以直接给 register_chrdev()传递0主设备号
       以获得可用的主设备号，并在 devfs_register()中指定次设备号；

    /* 创建设备目录 */
    devfs_handle_t devfs_mk_dir(devfs_handle_t dir, const char *name, void *info);
    /* 创建设备文件 */
    devfs_handle_t devfs_register(devfs_handle_t dir, const char *name, unsigned int flags,
                                  unsigned int major, unsigned int minor, umode_t mode,
                                  void *ops, void *info);
    /* 撤销设备文件 */
    void devfs_unregister(devfs_handle_t de);

    eg.
    static devfs_handle_t devfs_handle;
    static int __init xxx_init(void)
    {
        int ret;
        int i;
        /* 在内核中这册设备 */
        ret = register_chrdev(XXX_MAJOR, DEVICE_NAME, &xxx_fops);
        if (ret < 0) {
            printk(DEVICE_NAME " can't register major number\n");
            return ret;
        }
        /* 创建设备文件 */
        devfs_handle = devfs_register(NULL, DEVICE_NAME, DEVFS_FL_DEFAULT
                                      , XXX_MAJOR, 0, S_IFCHR | S_IRUSR | S_IW_USR
                                      , &xxx_fops, NULL);
        ...
        printk(DEVICE_NAME " initialized\n");
        return 0;
    }
    static void __exit xxx_exit(void)
    {
        devfs_unregister(devfs_handle);             /* 撤销设备文件 */
        unregister_chrdev(XXX_MAJOR, DEVICE_NAME);  /* 注销设备 */
    }
    module_init(xxx_init);
    module_exit(xxx_exit);

    使用register_chrdev()和unregister_chrdev()在Linux 2.6以后的内核中仍被采用；
    用于创建和删除devfs文件节点，这些API已经被删除了；


### udev用户空间设备管理

#### udev与devfs的区别
    在Linux 2.6内核中，devfs被认为是过时的方法，并最后被抛弃了，udev取代了它；
    内核空间的devfs所做的工作，可以被用户空间的udev来完成；

    Linux设计中强调的一个基本观点时机制和策略的分离；
    机制时做某样事情的固定步骤、方法，而策略就是每一个步骤采取的不同方式；
    机制是相对固定的，而每个步骤采用的策略是不固定的；
    机制是稳定的，而策略是灵活的，因此在Linux内核中，不应该实现策略；

    udev完全在用户态工作，利用设备加入或移除时内核所发送的热插拔事件（Hotplug Event）来工作；
    1. 在热插拔时，设备的详细信息会由内核通过netlink套接字发送出来，发出的事情叫uevent；
       udev的设备命令策略、权限控制和事件处理都是在用户态下完成的，
       它利用从内核收到的信息来进行创建设备文件节点等工作；
    2. 对于冷插拔的设备，Linux内核提供了sysfs下面一个uevent节点，可以往该节点写一个“add”，
       导致内核重新发送netlink，之后udev就可以收到冷插拔的netlink消息；
       eg.
       /sys/module/psmouse/uevent

    采用devfs，当一个并不存在的/dev节点被打开的时候，devfs能自动加载对应的驱动，而udev则不这么做；
    udev的设计者认为Linux应该在设备被发现的时候加载驱动模块，而不是当它被访问的时候；
    认为devfs所提供的打开/dev节点时自动加载驱动的功能对一个配置正确的计算机阿里说是多余的；
    系统中所有的设备都应该产生热插拔事件并加载恰当的驱动，而udev能注意到这点并且为它创建对应的设备节点；

#### sysfs文件系统与Linux设备模型
    Linux 2.6以后的内核引入了sysfs文件系统，sysfs被看成是与proc、devfs、和devpty同类别的文件系统，
    该文件系统是一个虚拟的文件系统，它可以产生一个包括所有系统硬件的层级视图，
    与提供进程和状态信息的proc文件系统十分类似；

    sysfs把连接在系统上的设备和总线组织成为一个分级的文件，
    它们可以由用户空间存取，向用户空间导出内核数据结构以及它们的属性；
    sysfs的一个目的就是展示设备驱动模型中各组件的层次关系，
    其顶级目录包括block、bus、dev、devices、class、fs、kernel、power和firmware等；

    block目录包含所有的块设备；
    devices目录包含系统所有的设备，并根据设备挂接的总线类型组织成层次结构；
    bus目录包含系统中所有的总线类型；
    class目录包含系统中的设备类型（如网卡设备、声卡设备、输入设备等）；

    在/sys/bus的pci等子目录下，又会再分出drivers和devices目录，
    而devices目录中的文件是对/sys/devices目录中文件的符号链接；
    同样地，在/sys/class目录下也包含许多对/sys/devices下文件的链接；

    Linux设备模型与设备、驱动、总线和类的现实状况是直接对应的，也正符合Linux 2.6以后内核的设备模型；
    大多数情况下，Linux 2.6以后的内核中的设备驱动核心层代码作为“幕后大佬”可处理好Linux驱动模型中设备、总线
    和类之间的关系；
    内核中的总线和其它内核子系统会完成与设备模型的交互，使得编写底层驱动时可以不关心设备模型，
    只需要按照每个框架的要求，填充 xxx_driver里的各种回调函数，xxx是总线的名字；

    在Linux内核中，分别使用 bus_type、device_driver 和 device 来描述总线、驱动和设备；

    /* 总线 */
    struct bus_type {
        const char                   *name;
        const char                   *dev_name;
        struct device                *dev_root;
        struct device_attribute      *dev_attrs;    /* use dev_groups instead */
        const struct attribute_group **bus_groups;
        const struct attribute_group **dev_groups;
        const struct attribute_group **drv_groups;

        int (*match)(struct device *dev, struct device_driver *drv);
        int (*uevent)(struct device *dev, struct kobj_uevent_env *env);
        int (*probe)(struct device *dev);
        int (*remove)(struct device *dev);
        void (*shutdown)(struct device *dev);

        int (*online)(struct device *dev);
        int (*offline)(struct device *dev);

        int (*suspend)(struct device *dev, pm_message_t state);
        int (*resume)(struct devvice *dev);

        const struct dev_pm_ops *pm;

        struct iommu_ops        *iommu_ops;

        struct subsys_private   *p;
        struct lock_class_key   lock_key;
    };

    /* 驱动 */
    struct device_driver {
        const char      *name;
        struct bus_type *bus;

        struct module   *owner;
        const char      *mod_name;              /* used for built-in modules */

        bool            suppress_bind_attrs;    /* disables bind/unbind via sysfs */

        const struct of_device_id   *of_match_table;
        const struct acpi_device_id *acpi_match_table;

        int (*probe)(struct device *dev);
        int (*remove)(struct device *dev);
        void (*shutdown)(struct device *dev);
        int (*suspend)(struct device *dev, pm_message_t state);
        int (*resume)(struct devvice *dev);
        const struct attribute_groups **groups;

        const struct dev_pm_ops *pm;

        struct driver_private   *p;
    };

    /* 设备 */
    struct device {
        struct device            *parent;

        struct device_private    *p;

        struct                   kobject kobj;
        const char               *init_name;    /* initial name of the device */
        const struct device_type *type;

        struct mutex             mutex;         /* mutex to synchronize calls to its driver */

        struct bus_type          *bus;          /* type of bus device is on */
        struct device_driver     *driver;       /* which driver has allocated this device */

        void                     *platform_data;    /* Platform specific data,
                                                       device core doesn't touch it */

        struct dev_pm_info       power;
        struct dev_pm_domain     *pm_domain;

    #ifdef CONFiG_PINCTRL
        struct dev_pin_info      *pins;
    #endif

    #ifdef CONFiG_Numa
        int numa_node;          /* NUMA node this device is close to */
    #endif
        u64 *dma_mask;          /* dma mask (if dma'able device) */
        u64 coherent_dma_mask;  /* Like dma_mask, but for alloc_coherent mappings as
                                   not all hardware supports 64 bit addresses for consistent
                                   allocations such descriptors */

        struct device_dma_parameters *dma_parms;

        struct list_head             dma_pools; /* dma pools (if dma'ble) */
    #ifdef CONFiG_DMA_CMA
        struct cma                   *cma_area; /* contiguous memory area for
                                                   dma allocations */
    #endif
        struct dev_archdata          archdata;  /* arch specific additions */

        struct device_node           *of_node;  /* associated device tree node */
        struct acpi_dev_node         acpi_node; /* associated ACPI device node */

        dev_t                        devt;      /* dev_t, creates the sysfs "dev" */
        u32                          id;        /* device instance */

        spinlock_t                   devres_lock;
        struct list_head             devres_head;

        struct klist_node            knode_class;
        struct class                 *class;
        const struct attribute_group **groups;  /* optional groups */

        void (*release)(struct device *dev);
        struct iommu_group *iommu_group;

        boo offline_disabled:1;
        boo offline:1;
    };

    1. device_driver 和 device 分别表示驱动和设备，
       而这两者都必须依附于一种总线，因此都包含 struct bus_type 指针；
    2. 在Linux内核中，设备和驱动是分开注册的，注册1个设备的时候，并不需要驱动已经存在，
       而1个驱动被注册的时候，也不需要对应的设备已经被注册；
    3. 设备和驱动各自涌向内核，而每个设备和驱动涌入内核的时候，都会取寻找自己的另一半，
       而正是 bus_type的 match()成员函数将两者捆绑在一起；
    4. 一旦配对成功，xxx_driver的 probe()就被执行（xxx是总线名，如 platform、pci、i2c、spi、usb等）；

    总线、驱动和设备最终都会落实为sysfs中的1个目录，因为它们实际上都可以认为是kobject的派生类，
    kobject可看作是所有总线、设备和驱动的抽象基类，1个kobject对应sysfs中的一个目录；

    总线、设备和驱动中的各个 attribute 则直接落实为sysfs中的一个文件，
    attribute会伴随着 show()和 store()这两个函数，分别用于读写该attribute对应的sysfs文件；

    struct attribute {
        const char            *name;
        umode_t               mode;
    #ifdef CONFiG_DEBUG_LOCK_ALLOC
        bool                  ignore_lockdep:1;
        struct lock_class_key *key;
        struct lock_class_key skey;
    #endif
    };

    struct bus_attribute {
        struct attribute attr;
        ssize_t (*show)(struct bus_type *bus, char *buf);
        ssize_t (*store)(struct bus_type *bus, const char *buf, size_t count);
    };

    struct driver_attribute {
        struct attribute attr;
        ssize_t (*show)(struct device_driver *driver, char *buf);
        ssize_t (*store)(struct device_driver *driver, const char *buf, size_t count);
    };

    struct device_attribute {
        struct attribute attr;
        ssize_t (*show)(struct device *dev, struct device_attribute *attr, char *buf);
        ssize_t (*store)(struct device *dev, struct device_attribute *attr,
                         const char *buf, size_t count);
    };

    事实上，sysfs中的目录来源于 bus_type、device_driver、device，而目录中的文件则来源于 attribute；

    Linux内核中定义了一些快捷方式以方便attribute的创建工作：
    #define BUS_ATTR(_name, _mode, _show, _store)   \
            struct bus_attribute bus_attr_##_name = __ATTR(_name, _mode, _show, _store)
    #define BUS_ATTR_RW(_name) \
            struct bus_attribute bus_attr_##_name = __ATTR_RW(_name)
    #define BUS_ATTR_RO(_name) \
            struct bus_attribute bus_attr_##_name = __ATTR_RO(_name)

    #define DRIVER_ATTR_RW(_name) \
            struct driver_attribute driver_attr_##_name = __ATTR_RW(_name)
    #define DRIVER_ATTR_RO(_name) \
            struct driver_attribute driver_attr_##_name = __ATTR_RO(_name)
    #define DRIVER_ATTR_WO(_name) \
            struct driver_attribute driver_attr_##_name = __ATTR_WO(_name)

    #define DEVICE_ATTR(_name, _mode, _show, _store) \
            struct device_attribute dev_attr_##_name = __ATTR(_name, _mode, _show, _store)
    #define DEVICE_ATTR_RW(_name) \
            struct device_attribute dev_attr_##_name = __ATTR_RW(_name)
    #define DEVICE_ATTR_RO(_name) \
            struct device_attribute dev_attr_##_name = __ATTR_RO(_name)
    #define DEVICE_ATTR_WO(_name) \
            struct device_attribute dev_attr_##_name = __ATTR_WO(_name)

    其中，
    #define __ATTR(_name, _mode, _show, _store) { \
        .attr  = {.name = __stringify(_name), \
                  .mode = VERIFY_OCTAL_PERMISSIONS(_mode) }, \
        .show  = _show, \
        .store = _store, \
    }
    #define __ATTR_RO(_name) { \
        .attr = { .name = __stringify(_name), .mode = S_IRUGO }, \
        .show = _name##_show, \
    }
    #define __ATTR_WO(_name) { \
        .attr  = { .name = __stringify(_name), .mode = S_IWUSR }, \
        .store = _name##_store, \
    }
    #define __ATTR_RW(_name) \
            __ATTR(_name, (S_IWUSR | S_IRUGO), _name##_show, _name##_store)

    eg. drivers/base/bus.c:
    static BUS_ATTR(drivers_probe, S_IWUSR, NULL, store_drivers_probe);
    staitc BUS_ATTR(drivers_autoprobe, S_IWUSR | S_IRUGO \
                    , show_drivers_autoprobe, store_drivers_autoprobe);
    static BUS_ATTR(uevent, S_IWUSR, NULL, bus_uevent_store);
    |
    -> /sys/bus/platform:
    devices drivers drivers_autoprobe drivers_probe uevent

#### udev的组成
    udev在用户空间中执行，动态建立/删除设备文件，允许每个人都不用关心主/次设备号而提供LSB
    （Linux标准规范，Linux Standard Base）名称，并且可以根据需要固定名称；
    udev的工作过程如下：
    1. 当内核检测到系统中出现了新设备后，内核会通过netlink套接字发送uevent；
    2. udev获取内核发送的信息，进行规则的匹配；
       匹配的事物包括SUBSYSTEM、ACTION、attribute、内核提供的名称（通过KERNEL=）以及其它的环境变量；

#### udev规则文件
    udev的规则文件以行为单位，以“#”开头的行代表注释行；
    其余的每一行代表一个规则；每个规则分成一个或多个匹配部分和赋值部分；
    匹配部分用匹配专用的关键字来表示，相应的赋值部分用赋值专用的关键字来表示；
    匹配部分，可以通过“*”、“?“、[a ~ c]、[1 ~ 9]等shell通配符来灵活匹配多个项目；

    匹配关键字包括：
    ACTION      行为
    KERNEL      匹配内核设备名
    BUS         匹配总线类型
    SUBSYSTEM   匹配子系统名
    ATTR        属性
    ...

    赋值关键字包括：
    NAME        创建的设备文件名
    SYMLINK     符号创建链接名
    OWNER       设置设备的所有者
    GROUP       设置设备的组
    IMPORT      调用外部程序
    MODE        节点访问权限
    ...


    匹配键和赋值键操作符解释见下表：
    操作符         匹配或赋值           解释
    －－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
    ==            匹配              相等比较
    !=            匹配              不等比较
    =             赋值              分配一个特定的值给该键,他可以覆盖之前的赋值
    +=            赋值              追加特定的值给已经存在的键
    :=            赋值              分配一个特定的值给该键,后面的规则不可能覆盖它

    udev规则匹配表：
    键                   含义
    －－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
    ACTION               事件 (uevent) 的行为,例如：add( 添加设备 )、remove( 删除设备 ).
    KERNEL               在内核里看到的设备名字,比如sd*表示任意SCSI磁盘设备
    DEVPATH              内核设备录进,比如/devices/*
    SUBSYSTEM            子系统名字,例如：sda 的子系统为 block.
    BUS                  总线的名字,比如IDE,USB
    DRIVER               设备驱动的名字,比如ide-cdrom
    ID                   独立于内核名字的设备名字
    SYSFS{ value}        sysfs属性值,他可以表示任意
    ENV{ key}            环境变量,可以表示任意
    PROGRAM              可执行的外部程序,如果程序返回0值,该键则认为为真(true)
    RESULT               上一个PROGRAM调用返回的标准输出.
    NAME                 根据这个规则创建的设备文件的文件名.
                        （注意：仅仅第一行的NAME描述是有效的,后面的均忽略.
                               如果想使用使用两个以上的名字来访问一个设备的话,可以考虑SYMLINK键.)
    SYMLINK              为 /dev/下的设备文件产生符号链接.由于 udev 只能为某个设备产生一个设备文件,
                        （所以为了不覆盖系统默认的 udev 规则所产生的文件,推荐使用符号链接.）
    OWNER                设备文件的属组
    GROUP                设备文件所在的组.
    MODE                 设备文件的权限,采用8进制
    RUN                  为设备而执行的程序列表
    LABEL                在配置文件里为内部控制而采用的名字标签(下下面的GOTO服务)
    GOTO                 跳到匹配的规则（通过LABEL来标识）,有点类似程序语言中的GOTO
    IMPORT{ type}        导入一个文件或者一个程序执行后而生成的规则集到当前文件
    WAIT_FOR_SYSFS       等待一个特定的设备文件的创建.主要是用作时序和依赖问题.
    PTIONS               特定的选项：
    last_rule            对这类设备终端规则执行；
    ignore_device        忽略当前规则；
    ignore_remove        忽略接下来的并移走请求.
    all_partitions       为所有的磁盘分区创建设备文件.

    udev一些特殊的值和替换值：
    －－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
    $kernel,       %k：          设备的内核设备名称,例如：sda、cdrom.
    $number,       %n：          设备的内核号码,例如：sda3 的内核号码是 3.
    $devpath,      %p：          设备的 devpath路径.
    $id,           %b：          设备在 devpath里的 ID 号.
    $sysfs{file},  %s{file}：    设备的 sysfs里 file 的内容.其实就是设备的属性值.
    $env{key},     %E{key}：     一个环境变量的值.
    $major,        %M：          设备的 major 号.
    $minor，       %m：           设备的 minor 号.
    $result,       %c：          PROGRAM 返回的结果
    $parent,       %P：          父设备的设备文件名.
    $root,         %r：          udev_root的值,默认是 /dev/.
    $tempnode,     %N：          临时设备名.
    %%：                         符号 % 本身.
    $$：                         符号 $ 本身.


    eg.（写在一行上）
    SUBSYSTEM=="net", ACTION=="add", DRIVERS=="?*", ATTR{address}=="08:00:27:35:be:ff",
      ATTR{dev_id}=="0x0", ATTR{type}=="1", KERNEL=="eth*", NAME="eth1"
    其中的”匹配“部分包括SUBSYSTEM、ACTION、ATTR、KERNEL等，而”赋值“部分有一项，是NAME；
    这个规则的意思是：
    当系统中出现的新硬件属于net子系统范畴，系统对该硬件采取的动作是”add“这个硬件，且这个硬件的”address“
    属性信息等于”08:00:27:35:be:ff“，”dev_id“属性等于”0x0“、”type“属性为1等，
    此时，对这个硬件在udev层次施行的动作是创建 /dev/eth1；

    udev和devfs在命令方面的差异：
    如果系统中有两个USB打印机，一个可能被称为/dev/usb/lp0，另一个便是/dev/usb/lp1；
    但是到底哪个文件对应哪个打印机是无法确定的，lp0、lp1和实际的设备没有一一对应的关系，
    映射关系会因设备发现的顺序、打印机本身关闭等而不确定；
    因此，理想的方式是两个打印机应该采用基于它们的序列号或者其它标识信息的办法来进行确定的映射，
    devfs无法做到这一点，udev却可以做到；
    |
    使用如下规则：（写在一行上）
    SUBSYSTEM=="usb", ATTR{serial}=="HXOLL0012202323480",
      NAME="lp_epson", SYMLINK+="printers/epson_stylus"
    序列号为“HXOLL0012202323480”的USB打印机不管何时被插入，对应的设备名都是/dev/lp_epson；


    查找规则文件能利用的内核信息和sysfs属性信息；
    eg. udevadm info -a -p /sys/devices/platform/serial8250/tty/ttyS0

    如果/dev/下面的节点已经被创建，但是不知道它对应的/sys具体节点路径，可以反向分析；
    eg. udevadm info -a -p $(udevadm info -q path -n /dev/<节点名>)

    查看ENV的一些参数；
    eg. udevadm info -q env -p /sys/block/sdb

    在嵌入式系统在，也可以用udev的轻量级版本mdev，mdev集成在busybox中；
    在编译busybox的时候，选中mdev相关项目即可；
    Android也没有采用udev，它采用的是vold；vold的机制和udev是一样的；
