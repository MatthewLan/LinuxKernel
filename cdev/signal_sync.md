
# 异步通知与异步I/O
    linux-4.14
    - include/uapi/asm-generic/signal.h
    - include/uapi/asm-generic/siginfo.h
    - include/linux/fs.h
    - fs/fcntl.c


### SIGxxx
    信号            值           含义
    SIGHUP          1           挂起
    SIGINT          2           终端中断
    SIGQUIT         3           终端退出
    SIGILL          4           无效命令
    SIGTRAP         5           跟踪陷阱
    SIGIOT          6           IOT陷阱
    SIGBUS          7           BUS错误
    SIGFPE          8           浮点异常
    SIGKILL         9           强行终止（不能被捕获或忽略）
    SIGUSR1         10          用户定义的信号1
    SIGSEGV         11          无效的内存段处理
    SIGUSR2         12          用户定义的信号2
    SIGPIPE         13          半关闭管道的写操作已经发生
    SIGALRM         14          计时器到期
    SIGTERM         15          终止
    SIGSTKFLT       16          堆栈错误
    SIGCHLD         17          子进程已经停止或退出
    SIGCONT         18          如果停止了，继续执行
    SIGSTOP         19          停止执行（不能被捕获或忽略）
    SIGTSTP         20          终端停止信号
    SIGTTIN         21          后台进程需要从终端读取输入
    SIGTTOU         22          后台进程需要从终端读取写出
    SIGURG          23          紧急的套接字事件
    SIGXCPU         24          超额使用CPU分配的时间
    SIGXFSZ         25          文件尺寸超额
    SIGVTALRM       26          虚拟时钟信号
    SIGPROF         27          时钟信号描述
    SIGWINCH        28          窗口尺寸变化
    SIGIO           29          I/O
    SIGPWR          30          断电重启

### 信号的接收
    为了能在用户空间中处理一个设备释放的信号：
    1. 通过 F_SETOWN IO控制命令设置设备文件的拥有者为本进程，这样从设备驱动发出的信号才能被本进程接受到；
    2. 通过 F_SETFL IO控制命令设置设备文件以支持 FASYNC，即异步同志模式；
    3. 通过 signal()函数连接信号和信号处理函数；

### 信号的释放
    为了使设备支持异步通知机制：
    1. 支持 F_SETOWN 命令，能在这个控制命令处理中设置 flip->f_owner 为对应进程ID；
       该项工作已由内核完成，设备驱动无须处理；
    2. 支持 F_SETFL 命令的处理，每当 FASYNC 标志改变时，驱动程序中 fasync()函数将得以执行；
       因此驱动中应实现fasync()函数；
    3. 在设备资源可获得时，调用 kill_fasync()函数激发相应的信号；


### struct fasync_struct
    struct fasync_struct {
        spinlock_t           fa_lock;
        int                  magic;
        int                  fa_fd;
        struct fasync_struct *fa_next;  /* singly linked list */
        struct file          *fa_file;
        struct rcu_head      fa_rcu;
    };

### fasync_helper()
    int fasync_helper(int fd, struct file * filp, int on, struct fasync_struct **fapp)
    {
        if (!on)
            return fasync_remove_entry(filp, fapp);
        return fasync_add_entry(fd, filp, fapp);
    }
    处理 FASYNC 标记变更的函数;

### kill_fasync()
    void kill_fasync(struct fasync_struct **fp, int sig, int band)
    {
        if (*fp) {
            rcu_read_lock();
            kill_fasync_rcu(rcu_dereference(*fp), sig, band);
            rcu_read_unlock();
        }
    }
    释放信号用的函数;
        可读时，设置为 POLL_IN;
        可写时，设置为 POLL_OUT;

### struct file_operations
    struct file_operations {
        ...
        int (*fasync) (int fd, struct file *filp, int on);
        ...
    };
