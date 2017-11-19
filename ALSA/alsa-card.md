
# ALSA (Advanced Linux Sound Architecture)
    linux-4.14
    - include/sound/
    - sound/

    ALSA - card:
    include/sound/core.h
    sound/core/

### ALSA设备文件结构
    $ ll /dev/snd/
    crw-rw----+ 1 root audio 116,  2 Nov 18 21:08 controlC0
    crw-rw----+ 1 root audio 116,  7 Nov 18 21:08 hwC0D0
    crw-rw----+ 1 root audio 116,  8 Nov 18 21:08 hwC0D1
    crw-rw----+ 1 root audio 116,  9 Nov 18 21:08 hwC0D3
    crw-rw----+ 1 root audio 116,  4 Nov 18 21:08 pcmC0D0c
    crw-rw----+ 1 root audio 116,  3 Nov 18 21:49 pcmC0D0p
    crw-rw----+ 1 root audio 116,  5 Nov 18 21:08 pcmC0D3p
    crw-rw----+ 1 root audio 116,  6 Nov 18 21:08 pcmC0D7p
    crw-rw----+ 1 root audio 116,  1 Nov 18 21:08 seq
    crw-rw----+ 1 root audio 116, 33 Nov 18 21:08 timer

    controlC0   -->     用于声卡的控制，例如通道选择，混音，麦克风的控制等
    hwC0Dn      -->     声卡0中的设备n
    pcmC0Dnc    -->     声卡0中的设备n中用于录音的pcm设备 (c代表capture)
    pcmC0Dnp    -->     声卡0中的设备n中用于播放的pcm设备 (p代表playback)
    seq         -->     音序器
    timer       -->     定时器

    $ ll /proc/asound/
    dr-xr-xr-x. 6 root root 0 Nov 18 22:41 card0
    -r--r--r--. 1 root root 0 Nov 18 22:41 cards
    -r--r--r--. 1 root root 0 Nov 18 22:41 devices
    -r--r--r--. 1 root root 0 Nov 18 22:41 hwdep
    -r--r--r--. 1 root root 0 Nov 18 22:41 modules
    dr-xr-xr-x. 2 root root 0 Nov 18 22:41 oss
    lrwxrwxrwx. 1 root root 5 Nov 18 22:41 PCH -> card0
    -r--r--r--. 1 root root 0 Nov 18 22:41 pcm
    dr-xr-xr-x. 2 root root 0 Nov 18 22:41 seq
    -r--r--r--. 1 root root 0 Nov 18 22:41 timers
    -r--r--r--. 1 root root 0 Nov 18 22:41 version

### struct snd_card
    snd_card 是整个ALSA音频驱动最顶层的一个结构，整个声卡的软件逻辑结构开始于该结构，
    几乎所有与声音相关的逻辑设备都是在snd_card的管理之下，声卡驱动的第一个动作通常就是创建一个snd_card结构体。

    struct snd_card {
        int number;                 /* number of soundcard (index to snd_cards) */
        char id[16];                /* id string of this card */
        char driver[16];            /* driver name */
        char shortname[32];         /* short name of this soundcard */
        char longname[80];          /* name of this soundcard */
        char irq_descr[32];         /* Interrupt description */
        char mixername[80];         /* mixer name */
        char components[128];       /* card components delimited with space */
        struct module *module;      /* top-level module */

        void *private_data;         /* private data for soundcard */
        void (*private_free) (struct snd_card *card);
                                    /* callback for freeing of private data */
        struct list_head devices;   /* devices */

        struct device ctl_dev;                  /* control device */
        unsigned int last_numid;                /* last used numeric ID */
        struct rw_semaphore controls_rwsem;     /* controls list lock */
        rwlock_t ctl_files_rwlock;              /* ctl_files list lock */
        int controls_count;                     /* count of all controls */
        int user_ctl_count;                     /* count of all user controls */
        struct list_head controls;              /* all controls for this card */
        struct list_head ctl_files;             /* active control files */

        struct snd_info_entry *proc_root;       /* root for soundcard specific files */
        struct snd_info_entry *proc_id;         /* the card id */
        struct proc_dir_entry *proc_root_link;  /* number link to real id */

        struct list_head files_list;            /* all files associated to this card */
        struct snd_shutdown_f_ops *s_f_ops;     /* file operations in the shutdown state */
        spinlock_t files_lock;                  /* lock the files for this card */
        int shutdown;                           /* this card is going down */
        struct completion *release_completion;
        struct device *dev;                     /* device assigned to this card */
        struct device card_dev;                 /* cardX object for sysfs */
        const struct attribute_group *dev_groups[4];    /* assigned sysfs attr */
        bool registered;                        /* card_dev is registered? */

    #ifdef CONFIG_PM
        unsigned int power_state;   /* power state */
        wait_queue_head_t power_sleep;
    #endif

    #if IS_ENABLED(CONFIG_SND_MIXER_OSS)
        struct snd_mixer_oss *mixer_oss;
        int mixer_oss_change_count;
    #endif
    };

    struct list_head devices    记录该声卡下所有逻辑设备的链表
    struct list_head controls   记录该声卡下所有的控制单元的链表
    void *private_data          声卡的私有数据，可以在创建声卡时通过参数指定数据的大小
    driver                      芯片的ID字符串，alsa-lib会使用到该字符串，必须唯一性
    shortname                   更多地用于打印信息
    longname                    出现在/proc/asound/cards中

