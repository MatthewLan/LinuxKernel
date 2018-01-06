
# cdev
    linux-4.14
    - include/linux/cdev.h
    - include/linux/kdev_t.h
    - include/linux/fs.h
    - include/linux/uaccess.h
    - include/asm/uaccess.h
    - fs/char_dev.c


### struct cdev
    struct cdev {
        struct kobject kobj;                /* 内嵌的kobject对象 */
        struct module *owner;               /* 所属模块 */
        const struct file_operations *ops;  /* 文件操作结构体 */
        struct list_head list;
        dev_t dev;                          /* 设备号 */
        unsigned int count;
    } __randomize_layout;

### 设备号
    32位，高12位为主设备号，低20位为次设备号;

    #define MINORBITS       20
    #define MINORMASK       ((1U << MINORBITS) - 1)

    #define MAJOR(dev)      ((unsigned int) ((dev) >> MINORBITS))
    #define MINOR(dev)      ((unsigned int) ((dev) & MINORMASK))
    #define MKDEV(ma,mi)    (((ma) << MINORBITS) | (mi))

### cdev_init()
    初始化cdev的成员，并建立cdev和file_operations之间的联系；

    void cdev_init(struct cdev *cdev, const struct file_operations *fops)
    {
        memset(cdev, 0, sizeof *cdev);
        INIT_LIST_HEAD(&cdev->list);
        kobject_init(&cdev->kobj, &ktype_cdev_default);
        cdev->ops = fops;
    }

### cdev_alloc()
    动态申请一个cdev内存；
    cdev_alloc()与cdev_init()所作的工作差不多，
    仅仅相差建立cdev和file_operations之间的联系，所以不要同时使用；

    struct cdev *cdev_alloc(void)
    {
        struct cdev *p = kzalloc(sizeof(struct cdev), GFP_KERNEL);
        if (p) {
            INIT_LIST_HEAD(&p->list);
            kobject_init(&p->kobj, &ktype_cdev_dynamic);
        }
        return p;
    }

### cdev_add()
    向系统添加一个cdev，完成字符设备的注册；通常在字符设备驱动模块加载函数中被调用；
    在该函数调用之前，应首先调用register_chrdev_region()或alloc_chrdev_region()向系统申请设备号；

    int cdev_add(struct cdev *p, dev_t dev, unsigned count)
    {
        int error;
        p->dev = dev;
        p->count = count;
        error = kobj_map(cdev_map, dev, count, NULL, exact_match, exact_lock, p);
        if (error)
            return error;
        kobject_get(p->kobj.parent);
        return 0;
    }

### cdev_del()
    向系统删除一个cdev，完成字符设备的注销；通常在字符设备驱动模块卸载函数中被调用；
    该函数调用之后，应调用unregister_chrdev_region()，用以释放原先申请的设备号；

    void cdev_del(struct cdev *p)
    {
        cdev_unmap(p->dev, p->count);
        kobject_put(&p->kobj);
    }


### register_chrdev_region()
    分配设备号，用于已知起始设备的设备号的情况；

    int register_chrdev_region(dev_t from, unsigned count, const char *name)
    {
        struct char_device_struct *cd;
        dev_t to = from + count;
        dev_t n, next;

        for (n = from; n < to; n = next) {
            next = MKDEV(MAJOR(n)+1, 0);
            if (next > to)
                next = to;
            cd = __register_chrdev_region(MAJOR(n), MINOR(n), next - n, name);
            if (IS_ERR(cd))
                goto fail;
        }
        return 0;
    fail:
        to = n;
        for (n = from; n < to; n = next) {
            next = MKDEV(MAJOR(n)+1, 0);
            kfree(__unregister_chrdev_region(MAJOR(n), MINOR(n), next - n));
        }
        return PTR_ERR(cd);
    }

### alloc_chrdev_region()
    分配设备号，用于设备号未知，向系统动态申请未被占用的设备号情况；
    函数调用成功后，会把得到的设备号放入第一参数dev中；

    int alloc_chrdev_region(dev_t *dev, unsigned baseminor, unsigned count, const char *name)
    {
        struct char_device_struct *cd;
        cd = __register_chrdev_region(0, baseminor, count, name);
        if (IS_ERR(cd))
            return PTR_ERR(cd);
        *dev = MKDEV(cd->major, cd->baseminor);
        return 0;
    }

### unregister_chrdev_region()
    释放原先申请的设备号；

    void unregister_chrdev_region(dev_t from, unsigned count)
    {
        dev_t to = from + count;
        dev_t n, next;
        for (n = from; n < to; n = next) {
            next = MKDEV(MAJOR(n)+1, 0);
            if (next > to)
                next = to;
            kfree(__unregister_chrdev_region(MAJOR(n), MINOR(n), next - n));
        }
    }


