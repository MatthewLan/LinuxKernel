# 信号量
    <<Linux设备驱动开发详解：基于最新的Linux 4.0内核>>


    信号量（Semaphore）是操作系统中最典型地用于同步和互斥地手段，信号量的值可以是0、1或n；
    信号量与操作系统中地经典概念PV操作对应；

    P（S）： 如果信号量S的值大于零，该进程继续执行；
            如果信号量S的值为零，将该进程置为等待状态，排入信号量地等待队列，直到V操作唤醒之；


##### 信号量相关的操作
    1. 定义信号量
    struct semaphore sem;

    2. 初始化信号量
    void sema_init(struct semaphore *sem, int val);
    初始化信号量，并设置信号量sem的值为val；

    3. 获得信号量
    void down(struct semaphore *sem);
    用于获得信号量sem，它会导致睡眠，因此不能在中断上下文中使用；
    因为down()进入睡眠状态的进程不能被信号打断；

    int down_interruptible(struct semaphore *sem);
    因为down_interruptible()进入睡眠状态的进程能被信号打断，信号也会导致该函数返回，此时函数返回值非0；
    使用该函数，对返回值一般会进行检查，如果非0，通常立即返回 -ERESTARTSYS;
    e.g.
    if (down_interruptible(&sem))
        return -ERESTARTSYS;

    int down_trylock(struct semaphore *sem);
    该函数尝试获得信号量sem()，如果能够立刻获得，它就获得该信号量并返回0，否则返回非0值；
    它不会导致调用者睡眠，可以在中断上下文中使用；

    4. 释放信号量
    void up(struct semaphore *sem);
    该函数释放信号量sem，唤醒等待者；
