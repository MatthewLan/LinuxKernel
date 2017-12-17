
# device
    linux-4.14
    - include/linux/device.h
    - drivers/base/core.c


### struct device
    struct device {
        struct device       *parent;

        struct device_private   *p;

        struct kobject kobj;
        const char      *init_name;     /* initial name of the device */
        const struct device_type *type;

        struct mutex        mutex;      /* mutex to synchronize calls to its driver. */

        struct bus_type      *bus;      /* type of bus device is on */
        struct device_driver *driver;   /* which driver has allocated this device */
        void        *platform_data;     /* Platform specific data, device core
                                           doesn't touch it */
        void        *driver_data;       /* Driver data, set and get with dev_set/get_drvdata */
        struct dev_links_info   links;
        struct dev_pm_info      power;
        struct dev_pm_domain    *pm_domain;

    #ifdef CONFIG_GENERIC_MSI_IRQ_DOMAIN
        struct irq_domain   *msi_domain;
    #endif
    #ifdef CONFIG_PINCTRL
        struct dev_pin_info *pins;
    #endif
    #ifdef CONFIG_GENERIC_MSI_IRQ
        struct list_head    msi_list;
    #endif

    #ifdef CONFIG_NUMA
        int     numa_node;          /* NUMA node this device is close to */
    #endif
        const struct dma_map_ops *dma_ops;
        u64     *dma_mask;          /* dma mask (if dma'able device) */
        u64     coherent_dma_mask;  /* Like dma_mask, but for alloc_coherent mappings as
                                       not all hardware supports 64 bit addresses for
                                       consistent allocations such descriptors. */
        unsigned long   dma_pfn_offset;

        struct device_dma_parameters *dma_parms;

        struct list_head    dma_pools;      /* dma pools (if dma'ble) */

        struct dma_coherent_mem *dma_mem;   /* internal for coherent mem override */
    #ifdef CONFIG_DMA_CMA
        struct cma *cma_area;               /* contiguous memory area for dma allocations */
    #endif
        /* arch specific additions */
        struct dev_archdata archdata;

        struct device_node      *of_node;   /* associated device tree node */
        struct fwnode_handle    *fwnode;    /* firmware device node */

        dev_t           devt;               /* dev_t, creates the sysfs "dev" */
        u32             id;                 /* device instance */

        spinlock_t          devres_lock;
        struct list_head    devres_head;

        struct klist_node   knode_class;
        struct class        *class;
        const struct attribute_group **groups;      /* optional groups */

        void    (*release)(struct device *dev);
        struct iommu_group  *iommu_group;
        struct iommu_fwspec *iommu_fwspec;

        bool            offline_disabled:1;
        bool            offline:1;
        bool            of_node_reused:1;
    };