### struct file_operations
    struct file_operations {
        struct module *owner;
        /* 修改一个文件的当前读写位置，并将新位置返回；出错时，返回一个负值 */
        loff_t (*llseek) (struct file *, loff_t, int);
        /* 从设备中读取数据，成功时返回读取的字节数；出错时返回一个负值 */
        ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
        /* 向设备发送数据，成功时返回 写入的字节数；若该函数未被实现，将返回-EINVAL */
        ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
        ssize_t (*read_iter) (struct kiocb *, struct iov_iter *);
        ssize_t (*write_iter) (struct kiocb *, struct iov_iter *);
        int (*iterate) (struct file *, struct dir_context *);
        int (*iterate_shared) (struct file *, struct dir_context *);
        /* 询问设备是否可被非阻塞地立即读写;
           当询问的条件未被触发时，select()和poll()系统调用将引起进程的阻塞 */
        unsigned int (*poll) (struct file *, struct poll_table_struct *);
        /* 提供设备相关控制命令的实现，成功时返回一个非负值 -> fcntl(), ioctl() */
        long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
        long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
        /* 将设备内存映射到进程的虚拟地址空间中；若该函数未被实现，将返回-ENODEV */
        int (*mmap) (struct file *, struct vm_area_struct *);
        int (*open) (struct inode *, struct file *);
        int (*flush) (struct file *, fl_owner_t id);
        int (*release) (struct inode *, struct file *);
        int (*fsync) (struct file *, loff_t, loff_t, int datasync);
        int (*fasync) (int, struct file *, int);
        int (*lock) (struct file *, int, struct file_lock *);
        ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int);
        unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long,
            unsigned long, unsigned long);
        int (*check_flags)(int);
        int (*flock) (struct file *, int, struct file_lock *);
        ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, loff_t *, size_t,
            unsigned int);
        ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *,size_t,
            unsigned int);
        int (*setlease)(struct file *, long, struct file_lock **, void **);
        long (*fallocate)(struct file *file, int mode, loff_t offset, loff_t len);
        void (*show_fdinfo)(struct seq_file *m, struct file *f);
    #ifndef CONFIG_MMU
        unsigned (*mmap_capabilities)(struct file *);
    #endif
        ssize_t (*copy_file_range)(struct file *, loff_t, struct file *, loff_t,size_t,
            unsigned int);
        int (*clone_file_range)(struct file *, loff_t, struct file *, loff_t, u64);
        ssize_t (*dedupe_file_range)(struct file *, u64, u64, struct file *, u64);
    } __randomize_layout;

    1. mmap()函数对于帧缓冲等设备特别有意义,
    帧缓冲被映射到用户空间后，应用程序可以直接访问它而无须在内核和应用间进行内存赋值;

    2. open()函数可以不被实现，在这种情况下，设备的打开操作永远成功；


### register_chrdev()
    1. 申请设备号；
    2. 动态分配cdev；
    3. 建立cdev与的联系；
    4. 向系统添加一个cdev，完成字符设备的注册；

    static inline int register_chrdev(unsigned int major, const char *name,
                                      const struct file_operations *fops)
    {
        return __register_chrdev(major, 0, 256, name, fops);
    }

    int __register_chrdev(unsigned int major, unsigned int baseminor,
              unsigned int count, const char *name, const struct file_operations *fops)
    {
        struct char_device_struct *cd;
        struct cdev *cdev;
        int err = -ENOMEM;

        cd = __register_chrdev_region(major, baseminor, count, name);
        if (IS_ERR(cd))
            return PTR_ERR(cd);

        cdev = cdev_alloc();
        if (!cdev)
            goto out2;

        cdev->owner = fops->owner;
        cdev->ops = fops;
        kobject_set_name(&cdev->kobj, "%s", name);

        err = cdev_add(cdev, MKDEV(cd->major, baseminor), count);
        if (err)
            goto out;

        cd->cdev = cdev;

        return major ? 0 : cd->major;
    out:
        kobject_put(&cdev->kobj);
    out2:
        kfree(__unregister_chrdev_region(cd->major, baseminor, count));
        return err;
    }

### unregister_chrdev()
    1. 释放原先申请的设备号;
    2. 向系统删除一个cdev，完成字符设备的注销;
    3. 销毁原先动态分配cdev;

    static inline void unregister_chrdev(unsigned int major, const char *name)
    {
        __unregister_chrdev(major, 0, 256, name);
    }

    void __unregister_chrdev(unsigned int major, unsigned int baseminor,
             unsigned int count, const char *name)
    {
        struct char_device_struct *cd;
        cd = __unregister_chrdev_region(major, baseminor, count);
        if (cd && cd->cdev)
            cdev_del(cd->cdev);
        kfree(cd);
    }


### copy_from_user()
    static __always_inline unsigned long __must_check
    copy_from_user(void *to, const void __user *from, unsigned long n)
    {
        if (likely(check_copy_size(to, n, false)))
            n = _copy_from_user(to, from, n);
        return n;
    }

    static inline unsigned long
    _copy_from_user(void *to, const void __user *from, unsigned long n)
    {
        unsigned long res = n;
        might_fault();
        if (likely(access_ok(VERIFY_READ, from, n))) {  /* 检查用户空间的缓冲区的合法性 */
            kasan_check_write(to, n);
            res = raw_copy_from_user(to, from, n);
        }
        if (unlikely(res))
            memset(to + (n - res), 0, res);
        return res;
    }

    *** 注意：在内核空间与用户空间的界面处，内核检查用户空间缓冲区的合法性显得尤其必要；

### copy_to_user()
    static __always_inline unsigned long __must_check
    copy_to_user(void __user *to, const void *from, unsigned long n)
    {
        if (likely(check_copy_size(from, n, true)))
            n = _copy_to_user(to, from, n);
        return n;
    }

    static inline unsigned long
    _copy_to_user(void __user *to, const void *from, unsigned long n)
    {
        might_fault();
        if (access_ok(VERIFY_WRITE, to, n)) {
            kasan_check_read(from, n);
            n = raw_copy_to_user(to, from, n);
        }
        return n;
    }

### get_user()
### put_user()
    get_user(x, ptr)
    put_user(x, ptr)
    x  : 内核空间的整型变量
    ptr: 用户空间的地址
