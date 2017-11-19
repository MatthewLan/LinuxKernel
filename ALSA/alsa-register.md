# ALSA (Advanced Linux Sound Architecture)
    linux-4.14
    - include/sound/
    - sound/

    ALSA - register: snd_card_register()
    include/sound/core.h
    sound/core/init.c
    sound/core/device.c
    sound/core/pcm.c
    sound/core/sound.c

### struct snd_minor
    struct snd_minor {
        int type;                               /* SNDRV_DEVICE_TYPE_XXX */
        int card;                               /* card number */
        int device;                             /* device number */
        const struct file_operations *f_ops;    /* file operations */
        void *private_data;                     /* private data for f_ops->open */
        struct device *dev;                     /* device for sysfs */
        struct snd_card *card_ptr;              /* assigned card instance */
    };

    每个snd_minor结构体保存了声卡下某个逻辑设备的上下文信息;


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

### snd_device_register_all()
    int snd_device_register_all(struct snd_card *card)
    {
        list_for_each_entry(dev, &card->devices, list) {
            __snd_device_register(dev);
        }
    }

    static int __snd_device_register(struct snd_device *dev)
    {
        if (dev->state == SNDRV_DEV_BUILD) {
            if (dev->ops->dev_register) {
                /* In snd_pcm_new(): initialize struct snd_device_ops ops */
                dev->ops->dev_register(dev);
            }
            dev->state = SNDRV_DEV_REGISTERED;
        }
    }

### snd_pcm_dev_register()
    static int snd_pcm_dev_register(struct snd_device *device)
    {
        struct snd_pcm_substream *substream;
        struct snd_pcm *pcm;
        snd_pcm_add(pcm);
        for (cidx = 0; cidx < 2; cidx++) {
            switch (cidx) {
            case SNDRV_PCM_STREAM_PLAYBACK:
                devtype = SNDRV_DEVICE_TYPE_PCM_PLAYBACK;
                break;
            case SNDRV_PCM_STREAM_CAPTURE:
                devtype = SNDRV_DEVICE_TYPE_PCM_CAPTURE;
                break;
            }
            /* snd_pcm_f_ops: sound/core/pcm_native.c */
            snd_register_device(devtype, pcm->card, pcm->device,
                                &snd_pcm_f_ops[cidx], pcm, \
                                &pcm->streams[cidx].dev);
            for (substream = pcm->streams[cidx].substream; \
                substream; substream = substream->next)
                snd_pcm_timer_init(substream);
        }
        pcm_call_notify(pcm, n_register);
    }

### snd_register_device()
    int snd_register_device(int type, struct snd_card *card, int dev, \
                            const struct file_operations *f_ops, \
                            void *private_data, struct device *device)
    {
        struct snd_minor *preg;
        preg = kmalloc(sizeof *preg, GFP_KERNEL);
        preg->type = type; /* SNDRV_DEVICE_TYPE_PCM_PLAYBACK / SNDRV_DEVICE_TYPE_PCM_CAPTURE */
        preg->card = card ? card->number : -1;  /* card的编号: Cn */
        preg->device = dev;                     /* pcm实例的编号: Dn */
        preg->f_ops = f_ops;                    /* struct file_operations snd_pcm_f_ops[2] */
        preg->private_data = private_data;      /* 指向该pcm的实例 */
        preg->card_ptr = card;
        minor = snd_find_free_minor(type, card, dev);   /* 确定数组的索引值minor */
        preg->dev = device;
        device->devt = MKDEV(major, minor);     /* minor 作为pcm设备的此设备号 */
                                                /* major alsa_sound_init()中用到的是同一个 */
        device_add(device);                     /* 添加设备 */
        snd_minors[minor] = preg;       /* 把该snd_minor结构的地址放入全局数组snd_minors[minor]中 */
    }

