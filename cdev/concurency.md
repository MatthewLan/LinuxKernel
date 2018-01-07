
# 并发控制
    linux-4.14
    - include/linux/irqflags.h
    - include/linux/bottom_half.h
    - include/asm-generic/atomic.h
    - include/asm-generic/bitops/atomic.h
    - include/linux/spinlock.h
    - include/linux/rwlock.h
    - include/linux/seqlock.h
    - include/linux/rcupdate.h
    - include/linux/rculist.h
    - include/linux/semaphore.h
    - include/linux/mutex.h
    - include/linux/completion.h


### 中断屏蔽
    禁止/使能本CPU内的中断，不能解决SMP多CPU引发的竞态
    local_irq_disable - local_irq_enable()

    禁止中断，并保存目前CPU的中断位信息
    local_irq_save()
    使能中断，并恢复原先CPU的中断位信息
    local_irq_restore()

    关底半部/开底半部
    local_bh_disable() - local_bh_enable()


### 原子操作
    保证对一个整型数据的修改时排他性的;

##### 整型原子操作
    1. 设置原子变量的值
    void atomic_set(atomic_t *v, int i); //宏定义
    atomic_t v = ATOMIC_INIT(i);

    2. 获取原子变量的值
    void atomic_read(atomic_t *v); //宏定义

    3. 原子变量加/减
    void atomic_add(int i, atomic_t *v);
    void atomic_sub(int i, atomic_t *v);

    4. 原子变量自增/自减
    void atomic_inc(atomic_t *v);
    void atomic_dec(atomic_t *v);

    5. 操作并测试
    int atomic_inc_and_test(atomic_t *v);
    int atomic_dec_and_test(atomic_t *v);
    int atomic_sub_and_test(int i, atomic_t *v);
    对原子变量执行自增、自减、减操作后，测试其是否为0；为0返回true，否则返回flase;

    6. 操作并返回
    int atomic_add_return(int i, atomic_t *v);
    int atomic_sub_return(int i, atomic_t *v);
    int atomic_inc_return(atomic_t *v); //宏定义
    int atomic_dec_return(atomic_t *v); //宏定义

##### 位原子操作
    1. 设置位: 设置addr地址的第nr位为1
    void set_bit(int nr, volatile unsigned long *addr);

    2. 清除位: 设置addr地址的第nr位为0
    void clear_bit(int nr, volatile unsigned long *addr);

    3. 改变位: 对addr地址的第nr位进行反置
    void change_bit(int nr, volatile unsigned long *addr);

    4. 测试位: 返回addr地址的第nr位
    int test_bit(int nr, const volatile unsigned long *addr);

    5. 测试并操作位
    int test_and_set_bit(int nr, volatile unsigned long *addr);
    int test_and_clear_bit(int nr, volatile unsigned long *addr);
    int test_and_change_bit(int nr, volatile unsigned long *addr);


