#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/sched/signal.h>
#include <linux/poll.h>


/************************************************
 * Macros
 ***********************************************/
#define GLOBALMEM_SIZE      0x1000
#define GLOBALMEM_MAJOR     238

#define GLOBALMEM_MAGIC     'g'
#define MEM_CLEAR           _IO(GLOBALMEM_MAGIC, 0)


/************************************************
 * Types
 ***********************************************/
struct globalmem_dev {
    struct cdev       cdev;
    unsigned int      cur_len;              /* 0 - FIFO empty; GLOBALMEM_SIZE - FIFO full */
    unsigned char     mem[GLOBALMEM_SIZE];
    struct mutex      mutex;
    wait_queue_head_t wq_head_r;
    wait_queue_head_t wq_head_w;
};


/************************************************
 * Variables
 ***********************************************/
static int globalmem_major = GLOBALMEM_MAJOR;
module_param(globalmem_major, int, S_IRUGO);

struct globalmem_dev *globalmem_devp = NULL;


/************************************************
 * Functions
 ***********************************************/
static loff_t globalmem_llseek(struct file *filp, loff_t offset, int orig)
{
    loff_t ret = 0;

    switch (orig) {
    case 0:     /* header position */
        if ((0 > offset) \
            || (GLOBALMEM_SIZE < (unsigned int)offset)) {
            ret = -EINVAL;
            break;
        }
        filp->f_pos = (unsigned int)offset;
        ret         = filp->f_pos;
        break;
    case 1:     /* current position */
        if ((0 > (filp->f_pos + offset)) \
            || (GLOBALMEM_SIZE < (filp->f_pos + offset))) {
            ret = -EINVAL;
            break;
        }
        filp->f_pos += offset;
        ret          = filp->f_pos;
        break;
    default:
        ret = -EINVAL;
        break;
    }
    return ret;
}

static ssize_t globalmem_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    int                  ret   = 0;
    struct globalmem_dev *dev  = filp->private_data;
    DECLARE_WAITQUEUE(wq_entry, current);

    mutex_lock(&dev->mutex);
    add_wait_queue(&dev->wq_head_r, &wq_entry);

    while (0 == dev->cur_len) {
        if (O_NONBLOCK & filp->f_flags) { /* 非阻塞情况 */
            ret = -EAGAIN;
            goto error_1;
        }
        __set_current_state(TASK_INTERRUPTIBLE); /* 标记task_struct的一个浅度睡眠标记，并未真正睡眠 */
        mutex_unlock(&dev->mutex); /* 把读进程调度出去前，必须释放互斥体，避免与写进程间死锁 */

        schedule(); /* 调度其它进程执行,读进程进入睡眠 */
        if (signal_pending(current)) {
            ret = -ERESTARTSYS;
            goto error_2;
        }
        mutex_lock(&dev->mutex);
    }
    if (count > dev->cur_len)
        count = dev->cur_len;

    if (copy_to_user(buf, dev->mem, count)) {
        ret = -EFAULT;
        goto error_1;
    }
    else {
        memcpy(dev->mem, dev->mem + count, dev->cur_len - count);
        dev->cur_len -= count;
        printk(KERN_INFO "read %ld bytes from %d\n", count, dev->cur_len);
        wake_up_interruptible(&dev->wq_head_w); /* 唤醒写进程 */
        ret = count;
    }

error_1:
    mutex_unlock(&dev->mutex);
error_2:
    remove_wait_queue(&dev->wq_head_r, &wq_entry);
    set_current_state(TASK_RUNNING);
    return ret;
}

static ssize_t globalmem_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    int                  ret   = 0;
    struct globalmem_dev *dev  = filp->private_data;
    DECLARE_WAITQUEUE(wq_entry, current);

    mutex_lock(&dev->mutex);
    add_wait_queue(&dev->wq_head_w, &wq_entry);

    while (GLOBALMEM_SIZE == dev->cur_len) {
        if (O_NONBLOCK & filp->f_flags) {
            ret = -EAGAIN;
            goto error_1;
        }
        __set_current_state(TASK_INTERRUPTIBLE); /* 标记task_struct的一个浅度睡眠标记，并未真正睡眠 */
        mutex_unlock(&dev->mutex); /* 把写进程调度出去前，必须释放互斥体，避免与读进程间死锁 */

        schedule(); /* 调度其它进程执行,写进程进入睡眠 */
        if (signal_pending(current)) {
            ret = -ERESTARTSYS;
            goto error_2;
        }
        mutex_lock(&dev->mutex);
    }
    if (count > GLOBALMEM_SIZE - dev->cur_len)
        count = GLOBALMEM_SIZE - dev->cur_len;

    if (copy_from_user(dev->mem + dev->cur_len, buf, count)) {
        ret = -EFAULT;
        goto error_1;
    }
    else {
        dev->cur_len += count;
        printk(KERN_INFO "written %ld bytes from %d\n", count, dev->cur_len);
        wake_up_interruptible(&dev->wq_head_r); /* 唤醒读进程 */
        ret = count;
    }