### snd_pcm_new_stream()
    snd_pcm_new()
        _snd_pcm_new()
            snd_pcm_new_stream(pcm, SNDRV_PCM_STREAM_PLAYBACK, playback_count)
            snd_pcm_new_stream(pcm, SNDRV_PCM_STREAM_CAPTURE, capture_count)
    int snd_pcm_new_stream(struct snd_pcm *pcm, int stream, int substream_count)
    {
        struct snd_pcm_str *pstr = &pcm->streams[stream];
        struct snd_pcm_substream *substream, *prev;
        pstr->stream = stream;
        pstr->pcm = pcm;
        pstr->substream_count = substream_count;
        ...
        dev_set_name(&pstr->dev, "pcmC%iD%i%c", pcm->card->number, pcm->device, \
                     stream == SNDRV_PCM_STREAM_PLAYBACK ? 'p' : 'c');
        if (!pcm->internal) {
            snd_pcm_stream_proc_init(pstr);
        }
        for (idx = 0, prev = NULL; idx < substream_count; idx++) {
            substream = kzalloc(sizeof(*substream), GFP_KERNEL);
            substream->pcm = pcm;
            substream->pstr = pstr;
            substream->number = idx;
            substream->stream = stream;
            sprintf(substream->name, "subdevice #%i", idx);
            substream->buffer_bytes_max = UINT_MAX;
            ...
            if (!pcm->internal) {
                snd_pcm_substream_proc_init(substream);
            }
            ...
            INIT_LIST_HEAD(&substream->self_group.substreams);
            list_add_tail(&substream->link_list, &substream->self_group.substreams);
            ...
            prev = substream;
        }
    }

### struct file_operations snd_pcm_f_ops[2]
    sound/core/pcm_native.c
    const struct file_operations snd_pcm_f_ops[2] = {
        {
            .owner =             THIS_MODULE,
            .write =             snd_pcm_write,
            .write_iter =        snd_pcm_writev,
            .open =              snd_pcm_playback_open,
            .release =           snd_pcm_release,
            .llseek =            no_llseek,
            .poll =              snd_pcm_playback_poll,
            .unlocked_ioctl =    snd_pcm_ioctl,
            .compat_ioctl =      snd_pcm_ioctl_compat,
            .mmap =              snd_pcm_mmap,
            .fasync =            snd_pcm_fasync,
            .get_unmapped_area = snd_pcm_get_unmapped_area,
        },
        {
            .owner =             THIS_MODULE,
            .read =              snd_pcm_read,
            .read_iter =         snd_pcm_readv,
            .open =              snd_pcm_capture_open,
            .release =           snd_pcm_release,
            .llseek =            no_llseek,
            .poll =              snd_pcm_capture_poll,
            .unlocked_ioctl =    snd_pcm_ioctl,
            .compat_ioctl =      snd_pcm_ioctl_compat,
            .mmap =              snd_pcm_mmap,
            .fasync =            snd_pcm_fasync,
            .get_unmapped_area = snd_pcm_get_unmapped_area,
        }
    };


## Sound设备的加载
    sound/core/sound.c

### alsa_sound_init()
    static int __init alsa_sound_init(void)
    {
        snd_major = major;              /* major 与snd_register_device() 中的major是同一个 */
        snd_ecards_limit = cards_limit;
        if (register_chrdev(major, "alsa", &snd_fops)) {    /* 字符设备注册 */
            return -EIO;
        }
        if (snd_info_init() < 0) {
            unregister_chrdev(major, "alsa");
            return -ENOMEM;
        }
    }

    static const struct file_operations snd_fops =
    {
        .owner =    THIS_MODULE,
        .open =     snd_open,
        .llseek =   noop_llseek,
    };

### snd_open()
    static int snd_open(struct inode *inode, struct file *file)
    {
        unsigned int minor = iminor(inode);
        struct snd_minor *mptr = NULL;
        const struct file_operations *new_fops;
        mptr = snd_minors[minor]; /* 从snd_minors全局数组中取出当初注册pcm设备时填充的snd_minor结构 */
        if (mptr == NULL) {
            mptr = autoload_device(minor);
        }
        new_fops = fops_get(mptr->f_ops);   /* 从snd_minor结构中取出pcm设备的f_ops */
        replace_fops(file, new_fops);       /* 把file->f_op替换为pcm设备的f_ops */
        if (file->f_op->open)
            file->f_op->open(inode, file);  /* 调用pcm设备的f_ops->open() */
    }