### 自旋锁
##### 自旋锁的使用
    1. 定义自旋锁
    spinlock_t lock;

    2. 初始化自旋锁
    spin_lock_init(spinlock_t *lock); //宏定义

    3. 获得自旋锁
    void spin_lock(spinlock_t *lock);
    int spin_trylock(spinlock_t *lock);

    4. 释放自旋锁
    void spin_unlock(spinlock_t *lock);

    a. 自旋锁主要针对SMP、或单CPU但内核可抢占的情况; 对于单CPU和内核不支持抢占的系统，自旋锁退化为空操作;
    b. 得到锁的代码路径在执行临界区的时候，还可能受到中断和底半部BH的影响；
    c. spin_lock()/spin_unlock() 是自旋锁机制的基础，它们和
       关中断 local_irq_disable()/开中断l ocal_irq_enable()、
       关底半部 local_bh_disable()/开底半部 local_bh_enable()、
       关中断并保存状态字 local_irq_save()/开中断并恢复状态字 local_irq_restore()
       结合就形成了整套自旋锁机制；

    void spin_lock_irq(spinlock_t *lock);
        = spin_lock() + local_irq_disable()
    void spin_unlock_irq(spinlock_t *lock);
        = spin_unlock() + local_irq_enable()

    void spin_lock_irqsave(raw_spinlock_t *lock, unsigned long flags); //宏定义
        = spin_lock() + local_irq_save()
    void spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags);
        = spin_unlock() + local_irq_restore()

    void spin_lock_bh(spinlock_t *lock);
        = spin_lock() + local_bh_disable()
    void spin_unlock_bh(spinlock_t *lock);
        = spin_unlock() + local_bh_enable()

    int spin_trylock_irq(spinlock_t *lock);
    int spin_trylock_irqsave(spinlock_t *lock, unsigned long flags); //宏定义
    int spin_trylock_bh(spinlock_t *lock);

    int spin_is_locked(spinlock_t *lock);
    int spin_can_lock(spinlock_t *lock);

    d. 进程上下文中调用 spin_lock_irqsave()/spin_unlock_irqrestore;
       中断上下文中调用 spin_lock()/spin_unlock();
    e. 只有在占用锁的时间极短的情况下，使用自旋锁才是合理的；
       当临界区很大，或有共享设备的时候，需要较长时间占用锁，使用自旋锁会降低系统的性能；
    f. 自旋锁可能导致系统死锁；
    g. 在自旋锁锁定期间不能调用可能引起进程调度的函数，否则可能导致内核的崩溃；
       eg. copy_from_user(), copy_to_user(), kmalloc(), msleep()等
    l. 考虑跨平台，无论单/多核，在中断服务程序里也应该调用spin_lock();
       在单CPU情况下，若中断和进程可能访问同一临界区，进程里调用 spin_lock_irqsave()是安全的，在中断里其实
       不调用 spin_lock()也没有问题，因为spin_lock_irqsave()可以保证这个CPU的中断服务程序不可能执行；
       但是若CPU为多核，spin_lock_irqsave()不能屏蔽另外一个核的中断，所以另一个核可能造成并发问题；

##### 读写自旋锁
    在写操作方面，只能最多有1个写进程；在读方面，同时可以有多个读执行单元；
    读和写不能同时进行；

    1. 定义和初始化读写自旋锁
    rwlock_t lock;
    void rwlock_init(rwlock_t *lock); //宏定义

    2. 读锁定 (宏定义)
    void read_lock(rwlock_t *lock);
    void read_trylock(rwlock_t *lock);
    void read_lock_irq(rwlock_t *lock);
    void read_lock_irqsave(rwlock_t *lock, unsigned long flags);
    void read_lock_bh(rwlock_t *lock);

    3. 读解锁 (宏定义)
    void read_unlock(rwlock_t *lock);
    void read_unlock_irq(rwlock_t *lock);
    void read_unlock_irqrestore(rwlock_t *lock, unsigned long flags);
    void read_unlock_bh(rwlock_t *lock);

    4. 写锁定 (宏定义)
    void write_lock(rwlock_t *lock);
    void write_trylock(rwlock_t *lock);
    void write_lock_irq(rwlock_t *lock);
    void write_lock_irqsave(rwlock_t *lock, unsigned long flags);
    void write_lock_bh(rwlock_t *lock);

    5. 写解锁 (宏定义)
    void write_unlock(rwlock_t *lock);
    void write_unlock_irq(rwlock_t *lock);
    void write_unlock_irqrestore(rwlock_t *lock, unsigned long flags);
    void write_unlock_bh(rwlock_t *lock);

