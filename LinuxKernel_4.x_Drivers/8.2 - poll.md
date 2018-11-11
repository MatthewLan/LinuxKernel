# Linux设备驱动中的轮询编程
    <<Linux设备驱动开发详解：基于最新的Linux 4.0内核>>


### 应用程序中的轮询操作
    在用户程序中，select() 和 poll() 也是与设备阻塞与非阻塞访问息息相关的论题；
    使用非阻塞I/O的应用程序通常会使用 select()和 poll()系统调用查询是否可对设备进行无阻塞的访问；
    select()和poll()系统调用最终会使设备驱动中的 poll() 函数被执行，
    在Linux 2.5.45内核中还引入了 epoll()，即扩展的poll()；

##### select()
    int select(int numfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds
               , struct timeval *timeout);

    其中 readfds、writefds、exceptfds 分别是被select()监视的读、写和异常处理的文件描述符集合，
    numfds的值是需要检查的号码最高的 fd加 1；
    readfds文件集中的任何一个文件变得可读，select()返回；
    writefds()文件集中的任何一个文件变得可写，select()也返回；

    调用select()的时候，每个驱动的poll()接口都会被调用到，
    实际上执行select()的进程被挂到了每个驱动的等待队列上，可以被任何一个驱动唤醒；

##### poll()
    int poll(struct pollfd *fds, nfds_t nfds, int timeout);
    poll()的功能和实现原理与select()相似；

##### epoll()
    当多路复用的文件数量庞大、I/O流量频繁的时候，一般不太适合使用selec()和poll()，
    此种情况下，select()和poll()的性能表现较差，宜使用epoll；
    epoll()不会随着fd的数目増长而降低效率；

    int epoll_create(int size);
    创建一个epoll的句柄，size用来告诉内核要监听多少个fd；
    （自从Linux 2.6.8，参数size被忽略，内核会动态地调整大小，但必须大于0，以确保向后兼容性）
    当创建好epoll句柄后，它本身也会占用一个fd值，所以在使用完epoll后，必须调用close()关闭；

    int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
    告诉内核要监听什么类型的事件；
    参数epfd是epoll_create()的返回值；
    参数op表示动作：
      EPOLL_CTL_ADD：注册新的fd到epfd中；
      EPOLL_CTL_MOD：修改已经注册的fd的监听事件；
      EPOLL_CTL_DEL：从epfd中删除一个fd；
    参数fd表示将要被监听的fd；

    参数event是告诉内核需要监听的事件类型；
    typedef union epoll_data {
        void    *ptr;
        int      fd;
        uint32_t u32;
        uint64_t u64;
    } epoll_data_t;
    struct epoll_event {
        __uint32_t events;      /* Epoll events */
        epoll_data_t data;      /* User data variable */
    };
    events可以是以下几个宏的“或”：
      EPOLLIN ：对应的文件描述符可以读；
      EPOLLOUT：对应的文件描述符可以写；
      EPOLLPRI：对应的文件描述由紧急的数据可读（这里应该表示的是有socket带外数据到来）；
      EPOLLERR：对应的文件描述符发生错误；
      EPOLLHUP：对应的文件描述符被挂断；
      EPOLLET ：将epoll设为边缘触发模式（Edge Triggered），这是相对于水平触发（Level Triggered）来说的；
                LT是缺省的工作模式，在LT情况下，内核告诉用户一个fd是否就绪了，之后用户可以对这个就绪的fd进行
                I/O操作；但是如果用户不进行任何操作，该事件并不会丢失，
                而ET是高速工作方式，在这种模式下，当fd从未就绪变为就绪时，内核通过epoll告诉用户，然后它就会
                假设用户知道fd已经就绪，并且不会再为那个fd发送更多的就绪通知；
      EPOLLONESHOT：意味着一次性监听，当监听完这次事件后，如果还需要继续监听这个fd的话，
                需要再次把这个fd加入到epoll队列里；


    int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
    等待事件的产生，其中events参数时输出参数，用来从内核得到事件的集合，
    maxevents告诉内核本次最多收多少事件，maxevents的值不能大于创建epoll_create()时的size，
    参数timeout时超时时间（以毫秒为单位，0意味着立即返回，-1意味着永久等待）；
    该函数的返回值是需要处理的事件数目，若返回0，则表示已超时；


### 设备驱动中的轮询编程

##### poll()
    设备驱动中 poll()函数的原型是：
    unsigned int (*poll)(struct file *filp, struct poll_table *wait);

    第1个参数为file结构体指针，第2个参数为轮询表指针；
    这个函数应该进行两项工作：
    1. 对可能引起设备文件状态变化的等待队列调用 poll_wait()函数，将对应的等待队列头部添加到poll_table中；
    2. 返回表示是否能对设备进行无阻塞读、写访问的掩码；

##### poll_wait()
    用于向poll_table注册等待队列的关键poll_wait()函数的原型如下：
    void poll_wait(struct file *filp, wait_queue_head_t *queue, poll_table *wait);

    这个函数不会引起阻塞；
    poll_wait()函数所做的工作是把当前进程添加到wait参数指定的等待列表（poll_table）中，
    实际作用是让唤醒参数queue对应的等待队列可以唤醒因select()而睡眠的的进程；

##### 设备驱动中poll()函数的典型模板
    驱动程序poll()函数应该返回设备资源的可获取状态，即 POLLIN、POLLOUT、POLLPRI、POLLERR、POLLNVAL
    等宏的位“或”结果；
    每个宏的含义都表明设备的一种状态，如POLLIN（定义为0x0001）意味着设备可以无阻塞地读，
    POLLOUT（定义为0x0004）意味着设备可以无阻塞地写；

    << uapi/asm-generic/poll.h >>
    /* These are specified by iBCS2 */
    #define POLLIN      0x0001
    #define POLLPRI     0x0002
    #define POLLOUT     0x0004
    #define POLLERR     0x0008
    #define POLLHUP     0x0010
    #define POLLNVAL    0x0020

    /* The rest seem to be more-or-less nonstandard. Check them! */
    #define POLLRDNORM  0x0040
    #define POLLRDBAND  0x0080
    #ifndef POLLWRNORM
    #define POLLWRNORM  0x0100
    #endif
    #ifndef POLLWRBAND
    #define POLLWRBAND  0x0200
    #endif
    #ifndef POLLMSG
    #define POLLMSG     0x0400
    #endif
    #ifndef POLLREMOVE
    #define POLLREMOVE  0x1000
    #endif
    #ifndef POLLRDHUP
    #define POLLRDHUP       0x2000
    #endif

    #define POLLFREE    0x4000  /* currently only for epoll */


    poll()函数的典型模板：
    static unsigned int xxx_poll(struct file *filp, poll_table *wait)
    {
        unsigned int mask = 0;
        struct xxx_dev *dev = filp->private_data;   /* 获得设备结构体指针 */
        ...
        poll_wait(filp, &dev->r_wait, wait);        /* 加入读等待队列 */
        poll_wait(filp, &dev->w_wait, wait);        /* 加入写等待队列 */
        if (...)                                    /* 可读 */
            mask |= POLLIN | POLLRDNORM;            /* 标示数据可获得（对用户而言） */
        if (...)                                    /* 可写 */
            mask |= POLLOUT | POLLWRNORM;           /* 标示数据可写入 */
        ...
        return mask;
    }
