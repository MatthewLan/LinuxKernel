# 顺序锁
    <<Linux设备驱动开发详解：基于最新的Linux 4.0内核>>


    RCU（Read-Copy-Update），读-复制-更新；

    使用RCU的读端没有锁、内存屏障、原子指令类的开销，几乎可以认为是直接读（只是简单地标明读开始和读结束），
    而RCU的写执行单元在访问它的共享资源前首先复制一个副本，然后对副本进行修改，
    最后使用一个回调机制在适当的时机把指向原来数据的指针重新指向新的被修改的数据，
    这个时机就是所有引用该数据的CPU都退出对共享数据读操作的时候；

    等待适当时机的这一时期成为宽限期（Crace Period）；

    RCU可以看作读写锁的高性能版本，相比读写锁，RCU的优点在于
    既允许多个读执行单元同时访问被保护的数据，又允许多个读执行单元和多个写执行单元同时访问被保护的数据；

    但是，RCU不能替代读写锁，因为如果写比较多时，对读执行单元的性能提高不能弥补写执行单元同步导致的损失；
    因为使用RCU时，写执行单元之间的同步开销会比较大，它需要延迟数据结构的释放，
    复制被修改的数据结构，它也必须使用某种锁机制来同步并发的其它写执行单元的修改操作；


##### RCU操作
    1. 读锁定
    rcu_read_lcok()
    rcu_read_lock_bh()

    2. 读解锁
    rcu_read_unlock()
    rcu_read_unlock_bh()


    使用RCU进行读的模式如下：
    rcu_read_lock();
    ... /* 读临界区 */
    rcu_read_unlock();


    3. 同步RCU
    synchronize_rcu();

    该函数由RCU写执行单元调用，它将阻塞写执行单元，直到当前CPU上所有的已经存在（Ongoing）
    的读执行单元完成读临界区，写执行单元才可以继续下一步操作；
    synchronize_rcu()并不需要等待后续（Subsequent）读临界区的完成；

    CPU0                    CPU1                        CPU2
    rcu_read_lock()
                            synchronize_rcu()进入
    rcu_read_unlock()                                   rcu_read_lock()
                            synchronize_rcu()退出
                                                        rcu_read_unlock()

    3. 挂接回调
    void call_rcu(struct rcu_head *head, void (*func)(struct rcu_head *rcu));

    函数cal_rcu()也由RCU写执行单元调用，与synchronize_rcu()不同的是，它不会使写执行单元阻塞，
    因而可以在中断上下文或软中断中使用；
    该函数把函数func挂接到RCU回调函数链上，然后立即返回；
    挂接的回调函数会在一个宽限期结束（即所有已经存在的RCU读临界区完成）后被执行；


    4. 其它操作 - 指针
    rcu_assign_pointer(p, v);
    给RCU保护的指针p赋一个新的值；

    rcu_dereference(p);
    读端使用该函数获取一个RCU保护的的指针，之后既可以安全地引用它；
    一般需要在 rcu_read_lock()/rcu_read_unlocl()保护的区间引用这个指针；

    ruc_access_pointer(p);
    读端使用该函数获取一个RCU保护的指针，之后并不引用它；


    e.g.
    /* 写端分配一个内存，并初始化，之后把其地址赋值给全局的指针 */
    struct foo {
        int a;
        int b;
        int c;
    };
    struct foo *gp = NULL;
    /* ... */
    p = kmalloc(sizeof(*p), GFP_KERNEL);
    p->a = 1;
    p->b = 2;
    p->c = 3;
    rcu_assign_pointer(gp, p);

    /* 读端访问该片区域 */
    rcu_read_lcok();
    p = rcu_dereference(gp);
    if (NULL != p) {
        do_something_with(p->a, p->b, p->c);
    }
    rcu_read_unlock();


    5. 其它操作 - 链表
    static inline void list_add_rcu(struct list_head *new, struct list_head *head);
    把链表元素new插入RCU保护的链表head的开头；

    static inline void list_add_tail_rcu(struct list_head *new, struct list_head *head);
    把链表元素new插入RCU保护的链表head的末尾；

    static inline void list_del_rcu(struct list_head *entry);
    从RCU保护的链表中删除指定的链表元素entry；

    static inline void list_replace_rcu(struct list_head *old, struct list_head *new);
    使用新的链表元素new取代旧的链表元素old；

    list_for_each_entry_rcu(pos, head)
    该宏用于遍历由RCU保护的链表head，
    只要在读执行单元临界区使用该函数，它就可以安全地和其它RCU保护地链表操作函数并发运行；


    e.g.
    /* 链表的写端代码模型 */
    struct foo {
        struct list_head list;
        int a;
        int b;
        int c;
    };
    LIST_HEAD(head);
    /* ... */
    p = kmalloc(sizeof(*p), GFP_KERNEL);
    p->a = 1;
    p->b = 2;
    p->c = 3;
    list_add_rcu(&p->list, &head);

    /* 链表的读端代码模型 */
    rcu_read_lock();
    list_for_each_entry_rcu(p, head, list) {
        do_something_with(p->a, p->b, p->c);
    }
    rcu_read_unlock();