##### 顺序锁
    读执行单元不会被写执行单元阻塞；
    写执行单元与写执行单元之间仍然是互斥的；
    如果读执行单元在读操作期间，写执行单元已经发生了写操作，那么读执行单元必须重新读取数据，以确保得到的数据是完整的；

    1. 定义和初始化顺序锁
    seqlock_t sl;
    void seqlock_init(seqlock_t *sl); //宏定义 - 动态定义

    DEFINE_SEQLOCK(lockname); //静态定义

    2. 获取顺序锁
    void write_seqlock(seqlock_t *sl);
    void write_seqlock_irq(seqlock_t *sl);
    void write_seqlock_irqsave(seqlock_t *sl, unsigned long flags); //宏定义
    void write_sequnlock_bh(seqlock_t *sl);

    3. 释放顺序锁
    void write_sequnlock(seqlock_t *sl);
    void write_sequnlock_irq(seqlock_t *sl);
    void write_sequnlock_irqrestore(seqlock_t *sl, unsigned long flags);
    void write_sequnlock_bh(seqlock_t *sl);

    4. 读操作
    // 读执行单元在对被顺序锁sl保护的共享资源进行访问前需要调用该函数，该函数返回顺序锁sl的当前序号
    unsigned read_seqbegin(const seqlock_t *sl); //读开始

    // 读执行单元在访问完被顺序锁sl保护的共享资源后，需调用该函数来检查在读访问期间是否有写操作；
    // 如果有写操作，读执行单元就需要重新进行读操作
    unsigned read_seqretry(const seqlock_t *sl, unsigned start); //重读

    do {
        seqnum = read_seqbegin(&sl);
        //读操作代码
        ...
    } while (read_seqretry(&sl, seqnum));

##### RCU (读-复制-更新)
    a. 使用RCU的读端没有锁、内存屏障、原子指令类的开销，几乎可以认为是直接读(只是简单地标记读开始和读结束)，
       而RCU的写执行单元在访问它的共享资源前首先复制一个副本，然后对副本进行修改，最后使用一个回调机制在适当的时机
       把指向原来数据的指针重新指向新的被修改的数据，这个时机就是所有引用该数据的CPU都退出对共享数据读操作的时候；
    b. 等待适当时机的这一时期称为宽限期；
    c. 如果写比较多时，对读执行单元的性能提高不能弥补写执行单元同步导致的损失；

    1. 读锁定
    void rcu_read_lock(void);
    void rcu_read_lock_bh(void);

    2. 读解锁
    void rcu_read_unlock(void);
    void rcu_read_unlock_bh(void);

    3. 同步RCU
    void synchronize_rcu(void);

    3.1 该函数由RCU写执行单元调用，它将阻塞写执行单元，直到当前CPU上所有的已经存在的读执行单元完成读临界区，
        写执行单元才可以继续下一步操作；
    3.2 synchronize_rcu()并不需要等待后续读临界区的完成；

    4. 挂接回调
    void call_rcu(struct rcu_head *head, rcu_callback_t func);
    该函数由RCU写执行单元调用，它不会使写执行单元阻塞，因而可以在中断上下文或软中断中个使用；
    该函数把函数func挂接到RCU回调函数链上，然后立即返回；挂接的回调函数会在一个宽限期结束后被执行；

    rcu_assign_pointer(p, v)
    给RCU保护的指针赋一个新的值;

    rcu_dereference(p)
    获取一个RCU保护的指针，之后可以安全地引用它；
    一般需要在rcu_read_lock()/rcu_read_unlock()保护的区间引用这个指针；

    rcu_access_pointer(p)
    获取一个RCU保护的指针，之后并不引用它；可以用来判断指针是否为NULL；

    example:
        struct foo {
            int a;
            int b;
            int c;
        };
        struct foo *gp = NULL;
        struct foo *p  = NULL;
        ...
        p = kmalloc(sizeof(*p), GFP_KERNEL);
        p->a = 1;
        p->b = 2;
        p->c = 3;
        rcu_assign_pointer(gp, p);
        ...
        rcu_read_lock();
        if ((p = rcu_dereference(gp))) {
            do_something_with(p->a, p->b, p->c);
        }
        rcu_read_unlock();

    void list_add_rcu(struct list_head *new, struct list_head *head);
    把链表元素new插入RCU保护的链表head的开头；

    list_add_tail_rcu(struct list_head *new, struct list_head *head);
    把链表元素new插入RCU保护的链表head的末尾；

    void list_del_rcu(struct list_head *entry);
    从RCU保护的链表中删除指定的链表元素entry；

    void list_replace_rcu(struct list_head *old, struct list_head *new);
    使用新的链表元素new取代旧的链表元素old；

    list_for_each_entry_rcu(pos, head, member);
    该宏用于遍历由RCU保护的链表head；
    只要在读执行单元临界使用该函数，就可以安全地和其它RCU保护的链表操作函数并发运行；

    example:
        struct foo {
            struct list_head list;
            int a;
            int b;
            int c;
        };
        struct foo *p  = NULL;
        LIST_HEAD(head);
        ...
        p = kmalloc(sizeof(*p), GFP_KERNEL);
        p->a = 1;
        p->b = 2;
        p->c = 3;
        list_add_rcu(&p->list, &head);
        ...
        rcu_read_lock();
        list_for_each_entry_rcu(p, head, list) {
            do_something_with(p->a, p->b, p->c);
        }
        rcu_read_unlock();


