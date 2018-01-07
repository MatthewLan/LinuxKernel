
# 阻塞与非阻塞
    linux-4.14
    - include/linux/wait.h
    - include/linux/sched.h
    - include/linux/sched/signal.h
    - include/linux/poll.h


### 等待队列
    typedef struct wait_queue_head wait_queue_head_t;
    typedef struct wait_queue_entry wait_queue_entry_t;

    1. 定义和初始化“等待队列头部”
    struct wait_queue_head wq_head;
    init_waitqueue_head(wq_head); //宏定义 - &wq_head

    DECLARE_WAIT_QUEUE_HEAD(name); //宏定义

    2. 定义等待队列元素
    DECLARE_WAITQUEUE(name, tsk); //宏定义

    3. 添加/移除等待队列
    void add_wait_queue(struct wait_queue_head *wq_head, struct wait_queue_entry *wq_entry);
    将等待队列元素wq_entry 添加到等待队列头部wq_head 指向的双向链表中;

    void remove_wait_queue(struct wait_queue_head *wq_head, struct wait_queue_entry *wq_entry);
    将等待队列元素wq_entry 从由wq_head 头部指向的链表中移除;

    4. 等待事件
    wait_event(wq_head, condition); //宏定义
    wait_event_timeout(wq_head, condition, timeout); //宏定义
    wait_event_interruptible(wq_head, condition); //宏定义
    wait_event_interruptible_timeout(wq_head, condition, timeout); //宏定义

    a. 第1个参数wq_head 作为等待队列头部的队列被唤醒;
    b. 第2个参数condition 必须满足，否则继续阻塞;
    c. _interruptible的宏，可以被信号打断;
    d. _timeout的宏，意味着阻塞等待的超时时间，以jiffy为单位；在timeout到达时，不论condition是否满足，均返回;

    5. 唤醒队列
    void wake_up(struct wait_queue_head *wq_head); //宏定义
    void wake_up_interruptible(struct wait_queue_head *wq_head); //宏定义

    a. wake_up()               - wait_event()/wait_event_timeout()
       wake_up_interruptible() - wait_event_interruptible()/wait_event_interruptible_timeout()
    b. wake_up()可唤醒处于 TASK_INTERRUPTIBLE 和 TASK_UNINTERRUPTIBLE 的进程;
    c. wake_up_interruptible()只能唤醒处于 TASK_INTERRUPTIBLE 的进程;

    // 改变进程状态
    __set_current_state(state_value); //宏定义
    set_current_state(state_value); //宏定义

    // 调度其它进程执行
    void schedule(void);

    // 判断进程是否被信号唤醒
    int signal_pending(struct task_struct *p);


### 轮询操作
    struct file_operations {
        ...
        unsigned int (*poll) (struct file *filp, struct poll_table_struct *p);
        ...
    };
    a. 第1个参数为file结构体指针，第2个参数为轮询表指针；
    b. 对可能引起设备文件状态变化的等待队列调用 poll_wait() 函数，将对应的等待队列头部添加到poll_table中；
    c. 返回表示是否能对设备进行无阻塞读、写访问的掩码，
       即 POLLIN、POLLOUT、POLLPRI、POLLERR、POLLNVAL等宏的位或结果；

    void poll_wait(struct file *filp, wait_queue_head_t *wait_address, poll_table *p);
    a. 把当前进程添加到参数p指定的等待列表中，
       实际作用是让唤醒参数wait_address对应的等待队列可以唤醒因select()而睡眠的进程；
    b. 该函数不会引起阻塞；
