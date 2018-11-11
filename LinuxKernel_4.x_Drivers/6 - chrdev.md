# 字符设备驱动
    character device
    <<Linux设备驱动开发详解：基于最新的Linux 4.0内核>>


### Linux字符设备驱动结构

#### cdev 结构体
    在Linux内核中，使用cdev结构体描述一个字符设备；

    struct cdev {
        struct kobject         kobj;        /* 内嵌的kobject对象 */
        struct module          *owner;      /* 所属模块 */
        struct file_operations *ops;        /* 文件操作结构体 */
        struct list_head       list;
        dev_t                  dev;         /* 设备号 */
        unsigned int           count;
    };

    cdev结构体中的 dev_t成员定义了设备号，为32位，其中12位主设备号，20位位次设备号；
    从dev_t获取主设备号和次设备号：
    MAJOR(dev_t dev)
    MINOR(dev_t dev)
    通过主设备号和次设备号生成dev_t：
    MKDEV(int major, int minor)

    cdev结构体的另一个重要成员 file_operations定义了字符设备驱动提供给虚拟文件系统的接口函数；
    Linux内核提供了一组函数以用于操作cdev结构体：
    void cdev_init(struct cdev *cdev, const struct file_operations *fops);
    struct cdev *cdev_alloc(void);
    void cdev_put(struct cdev *p);
    int cdev_add(struct cdev *p, dev_t dev, unsigned count);
    void cdev_del(struct cdev *p);

    cdev_init()函数用于初始化cdev的成员，并建立cdev和file_operations之间的联系；
    void cdev_init(struct cdev *cdev, const struct file_operations *fops)
    {
        memset(cdev, 0, sizeof *cdev);
        INIT_LIST_HEAD(&cdev->list);
        kobject_init(&cdev->kobj, &ktype_cdev_default);
        cdev->ops = fops;   /* 将传入的文件操作结构体指针赋值给cdev的ops */
    }

    cdev_alloc()函数用于动态申请一个cdev内存；
    struct cdev *cdev_alloc(void)
    {
        struct cdev *p = kzalloc(sizeof(struct cdev), GFP_KERNEL);
        if (p) {
            INIT_LIST_HEAD(&p->list);
            kobject_init(&p->kobj, &ktype_cdev_dynamic);
        }
        return p;
    }

    cdev_add()函数和cdev_del()函数分别向系统添加和删除一个cdev，完成字符设备的注册和注销；
    对cdev_add()的调用通常发生在字符设备驱动模块加载函数中；
    对cdev_del()的调用通常发生在字符设备驱动模块卸载函数中；

#### 分配和释放设备号
    在调用cdev_add()函数向系统注册字符设备之前，
    应首先调用 register_chrdev_region() 或 alloc_chrdev_region()函数向系统申请设备号；
    int register_chrdev_region(dev_t from, unsigned count, const char *name);
    int alloc_chrdev_region(dev_t *dev, unsigned baseminor, unsigned count, const char *name);

    register_chrdev_region()函数用于已知起始设备的设备号的情况；
    而alloc_chrdev_region()用于设备号未知，向系统申请未被占用的设备号的情况，
    调用成功后，会把得到的设备号放入第一个参数dev中；
    alloc_chrdev_region()相比于register_chrdev_region()的优点在于它会自动避开设备号重复的冲突；

    在调用cdev_del()函数从体统注销字符设备后，unregister_chrdev_region()应该被调用以释放原先申请的设备号；
    void unregister_chrdev_region(dev_t from, unsigned count);