### 信号量
    信号量的值可以是0、1或者n；
    1. 定义和初始化信号量
    struct semaphore sem;
    void sema_init(struct semaphore *sem, int val);

    DEFINE_SEMAPHORE(name); //宏定义

    2. 获得信号量
    void down(struct semaphore *sem);
    该函数用于获得信号量sem，它会导致睡眠，因此不能在中断上下文中使用；
    进入睡眠状态的进程不能被信号打断；

    int __must_check down_interruptible(struct semaphore *sem);
    进入睡眠状态的进程能被信号打断；
    信号也会导致该函数返回，返回值为非0；
        if (down_interruptible(&sem))
            return -ERESTARTSYS;

    int __must_check down_trylock(struct semaphore *sem);
    尝试获得信号量sem，若能立刻获得，则获得该信号量并返回0，否则返回非0；
    该函数不会导致调用者睡眠，可以在中断上下文中使用；

    3. 释放信号量
    void up(struct semaphore *sem);

    与自旋锁不同的是，当获取不到信号量时，进程不会原地打转，而是进入休眠等待状态；
    由于新的linux内核倾向与直接使用mutex作为互斥手段，信号量左互斥不再被推荐使用；


### 互斥体
    1. 定义和初始化互斥体
    struct mutex lock;
    mutex_init(mutex); //宏定义 - &lock

    DEFINE_MUTEX(mutexname); //宏定义

    2. 获取互斥体
    void mutex_lock(struct mutex *lock);
    int __must_check mutex_lock_interruptible(struct mutex *lock);
    int mutex_trylock(struct mutex *lock);

    3. 释放互斥体
    void mutex_unlock(struct mutex *lock);

    a. 互斥体和自旋锁属于不同层次的互斥手段，前者的实现依赖于后者；
       在互斥体本身的实现上，为了保证互斥体结构存取的原子性，需要自旋锁来互斥；
    b. 互斥体是进程级的，用于多个进程之间对资源的互斥，虽然也在内核中，但是内核执行路径是以进程的身份，
       代表进程来争夺资源的；如果竞争失败，会发生进程上下文切换，当前进程进入睡眠状态，CPU将运行其它进程；
    c. 当锁不能被获取到时，使用互斥体的开销时进程上下文切换时间，使用自旋锁的开销时等待获取自旋锁；
       若临界区比较小，宜使用自旋锁；若临界区很大，应使用互斥体；
    d. 互斥体所保护的临界区可包含可能引起阻塞的代码，而自旋锁则绝对要避免用来保护包含这样的代码；
    e. 互斥体存在于进程上下文，因此如果被保护的共享资源需要在中断或软中断情况下使用，则只能选择自旋锁；
       如果一定要使用互斥体，只能通过方式进行，不能获取就立即返回，以避免阻塞；


### 完成量
    用于一个执行单元等待另一个执行单元执行完某事；
    1. 定义和初始化完成量
    struct completion x;
    init_completion(x); //宏定义 - &x
    void reinit_completion(struct completion *x);

    DECLARE_COMPLETION(work); //宏定义

    2. 等待完成量
    void wait_for_completion(struct completion *x);
    int wait_for_completion_interruptible(struct completion *x);
    bool try_wait_for_completion(struct completion *x);

    3. 唤醒完成量
    // 只唤醒一个等待的执行单元
    void complete(struct completion *x);
    // 唤醒所有等待同一完成量的执行单元
    void complete_all(struct completion *x);
