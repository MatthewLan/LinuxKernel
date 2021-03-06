#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>


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
    struct cdev   cdev;
    unsigned char mem[GLOBALMEM_SIZE];
    struct mutex  mutex;
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

static ssize_t globalmem_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    unsigned long        p     = *ppos;
    unsigned int         count = size;
    int                  ret   = 0;
    struct globalmem_dev *dev  = filp->private_data;

    mutex_lock(&dev->mutex);
    do {
        if (GLOBALMEM_SIZE <= p)
            break;
        if (count > GLOBALMEM_SIZE - p)
            count = GLOBALMEM_SIZE - p;

        if (copy_to_user(buf, dev->mem + p, count))
            ret = -EFAULT;
        else {
            *ppos += count;
            ret    = count;
            printk(KERN_INFO "read %u bytes from %lu\n", count, p);
        }
    } while (0);
    mutex_unlock(&dev->mutex);
    return ret;
}

static ssize_t globalmem_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    unsigned long        p     = *ppos;
    unsigned int         count = size;
    int                  ret   = 0;
    struct globalmem_dev *dev  = filp->private_data;

    mutex_lock(&dev->mutex);
    do {
        if (GLOBALMEM_SIZE <= p)
            break;
        if (count > GLOBALMEM_SIZE - p)
            count = GLOBALMEM_SIZE - p;

        if (copy_from_user(dev->mem + p, buf, count))
            ret = -EFAULT;
        else {
            *ppos += count;
            ret    = count;
            printk(KERN_INFO "written %u bytes from %lu\n", count, p);
        }
    } while (0);
    mutex_unlock(&dev->mutex);
    return ret;
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