#### file_operations 结构体
    file_operations结构体中的成员函数是字符设备驱动程序设计的主体内容，
    这些函数实际会在应用程序进行Linux的open()、write()、read()、close()等系统调用时最终被内核调用；

    struct file_operations {
        struct module *owner;
        loff_t (*llseek)(struct file *, loff_t, int);
        ssize_t (*read)(struct file *, char __user *, size_t, loff_t *);
        ssize_t (*write)(struct file *, const __user *, size_t, loff_t *);
        ssize_t (*aio_read)(struct kiocb *, const struct iovec *, unsigned long, loff_t);
        ssize_t (*aio_write)(struct kiocb *, const struct iovec *, unsigned long, loff_t);
        int (*iterate)(struct file *, struct dir_context *);
        unsigned int (*poll)(struct file *, struct poll_table_struct *);
        long (*unlocked_ioctl)(struct file *, unsigned int, unsigned long);
        long (*compat_ioctl)(struct file *, unsigned int, unsigned long);
        int (*mmap)(struct file *, struct vm_area_struct *);
        int (*open)(struct inode *, struct file *);
        int (*flush)(struct file *, fl_owner_t id);
        int (*release)(struct inode *, struct file *);
        int (*fsync)(struct file *, loff_t, loff_t, int datasync);
        int (*aio_fsync)(struct kiocb *, int datasync);
        int (*fasync)(int, struct file *, int);
        int (*lock)(struct file *, int, struct file_lock *);
        ssize_t (*sendpage)(struct file *, struct page *, int, size_t, loff_t *, int);
        unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long,
                                           unsigned long, unsigned long);
        int (*check_flags)(int);
        int (*flock)(struct file *, int, struct file_lock *);
        ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *,
                               size_t, unsigned int);
        int (*setlease)(struct file *, long, struct file_lock **);
        long (*fallocte)(struct file *, int mode, loff_t offset, loff_t len);
        int (*show_fdinfo)(struct seq_file *m, struct file *f);
    };

    llseek()函数用来修改一个文件的当前读写位置，并将新位置返回，在出错时，返回一个负值；

    read()函数用来从设备中读取数据，成功时该函数返回读取的字节数，出错时返回一个负值；
    它与用户空间中的 ssize_t read(int fd, void *buf, size_t count)
    和 size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)对应；

    write()函数向设备发送数据，成功时返回写入的字节数；
    如果此函数未被实现，当用户进行write()系统调用时，将得到-EINVAL返回值；
    它与用户空间中的 ssize_t write(int fd, const void *buf, size_t count)
    和 size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)对应；

    read()和write()如果返回0，则暗示 end-of-file（EOF）；

    unlocked_ioctl()提供设备相关控制命令的实现，当调用成功时，返回一个非负值；
    它与用户空间的 int fcntl(int fd, int cmd, ... /* arg */)
    和 int ioctl(int fd, int request, ...)对应；

    mmap()函数将设备内存映射到进程的虚拟地址空间，
    如果此函数未被实现，用户空间进行mmap()系统调用时将获得-ENODEV返回值；
    这个函数对于帧缓冲等设备特别有意义，帧缓冲被映射到用户空间后，应用程序可以直接访问它
    而无须在内核和应用间进行内存赋值；它与用户空间的
    void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)对应；

    当用户空间调用Linux API函数 open()打开设备文件时，设备驱动的 open()函数最终被调用；
    驱动程序可以不实现这个函数，这种情况下，设备的打开操作永远成功；与open()函数对应的是 release()函数；

    poll()函数一般用于询问设备是否可被非阻塞地立即读写；
    当询问的条件未触发时，用户空间进行 select() 和 poll()系统调用将引起进程的阻塞；

    aio_read()和aio_write()函数分别对与文件描述符对应的设备进行异步读、写操作；
    设备实现这两个函数后，用户空间可以对该设备文件描述符执行 SYS_io_setup、 SYS_io_submit、
    SYS_io_getevents、SYS_io_destroy等系统调用进行读写；

#### Linux字符设备驱动的组成
    在Linux中，字符设备驱动由如下几个部分组成：

##### 1. 字符设备驱动模块加载与卸载函数
    在字符设备驱动模块加载函数中应该实现设备号的申请和cdev的注册，而在卸载函数中应实现设备号的释放和cdev的注销；
    Linux内核的编码习惯是为设备定义一个设备相关的结构体，该结构体包含设备所涉及的cdev、私有数据及锁等信息；

    /* 设备结构体 */
    struct xxx_dev_t {
        struct cdev cdev;
        ...
    } xxx_dev;

    /* 设备驱动模块加载函数 */
    static int __init xxx_init(void)
    {
        ...
        cdev_init(&xxx_dev.cdev, &xxx_fops);    /* 初始化cdev */
        xxx_dev.cdev.owner = THIS_MODULE;
        /* 获取字符设备号 */
        if (xxx_major) {
            register_chrdev_region(xxx_dev_no, 1, DEV_NAME);
        } else {
            alloc_chrdev_region(&xxx_dev_no, 0, 1, DEV_NAME);
        }
        ret = cdev_add(&xxx_dev.cdev, xxx_dev_no, 1);   /* 注册设备 */
        ...
    }

    /* 设备驱动模块卸载函数 */
    static void __exit xxx_exit(void)
    {
        cdev_del(&xxx_dev.cdev);                    /* 释放占用的设备号 */
        unregister_chrdev_region(xxx_dev_no, 1);    /* 注销设备 */
        ...
    }