### device_create()
    struct device *device_create(struct class *class, struct device *parent,
                     dev_t devt, void *drvdata, const char *fmt, ...)
    {
        va_list vargs;
        struct device *dev;
        va_start(vargs, fmt);
        dev = device_create_vargs(class, parent, devt, drvdata, fmt, vargs);
        va_end(vargs);
        return dev;
    }

    struct device *device_create_vargs(struct class *class, struct device *parent,
                   dev_t devt, void *drvdata, const char *fmt, va_list args)
    {
        return device_create_groups_vargs(class, parent, devt, drvdata, NULL, fmt, args);
    }

    static struct device *device_create_groups_vargs(struct class *class,
                   struct device *parent,
                   dev_t devt, void *drvdata,
                   const struct attribute_group **groups,
                   const char *fmt, va_list args)
    {
        struct device *dev = NULL;
        int retval = -ENODEV;

        if (class == NULL || IS_ERR(class))
            goto error;
        dev = kzalloc(sizeof(*dev), GFP_KERNEL);
        if (!dev) {
            retval = -ENOMEM;
            goto error;
        }

        device_initialize(dev);
        dev->devt = devt;
        dev->class = class;
        dev->parent = parent;
        dev->groups = groups;
        dev->release = device_create_release;
        dev_set_drvdata(dev, drvdata);

        retval = kobject_set_name_vargs(&dev->kobj, fmt, args);
        if (retval)
            goto error;

        retval = device_add(dev);
        if (retval)
            goto error;
        return dev;
    error:
        put_device(dev);
        return ERR_PTR(retval);
    }

    int device_add(struct device *dev)
    {
        struct device *parent;
        struct kobject *kobj;
        struct class_interface *class_intf;

        dev = get_device(dev);
        if (!dev->p) {
            device_private_init(dev);
        }
        if (dev->init_name) {
            dev_set_name(dev, "%s", dev->init_name);
            dev->init_name = NULL;
        }
        if (!dev_name(dev) && dev->bus && dev->bus->dev_name)
            dev_set_name(dev, "%s%u", dev->bus->dev_name, dev->id);
        ...
        parent = get_device(dev->parent);
        kobj = get_device_parent(dev, parent);
        if (kobj)
            dev->kobj.parent = kobj;
        /* use parent numa_node */
        if (parent && (dev_to_node(dev) == NUMA_NO_NODE))
            set_dev_node(dev, dev_to_node(parent));
        /* first, register with generic layer. */
        /* we require the name to be set before, and pass NULL */
        kobject_add(&dev->kobj, dev->kobj.parent, NULL);
        if (platform_notify)
            platform_notify(dev);

        device_create_file(dev, &dev_attr_uevent);
        device_add_class_symlinks(dev);
        device_add_attrs(dev);
        bus_add_device(dev);
        dpm_sysfs_add(dev);
        device_pm_add(dev);

        if (MAJOR(dev->devt)) {
            device_create_file(dev, &dev_attr_dev);
            device_create_sys_dev_entry(dev);
            devtmpfs_create_node(dev);
        }
        /* Notify clients of device addition.  This call must come
         * after dpm_sysfs_add() and before kobject_uevent().
         */
        if (dev->bus)
            blocking_notifier_call_chain(&dev->bus->p->bus_notifier, BUS_NOTIFY_ADD_DEVICE, dev);
        kobject_uevent(&dev->kobj, KOBJ_ADD);
        bus_probe_device(dev);
        if (parent)
            klist_add_tail(&dev->p->knode_parent, &parent->p->klist_children);

        if (dev->class) {
            mutex_lock(&dev->class->p->mutex);
            /* tie the class to the device */
            klist_add_tail(&dev->knode_class, &dev->class->p->klist_devices);
            /* notify any interfaces that the device is here */
            list_for_each_entry(class_intf, &dev->class->p->interfaces, node)
                if (class_intf->add_dev)
                    class_intf->add_dev(dev, class_intf);
            mutex_unlock(&dev->class->p->mutex);
        }
        ...
    }


### device_destroy()
    void device_destroy(struct class *class, dev_t devt)
    {
        struct device *dev;
        dev = class_find_device(class, NULL, &devt, __match_devt);
        if (dev) {
            put_device(dev);
            device_unregister(dev);
        }
    }

    void device_unregister(struct device *dev)
    {
        pr_debug("device: '%s': %s\n", dev_name(dev), __func__);
        device_del(dev);
        put_device(dev);
    }

    void device_del(struct device *dev)
    {
        struct device *parent = dev->parent;
        struct class_interface *class_intf;
        /* Notify clients of device removal.  This call must come
         * before dpm_sysfs_remove().
         */
        if (dev->bus)
            blocking_notifier_call_chain(&dev->bus->p->bus_notifier, BUS_NOTIFY_DEL_DEVICE, dev);
        device_links_purge(dev);
        dpm_sysfs_remove(dev);
        if (parent)
            klist_del(&dev->p->knode_parent);
        if (MAJOR(dev->devt)) {
            devtmpfs_delete_node(dev);
            device_remove_sys_dev_entry(dev);
            device_remove_file(dev, &dev_attr_dev);
        }
        if (dev->class) {
            device_remove_class_symlinks(dev);
            mutex_lock(&dev->class->p->mutex);
            /* notify any interfaces that the device is now gone */
            list_for_each_entry(class_intf, &dev->class->p->interfaces, node)
                if (class_intf->remove_dev)
                    class_intf->remove_dev(dev, class_intf);
            /* remove the device from the class list */
            klist_del(&dev->knode_class);
            mutex_unlock(&dev->class->p->mutex);
        }
        device_remove_file(dev, &dev_attr_uevent);
        device_remove_attrs(dev);
        bus_remove_device(dev);
        device_pm_remove(dev);
        driver_deferred_probe_del(dev);
        device_remove_properties(dev);

        /* Notify the platform of the removal, in case they
         * need to do anything...
         */
        if (platform_notify_remove)
            platform_notify_remove(dev);
        if (dev->bus)
            blocking_notifier_call_chain(&dev->bus->p->bus_notifier,
                BUS_NOTIFY_REMOVED_DEVICE, dev);
        kobject_uevent(&dev->kobj, KOBJ_REMOVE);
        ...
        kobject_del(&dev->kobj);
        ...
        put_device(parent);
    }


### example
    eg. 创建led类,向类中添加设备,mdev会帮我们创建设备节点
    cls = class_create(THIS_MODULE, "led");
    device_create(cls, NULL, MKDEV(major, 0), NULL, "led0");

    eg.
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
