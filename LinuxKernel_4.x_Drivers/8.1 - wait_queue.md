# Linux设备驱动中的阻塞与非阻塞I/O
    <<Linux设备驱动开发详解：基于最新的Linux 4.0内核>>


### 阻塞与非阻塞
    阻塞操作是指在执行设备操作时，若不能获得资源，则挂起进程直到满足可操作的条件后再进行操作；
    被挂起的进程进入睡眠状态，被从调度器的运行队列移走，直到等待的条件被满足；
    而非阻塞操作的进程在不能进行设备操作时，并不挂起，它要么放弃，要么不停地查询，直至可以进行操作为止；

    驱动程序通常需要提供这样的能力：
    1. 当应用程序进行 read()、write()等系统调用时，若设备的资源不能获取，而用户又希望以阻塞的方式访问设备，
       驱动程序应在设备驱动的 xxx_read()、xxx_write()等操作中将进程阻塞直到资源可以获取，此后应用程序的
       read()、write()等调用才返回，整个过程仍然进行了正确的设备访问，用户并没有感知到；
    2. 若用户以非阻塞的方式访问设备文件，则当设备资源不可获取时，设备驱动的 xxx_read(）、xxx_write()等
       操作应立即返回，read()、write()等系统调用也随即被返回，应用程序收到 -EAGAIN返回值；

    因为阻塞的进程会进入休眠状态，所以必须确保有一个地方能够唤醒休眠的进程；
    唤醒进程的地方最大可能发生在中断里面，因为在硬件资源获得的同时往往伴随着一个中断；


### 等待队列
    在Linux驱动程序中，可以使用等待队列（Wait Queue）来阻塞进程的唤醒；
    等待队列，以队列为基础数据结构，与进程调度机制紧密结合，可以用来同步对系统资源的访问；
    （信号量在内核中也依赖等待队列来实现）

##### 等待队列的操作
    1. 定义“等待队列头部”
    wait_queue_head_t my_queue;
    wait_queue_head_t 是 __wait_queue_head结构体的一个typedef；

    2. 初始化“等待队列头部”
    init_waitqueue_head(&my_queue);

    DECLARE_WAIT_QUEUE_HEAD(name)
    该宏可以作为定义并初始化等待队列头部的“快捷方式”；


    3. 定义等待队列元素
    DECLARE_WAITQUEUE(name, tsk)
    该宏用于定义并初始化一个名为name的等待队列元素；

    4. 添加/移除等待队列
    void add_wait_queue(wait_queue_head_t *q, wait_queue_t *wait);
    将等待队列元素wait添加到等待队列头部q指向的双向链表中；

    void remove_wait_queue(wait_queue_head_t *q, wait_queue_t *wait);
    将等待队列元素wait从由q头部指向的链表中移除；

    5. 等待事件
    wait_event(queue, condition)
    wait_event_interruptible(queue, condition)
    wait_event_timeout(queue, condition, timeout)
    wait_event_interruptible_timeout(queue, condition, timeout)

    queue     作为等待队列头部的队列被唤醒；
    condition 必须满足，否则继续阻塞；

    wait_event() 和 wait_event_interruptible() 的区别在于后者可以被信号打断，而前者不能；
    加上 _timeout后的宏意味着阻塞等待的超时时间，以 jiffy 为单位，
    在第3个参数的timeout到达时，不论condition是否满足，均返回；

    6. 唤醒队列
    void wake_up(wait_queue_head_t *queue);
    void wake_up_interruptible(wait_queue_head_t *queue);
    上述操作会唤醒以 queue作为等待队列头部的的队列中所有的进程；

    wake_up() 应该与 wait_event() 或 wait_event_timeout() 成对使用；
    wake_up_interruptible() 则应与 wait_event_interruptible()
    或 wait_event_interruptible_timeout() 成对使用；

    wake_up() 可唤醒处于 TASK_INTERRUPTIBLE 和 TASK_UNINTERRUPTIBLE 的进程；
    而 wake_up_interruptible() 只能唤醒处于 TASK_INTERRUPTIBLE 的进程；

    6. 在等待队列上睡眠
    sleep_on(wait_queue_head_t *q);
    将目前进程的状态置为 TASK_UNINTERRUPTIBLE，并定义一个等待队列元素，
    之后把它挂到等待队列头部q指向的双向链表，直到资源可获得，q队列指向链接的进程被唤醒；

    interruptible_sleep_on(wait_queue_head_t *q);
    将目前进程的状态置为 TASK_INTERRUPTIBLE，并定义一个等待队列元素，
    之后把它附属到q指向的队列，直到资源可获得（q指引的等待队列被唤醒）或进程收到信号；

    sleep_on() 应该与 wake_up() 成对使用；
    interruptible_sleep_on() 应该与 wake_up_interruptible() 成对使用；

##### 等待队列的模板
    static ssize_t xxx_write(struct file *file, const char *buffer, size_t count, loff_t *ppos)
    {
        ...
        DECLARE_WAITQUEUE(wait, current);                   /* 定义等待队列元素 */
        add_wait_queue(&xxx_wait, &wait);                   /* 添加元素到等待队列 */
        /* 等待设备缓冲区可写 */
        do {
            avail = device_writable(...);
            if (avail < 0) {
                if (file->f_flags & O_NONBLOCK) {           /* 非阻塞 */
                    ret = -EAGAIN;
                    goto out;
                }
                __set_current_state(TASK_INTERRUPTIBLE);    /* 改变进程状态 */
                schedule();                                 /* 调度其它进程执行 */
                if (signal_pending(current)) {              /* 如果是因为信号唤醒 */
                    ret = -ERESTARTSYS;
                    goto out;
                }
            }
        } while (avail < 0);
        /* 写设备缓冲区 */
        device_write(...);
    out:
        remove_wait_queue(&xxx_wait, &wait);                /* 将元素移除 xxx_wait 指引的队列 */
        set_current_state(TASK_RUNNING);                    /* 设置进程状态为 TASK_RUNNING */
        return ret;
    }

    1. 如果是非阻塞访问（O_NONBLOCK被设置），设备忙时，直接返回“-EAGAIN“；
    2. 对于阻塞访问，会调用 __set_current_state(TASK_INTERRUPTIBLE) 进行进程状态切换，
       并显式通过“schedule()”调度其它进程执行；
    3. 醒来的时候要注意，由于调度出去的时候，进程状态是 TASK_INTERRUPTIBLE，即浅度睡眠，
       所以唤醒它的可能是信号；可以通过 signal_pending(current) 了解是不是信号唤醒，
       如果是，立即返回”-ERESTARTSYSY“；

    在 wait_queue_head_t 指向的链表上，新定义的 wait_queue 元素被插入，
    而这个新插入的元素绑定了一个 task_struct
    （当前做 xxx_write的current，这也是 DECLARE_WAITQUEUE 使用“current”作为参数的原因）；