##### 2. 字符设备驱动的 file_operations 结构体中的成员函数
    file_operations结构体中的成员函数是字符设备驱动与内核虚拟文件系统的接口，
    是用户空间对Linux进行系统调用最终的落实者；
    大多数字符设备驱动会实现 read()、write()和 ioctl()函数；

    /* 读设备 */
    ssize_t xxx_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
    {
        ...
        copy_to_user(buf, ..., ...);
        ...
    }

    /* 写设备 */
    ssize_t xxx_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
    {
        ...
        copy_from_user(..., buf, ...);
        ...
    }

    /* ioctl函数 */
    long xxx_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
    {
        ...
        switch (cmd) {
        case XXX_CMD1:
            ...
            break;
        case XXX_CMD2:
            ...
            break;
        default:
            /* 不能支持的命令 */
            return -ENOTTY;
        }
        return 0;
    }

    设备驱动的读/写函数中，filp是文件结构体指针，buf是用户空间内存的地址，该地址在内核空间不宜直接读写，
    count是要读/写的字节数，f_pos是读/写的位置相对于文件开头的偏移；

    内核空间和用户空间内存的复制函数：
    unsigned long copy_from_user(void *to, const void __user *from, unsigned long count);
    unsigned long copy_to_user(void __user *to, const void *from, unsigned long count);
    上述函数均返回不能被复制的字节数，因此，如果完全复制成功，返回值为0；

    如果要复制的内存是简单类型，如char、int、long等，则可以使用简单的 put_user() 和 get_user()；
    int val;                        /* 内核空间整型变量 */
    get_user(val, (int *)arg);      /* 用户->内核，arg是用户空间的地址 */
    put_user(val, (int *)arg);      /* 内核->用户，arg是用户空间的地址 */

    读和写函数中的 __user是一个宏，表明其后的指针指向用户空间，实际上更多地充当了代码自注释的功能；
    #ifdef __CHECKER__
    #define __user  __attribute__((noderef, address_space(1)))
    #else
    #define __user
    #endif

    用户空间不能直接访问内核空间的内存；
    内核空间虽然可以访问用户空间的缓冲区，但是在访问之前，一般需要先检查其合法性，
    通过 access_ok(type, addr, size) 进行判断，以确定传入的缓冲区的确属于用户空间；

    static inline unsigned long __must_check
    copy_from_user(void *to, const void __user *from, unsigned long n)
    {
        if (access_ok(VERIFY_READ, from, n))
            n = __copy_from_user(to, from, n);
        else /* security hole - plug it */
            memset(to, 0, n);
        return n;
    }

    static inline unsigned long __must_check
    copy_to_user(void __user *to, const void *from, unsigned long n)
    {
        if (access_ok(VERIFY_WRITE, to, n))
            n = __copy_to_user(to, from, n);
        return n;
    }

    I/O控制函数的cmd参数为事先定义的I/O控制命令，而arg为对应于该命令的参数；

    在字符设备驱动中，需要定义一个file_operations的实例，并将具体设备驱动的函数赋值给file_operations的成员；
    struct file_operations xxx_fops = {
        .owner          = THIS_MODULE,
        .read           = xxx_read,
        .write          = xxx_write,
        .unlocked_ioctl = xxx_ioctl,
        ...
    };


### ioctl 函数

#### ioctl 命令
    I/O控制命令的组成：
        设备类型    序列号     方向      数据尺寸
          8位       8位       2位      13/14位

    命令码的设备类型字段为一个“幻数”，可以是 0 ~ 0xff 的值，
    内核中的 ioctl-number.txt 给出了一些推荐的和已经被使用的“幻数”，新设备驱动定义“幻数”要避免与其冲突；

    命令码的方向字段为2位，该字段表示数据传送的方向，可能的值是
    _IOC_NONE（无数据传输）、_IOC_READ（读）、_IOC_WRITE（写）和 _IOC_READ|_IOC_WRITE（双向）；
    数据传送的方向是从应用程序的角度来看的；

    命令码的数据长度字段表示涉及的用户数据的大小，这个成员的宽度依赖于体系结构；

    内核还定义了 _IO()、_IOR()、_IOW()、_IOWR() 这4个宏来辅助生成命令；
    #define _IO(type, nr)           _IOC(_IOC_NONE, (type), (nr), 0)
    #define _IOR(type, nr, size)    _IOC(_IOC_READ, (type), (nr), (_IOC_TYPECHECK(size)))
    #define _IOW(type, nr, size)    _IOC(_IOC_WRITE, (type), (nr), (IOC_TYPECHECK(size)))
    #define _IOWR(type, nr, size)   _IOC(_IOC_READ|_IOC_WRITE, (type), (nr), \
                                         (IOC_TYPECHECK(size)))
    #define _IOC(dir, type, nr, size) \
            (((dir) << _IOC_DIRSHIFT) | \
            ((type) << _IOC_TYPESHIFT) | \
            ((nr)   << _IOC_NRSHIFT) | \
            ((size) << _IOC_SIZESHIFT))

#### 预定义命令
    内核中预定义了一些I/O控制命令，
    如果某设备驱动中包含了与预定义命令一样的命令码，这些命令码会作为预定义命令被内核处理，而不是被设备驱动处理；
    这些宏定义在内核的 include/uapi/asm-generic/ioctls.h 文件中；

    FIOCLEX ：即File IOctl Close on Exec，
              对文件设置专用标志，通知内核当 exec()系统调用发生时自动关闭打开的文件；
    FIONCLEX：即File IOctl Not Close on Exec，清除由FIOCLEX命令设置的标志；
    FIOQSIZE：获得一个文件或目录的大小，当用于设备文件时，返回一个ENOTTY错误；
    FIONBIO ：即File IOctl Non-Blocking I/O，这个调用修改在 filp->f_flags中的 O_NONBLOCK 标志；