### snd_card_new()
    int snd_card_new(struct device *parent, int idx, const char *xid,
                     struct module *module, int extra_size,
                     struct snd_card **card_ret)
    {
        struct snd_card *card;
        card = kzalloc(sizeof(*card) + extra_size, GFP_KERNEL);
        if (extra_size > 0)
            card->private_data = (char *)card + sizeof(struct snd_card);
        if (xid)
            strlcpy(card->id, xid, sizeof(card->id));
        card->dev = parent;
        card->number = idx;
        card->module = module;

        INIT_LIST_HEAD(&card->devices);
        init_rwsem(&card->controls_rwsem);
        rwlock_init(&card->ctl_files_rwlock);
        INIT_LIST_HEAD(&card->controls);
        INIT_LIST_HEAD(&card->ctl_files);
        spin_lock_init(&card->files_lock);
        INIT_LIST_HEAD(&card->files_list);

        device_initialize(&card->card_dev);
        card->card_dev.parent = parent;
        ...
        kobject_set_name(&card->card_dev.kobj, "card%d", idx);

        snd_ctl_create(card);       /* SNDRV_DEV_CONTROL */
        snd_info_card_create(card); /* info节点：/proc/asound/card0 */
        *card_ret = card;
    }

    eg.
    1. struct snd_card *card;
    2. snd_card_new(parent, idx, xid, THIS_MODULE, sizeof(struct mychip), card);
       struct mychip *chip = card->private_data;

### struct snd_device
    struct snd_device {
        struct list_head list;          /* list of registered devices */
        struct snd_card *card;          /* card which holds this device */
        enum snd_device_state state;    /* state of the device */
        enum snd_device_type type;      /* device type */
        void *device_data;              /* device structure */
        struct snd_device_ops *ops;     /* operations */
    };

    enum snd_device_state {
        SNDRV_DEV_BUILD,
        SNDRV_DEV_REGISTERED,
        SNDRV_DEV_DISCONNECTED,
    };

    enum snd_device_type {
        SNDRV_DEV_LOWLEVEL,
        SNDRV_DEV_CONTROL,
        SNDRV_DEV_INFO,
        SNDRV_DEV_BUS,
        SNDRV_DEV_CODEC,
        SNDRV_DEV_PCM,
        SNDRV_DEV_COMPRESS,
        SNDRV_DEV_RAWMIDI,
        SNDRV_DEV_TIMER,
        SNDRV_DEV_SEQUENCER,
        SNDRV_DEV_HWDEP,
        SNDRV_DEV_JACK,
    };

    struct snd_device_ops {
        int (*dev_free)(struct snd_device *dev);
        int (*dev_register)(struct snd_device *dev);
        int (*dev_disconnect)(struct snd_device *dev);
    };

### snd_device_new()
    int snd_device_new(struct snd_card *card, enum snd_device_type type,
                       void *device_data, struct snd_device_ops *ops)
    {
        struct snd_device *dev;
        struct list_head *p;
        dev = kzalloc(sizeof(*dev), GFP_KERNEL);
        INIT_LIST_HEAD(&dev->list);
        dev->card = card;
        dev->type = type;
        dev->state = SNDRV_DEV_BUILD;
        dev->device_data = device_data;
        dev->ops = ops;
        ...
        list_add(&dev->list, p);
    }

    SNDRV_DEV_CONTROL       snd_ctl_create()        include/sound/control.h
    SNDRV_DEV_INFO          snd_card_proc_new()     include/sound/info.h
    SNDRV_DEV_PCM           snd_pcm_new()           include/sound/pcm.h
    SNDRV_DEV_TIMER         snd_timer_new()         include/sound/timer.h
    SNDRV_DEV_SEQUENCER     snd_seq_device_new()    include/sound/seq_device.h
    SNDRV_DEV_HWDEP         snd_hwdep_new()         include/sound/hwdep.h
    SNDRV_DEV_JACK          snd_jack_new()          include/sound/jack.h

### snd_card_register()
    int snd_card_register(struct snd_card *card)
    {
        if (!card->registered)
            device_add(&card->card_dev);
        snd_device_register_all(card));
        if (snd_cards[card->number])
            return snd_info_card_register(card);
        if (*card->id) {
            memcpy(tmpid, card->id, sizeof(card->id));
            snd_card_set_id_no_lock(card, tmpid, tmpid);
        } else {
            src = *card->shortname ? card->shortname : card->longname;
            snd_card_set_id_no_lock(card, src, retrieve_id_from_card_name(src));
        }
        snd_cards[card->number] = card;
        init_info_for_card(card);
    }
