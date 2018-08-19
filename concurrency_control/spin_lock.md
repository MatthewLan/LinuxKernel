# 自旋锁
    <<Linux设备驱动开发详解：基于最新的Linux 4.0内核>>


    自旋锁（Spin Lock）是一种典型的对临界资源进行互斥访问的手段；
    为了获得一个自旋锁，在某CPU上运行的代码需先执行一个原子操作，该操作测试并设置某个内存变量；
    由于它是原子操作，所以在该操作完成之前其它的执行单元不可能访问这个内存变量；

    如果测试结果表明锁已经空闲，则程序获得这个自旋锁并继续执行；
    如果测试结果表明锁仍被占用，程序将在一个小的循环内重复这个“测试并设置”操作，即进行所谓的“自旋”；
    当自旋锁的持有者通过重置该变量释放这个自旋锁后，某个等待的“测试并设置”操作向其调用者报告锁已释放；

    在ARM体系结构下，自旋锁的实现借用了 ldrex指令、strex指令、
    ARM处理器内存屏障指令 dmb 和 dsb、wfe指令和 sev指令；


##### Linux中与自旋锁相关的操作主要有以下4种：
    1. 定义自旋锁
    spinlock_t lock;

    2. 初始化自旋锁
    spin_lock_init(lock);
    该宏用于动态初始化自旋锁lock；

    3. 获得自旋锁
    spin_lock(lock);
    该宏用于获得自旋锁lock，如果能够立即获得锁，则马上返回；否则它将在那里自旋，直到该自旋锁的保持者释放；

    spin_trylock(lock);
    该宏尝试获得自旋锁lock，如果能立即获得锁，它获得锁并返回true，否则立即返回false；

    4. 释放自旋锁
    spin_unlock(lock);
    该宏释放自旋锁lock，它与 spin_lock 或 spin_trylock 配对使用；


    自旋锁的一般使用方法：
    spinlock_t lock;            /* 定义一个自旋锁 */
    spin_lock_init(&lock);      /* 初始化自旋锁 */
    spin_lock(&lock);           /* 获取自旋锁，保护临界区 */
    ...                         /* 临界区 */
    spin_unlock(&lock);         /* 解锁 */


##### 自旋锁和关中断的结合
    自旋锁主要针对SMP或单CPU 但内核可抢占的情况，对于单CPU和内核不支持抢占的系统，自旋锁退化为空操作；
    在单CPU和内核可抢占的系统中，自旋锁持有期间中内核的抢占将被禁止；
    在多核SMP的情况下，任何一个核拿到了自旋锁，该核上的抢占调度也暂时禁止了，但是没有禁止另外一个核的抢占调度；

    尽管用了自旋锁可以保证临界区不受别的CPU和本CPU内的抢占进程打扰，
    但是得到锁的代码路径在执行临界区的时候，还可能受到中断和底半部（BH）的影响；
    为了防止这种影响，就需要用到自旋锁的衍生；

    spin_lock()/spin_unlock() 是自旋锁机制的基础，它们和
    关中断 local_irq_disable()/开中断 local_irq_enable()、
    关底半部 local_bh_disable()/开底半部 local_bh_enable()、
    关中断并保存状态字 local_irq_save()/开中断并恢复状态字 local_irq_restore()
    结合就形成了整套自旋锁机制；

    spin_lock_irq()   = spin_lock() + local_irq_disable()
    spin_unlock_irq() = spin_unlock() + local_irq_enable()

    spin_lock_irqsave()      = spin_lock() + local_irq_save()
    spin_unlock_irqrestore() = spin_unlock() + local_irq_restore()

    spin_lock_bh()   = spin_lock() + local_bh_disable()
    spin_unlock_bh() = spin_unlock() + local_bh_enable()

    spin_lock_irq(), spin_lock_irqsave(), spin_lock_bh()
    类似函数会为自旋锁的使用系好“安全带”以避免突如其来的中断驶入对系统造成的伤害；

    在多核编程的时候，如果进程和中断可能访问同一片临界资源，一般需要
    在进程上下文中调用 spin_lock_irqsave()/spin_unlock_irqrestore()，
    在中断上下文中调用 spin_lock()/spin_unlock()；

    这样，在CPU0上，无论进程上下文，还是中断上下文获得了自旋锁，
    此后，如果CPU1无论是进程上下文，还是中断上下文，想获得同一自旋锁，都必须忙等待，这避免一切核间并发的可能性；
    同时，由于每个核的进程上下文持有锁的时候用的是 spin_lock_irqsave()，
    所以该核上的中断是不可能进入的，这避免了核内并发的可能性；


##### 自旋锁使用的注意点
    驱动工程师应谨慎使用自旋锁，而且在使用中还要特别注意如下几个问题：

    1. 自旋锁实际上是忙等锁，当锁不可用时，CPU一直循环执行“测试并设置”该锁直到可用而取得该锁，CPU在等待自旋锁时
       不做任何有用的工作，仅仅是等待；因此，只有在占用锁的时间极短的情况下，使用自旋锁才是合理的；
       当临界区很大，或有共享设备的时候，需要较长时间占用锁，使用自旋锁会降低系统的性能；
    2. 自旋锁可能导致系统死锁；
       引发这个问题最常见的情况是递归使用一个自旋锁，即如果一个已经拥有某个自旋锁的CPU想第二次获得这个自旋锁，
       则该CPU将死锁；
    3. 在自旋锁锁定期间不能调用可能引起进程调度的函数；
       如果进程获得自旋锁之后再阻塞，如调用 copy_from_user()、copy_to_user()、kmalloc() 和 msleep()
       等函数，则可能导致内核的崩溃；
    4. 在单核情况下编程的时候，也应该认为CPU是多核的，驱动特别强调跨平台的概念；
