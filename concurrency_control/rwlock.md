# 读写自旋锁
    <<Linux设备驱动开发详解：基于最新的Linux 4.0内核>>


    自旋锁不关心锁定的临界区究竟在进行什么操作，不管是读还是写，它都一视同仁，即便多个执行单元同时读取临界资源
    也会被锁住；实际上，对共享资源并发访问时，多个执行单元同时读取它是不会有问题，自旋锁的衍生锁读写自旋锁
    （rwlock）可允许读的并发；

    读写自旋锁是一种比自旋锁粒度更小的锁机制，它保留了“自旋”的概念，但是在写操作方面，只能最多有1个写进程，
    在读操作方面，同时可以有多个读执行单元；
    当然，读核写也不能同时进行；


##### 读写自旋锁的操作
    1. 定义和初始化读写自旋锁
    rwlock_t my_rwlock;
    rwlock_init(&my_rwlock);

    2. 读锁定
    void read_lock(rwlock_t *lock);
    void read_lock_irqsave(rwlock_t *lock, unsigned long flags);
    void read_lock_irq(rwlock_t *lock);
    void read_lcok_bh(rwlock_t *lock);

    3. 读解锁
    void read_unlock(rwlock_t *lock);
    void read_unlock_irqrestore(rwlock_t *lock, unsigned long flags);
    void read_unlock_irq(rwlock_t *lock);
    void read_unlock_bh(rwlock_t *lock);

    在对共享资源进行读取之前，应该先调用读锁定函数，完成之后应调用读解锁函数；

    read_lock_irqsave() = read_lock() + local_irq_save()
    read_lock_irq()     = read_lock() + local_irq_disable()
    read_lock_bh()      = read_lock() + local_bh_disable()

    read_unlock_irqrestore() = read_unlock() + local_irq_restore()
    read_unlock_irq()        = read_unlock() + local_irq_enable()
    read_unlock_bh()         = read_unlock() + local_bh_enable()


    4. 写锁定
    void write_lock(rwlock_t *lock);
    void write_lock_irqsave(rwlock_t *lock, unsigned long flags);
    void write_lock_irq(rwlock_t *lock);
    void write_lock_bh(rwlock_t *lock);
    int  write_trylock(rwlock_t *lock);

    5. 写解锁
    void write_unlock(rwlock_t *lock);
    void write_unlock_irqrestore(rwlock_t *lock, unsigned long flags);
    void write_unlock_irq(rwlock_t *lock);
    void write_unlock_bh(rwlock_t *lock);

    在对共享资源进行写之前，应该先调用写锁定函数，完成之后应调用写解锁函数；

    write_lock_irqsave() = write_lock() + local_irq_save()
    write_lock_irq()     = write_lock() + local_irq_disable()
    write_lock_bh()      = write_lock() + local_bh_disable()

    write_unlock_irqrestore() = write_unlock() + local_irq_restore()
    write_unlock_irq()        = write_unlock() + local_irq_enable()
    write_unlock_bh()         = write_unlock() + local_bh_enable()


    读写自旋锁的一般使用方法：
    rwlock_t lock;                          /* 定义rwlock */
    rwlock_init(&lock);                     /* 初始化rwlock */
    ...
    read_lock(&lock);                       /* 读时获取锁 */
    ...                                     /* 临界资源 */
    read_unlock(&lock);
    ...
    write_lock_irqsave(&lock, flags);       /* 写时获取锁 */
    ...                                     /* 临界资源 */
    write_unlock_irqrestore(&lock, flags);
