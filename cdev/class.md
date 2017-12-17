
# class
    linux-4.14
    - include/linux/device.h
    - drivers/base/class.c


### struct class
    struct class {
        const char          *name;
        struct module       *owner;

        const struct attribute_group    **class_groups;
        const struct attribute_group    **dev_groups;
        struct kobject                  *dev_kobj;

        int (*dev_uevent)(struct device *dev, struct kobj_uevent_env *env);
        char *(*devnode)(struct device *dev, umode_t *mode);

        void (*class_release)(struct class *class);
        void (*dev_release)(struct device *dev);

        int (*suspend)(struct device *dev, pm_message_t state);
        int (*resume)(struct device *dev);
        int (*shutdown_pre)(struct device *dev);

        const struct kobj_ns_type_operations *ns_type;
        const void *(*namespace)(struct device *dev);

        const struct dev_pm_ops *pm;

        struct subsys_private *p;
    };


### class_create()
    #define class_create(owner, name)       \
    ({                      \
        static struct lock_class_key __key; \
        __class_create(owner, name, &__key);    \
    })

    struct class *__class_create(struct module *owner, const char *name,
        struct lock_class_key *key)
    {
        struct class *cls;
        int retval;

        cls = kzalloc(sizeof(*cls), GFP_KERNEL);
        if (!cls) {
            retval = -ENOMEM;
            goto error;
        }

        cls->name = name;
        cls->owner = owner;
        cls->class_release = class_create_release;

        retval = __class_register(cls, key);
        if (retval)
            goto error;

        return cls;

    error:
        kfree(cls);
        return ERR_PTR(retval);
    }

    eg. cls = class_create(THIS_MODULE, "led");


### class_destroy()
    void class_destroy(struct class *cls)
    {
        if ((cls == NULL) || (IS_ERR(cls)))
            return;
        class_unregister(cls);
    }