error_1:
    mutex_unlock(&dev->mutex);
error_2:
    remove_wait_queue(&dev->wq_head_w, &wq_entry);
    set_current_state(TASK_RUNNING);
    return ret;
}

static unsigned int globalmem_poll(struct file *filp, struct poll_table_struct *p)
{
    unsigned int         mask = 0;
    struct globalmem_dev *dev = filp->private_data;

    mutex_lock(&dev->mutex);
    poll_wait(filp, &dev->wq_head_r, p);    /* 添加读等待队列 */
    poll_wait(filp, &dev->wq_head_w, p);    /* 添加写等待队列 */

    if (0 != dev->cur_len)
        mask |= POLLIN | POLLRDNORM;        /* 标识数据可读（对用户而言） */
    if (GLOBALMEM_SIZE != dev->cur_len)
        mask |= POLLOUT | POLLWRNORM;       /* 标识数据可写 */
    mutex_unlock(&dev->mutex);
    return mask;
}

static long globalmem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long                 ret  = 0;
    struct globalmem_dev *dev = filp->private_data;

    switch (cmd) {
    case MEM_CLEAR:
        mutex_lock(&dev->mutex);
        memset(dev->mem, 0, GLOBALMEM_SIZE);
        mutex_unlock(&dev->mutex);
        printk(KERN_INFO "globalmem is set to zero\n");
        break;
    default:
        ret = -EINVAL;
        break;
    }
    return ret;
}

static int globalmem_open(struct inode *inode, struct file *filp)
{
    filp->private_data = globalmem_devp;
    return 0;
}

static int globalmem_release(struct inode *inode, struct file *filp)
{
    return 0;
}


static const struct file_operations globalmem_fops = {
    .owner          = THIS_MODULE,
    .llseek         = globalmem_llseek,
    .read           = globalmem_read,
    .write          = globalmem_write,
    .poll           = globalmem_poll,
    .unlocked_ioctl = globalmem_ioctl,
    .open           = globalmem_open,
    .release        = globalmem_release,
};

static int __init globalmem_init(void)
{
    int   ret    = 0;
    dev_t dev_no = MKDEV(globalmem_major, 0);

    if (globalmem_major) {
        printk(KERN_INFO "register_chrdev_region\n");
        ret = register_chrdev_region(globalmem_major, 1, "globalmem");
    }
    else {
        printk(KERN_INFO "alloc_chrdev_region\n");
        ret = alloc_chrdev_region(&dev_no, 0, 1, "globalmem");
        globalmem_major = MAJOR(dev_no);
    }
    if (0 > ret)
        goto error;

    if (!(globalmem_devp = kzalloc(sizeof(struct globalmem_dev), GFP_KERNEL))) {
        ret = -ENOMEM;
        goto error_nomem;
    }
    cdev_init(&globalmem_devp->cdev, &globalmem_fops);
    globalmem_devp->cdev.owner = THIS_MODULE;
    if ((ret = cdev_add(&globalmem_devp->cdev, dev_no, 1))) {
        printk(KERN_NOTICE "ERROR %d adding globalmem\n", ret);
        goto error_add;
    }
    mutex_init(&globalmem_devp->mutex);
    init_waitqueue_head(&globalmem_devp->wq_head_r);
    init_waitqueue_head(&globalmem_devp->wq_head_w);

    printk(KERN_NOTICE "globalmem_init OK\n");
    return 0;

error_add:
    kfree(globalmem_devp);
error_nomem:
    unregister_chrdev_region(dev_no, 1);
error:
    return ret;
}

static void __exit globalmem_exit(void)
{
    cdev_del(&globalmem_devp->cdev);
    kfree(globalmem_devp);
    unregister_chrdev_region(MKDEV(globalmem_major, 0), 1);
    printk(KERN_NOTICE "globalmem_exit OK\n");
}

module_init(globalmem_init);
module_exit(globalmem_exit);
MODULE_AUTHOR("Matthew Lan");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Globalmem Module");
