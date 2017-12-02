
# idma
    linux-4.14
    - sound/soc/samsumg/idma.c
    - sound/soc/soc-devres.c
    - sound/soc/soc-core.c
    - include/sound/memalloc.h
    - include/sound/pcm.h


### Platform驱动
    主要作用是完成音频数据的管理，最终通过CPU的数字音频接口（DAI）把音频数据传送给Codec进行处理，
    最终由Codec输出驱动耳机或者是喇叭的音信信号;
    在具体实现上，ASoC把Platform驱动分为两个部分：snd_soc_platform_driver和snd_soc_dai_driver;
    其中，platform_driver负责管理音频数据，把音频数据通过dma或其他操作传送至cpu dai中;
    dai_driver则主要完成cpu一侧的dai的参数配置，同时也会通过一定的途径把必要的dma等参数
    与snd_soc_platform_driver进行交互;


### module_platform_driver(), xxx_probe()
    static int asoc_idma_platform_probe(struct platform_device *pdev)
    {
        idma_irq = platform_get_irq(pdev, 0);
        ...
        return devm_snd_soc_register_platform(&pdev->dev, &asoc_idma_platform);
    }

    static struct platform_driver asoc_idma_driver = {
        .driver = {
            .name = "samsung-idma",
        },
        .probe = asoc_idma_platform_probe,
    };
    module_platform_driver(asoc_idma_driver);

### struct snd_soc_platform_driver, struct snd_pcm_ops
    static const struct snd_soc_platform_driver asoc_idma_platform = {
        .ops      = &idma_ops,
        .pcm_new  = idma_new,       /* pcm_new回调会在声卡的建立阶段被调用 */
        .pcm_free = idma_free,
    };

    static const struct snd_pcm_ops idma_ops = {
        .open       = idma_open,
        .close      = idma_close,
        .ioctl      = snd_pcm_lib_ioctl,
        .trigger    = idma_trigger,
        .pointer    = idma_pointer,
        .mmap       = idma_mmap,
        .hw_params  = idma_hw_params,   /* 在声卡的hw_params阶段被调用 */
        .hw_free    = idma_hw_free,
        .prepare    = idma_prepare,
    };

### devm_snd_soc_register_platform()
    int devm_snd_soc_register_platform(struct device *dev,
            const struct snd_soc_platform_driver *platform_drv)
    {
        struct device **ptr;
        ptr = devres_alloc(devm_platform_release, sizeof(*ptr), GFP_KERNEL);
        ret = snd_soc_register_platform(dev, platform_drv);
        if (ret == 0) {
            *ptr = dev;
            devres_add(dev, ptr);
        } else {
            devres_free(ptr);
        }
    }

### snd_soc_register_platform()
    int snd_soc_register_platform(struct device *dev,
            const struct snd_soc_platform_driver *platform_drv)
    {
        struct snd_soc_platform *platform;
        platform = kzalloc(sizeof(struct snd_soc_platform), GFP_KERNEL);
        snd_soc_add_platform(dev, platform, platform_drv);
    }

### snd_soc_add_platform()
    int snd_soc_add_platform(struct device *dev, struct snd_soc_platform *platform,
            const struct snd_soc_platform_driver *platform_drv)
    {
        snd_soc_component_initialize(&platform->component,
                                     &platform_drv->component_driver, dev);
        platform->dev = dev;
        platform->driver = platform_drv;

        if (platform_drv->probe)
            platform->component.probe = snd_soc_platform_drv_probe;
        if (platform_drv->remove)
            platform->component.remove = snd_soc_platform_drv_remove;
        ...
        snd_soc_component_add_unlocked(&platform->component);
        list_add(&platform->list, &platform_list);
        ...
    }

### snd_soc_component_initialize()
    在 asoc-codec-wm8994.md 中，共通部分;


### struct snd_dma_buffer
    struct snd_dma_buffer {
        struct snd_dma_device dev;  /* device type */
        unsigned char *area;        /* virtual pointer */
        dma_addr_t addr;            /* physical address */
        size_t bytes;               /* buffer size in bytes */
        void *private_data;         /* private for allocator; don't touch */
    };
    在ASoC架构中，dma buffer的信息保存在snd_pcm_substream结构的snd_dma_buffer *buf字段中;


### idma_new()
    static int idma_new(struct snd_soc_pcm_runtime *rtd)
    {
        ...
        if (pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream) {
            preallocate_idma_buffer(pcm, SNDRV_PCM_STREAM_PLAYBACK);
        }
    }

### preallocate_idma_buffer()
    static int preallocate_idma_buffer(struct snd_pcm *pcm, int stream)
    {
        struct snd_pcm_substream *substream = pcm->streams[stream].substream;
        struct snd_dma_buffer *buf = &substream->dma_buffer;

        buf->dev.dev = pcm->card->dev;
        buf->private_data = NULL;

        /* Assign PCM buffer pointers */
        buf->dev.type = SNDRV_DMA_TYPE_CONTINUOUS;
        buf->addr = idma.lp_tx_addr;
        buf->bytes = idma_hardware.buffer_bytes_max;
        buf->area = (unsigned char * __force)ioremap(buf->addr, buf->bytes);
    }


### idma_hw_params()
    static int idma_hw_params(struct snd_pcm_substream *substream,
                    struct snd_pcm_hw_params *params)
    {
        struct snd_pcm_runtime *runtime = substream->runtime;
        ...
        snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
        ...
    }

### snd_pcm_set_runtime_buffer()
    static inline void snd_pcm_set_runtime_buffer(struct snd_pcm_substream *substream,
                              struct snd_dma_buffer *bufp)
    {
        struct snd_pcm_runtime *runtime = substream->runtime;
        if (bufp) {
            runtime->dma_buffer_p = bufp;
            runtime->dma_area = bufp->area;
            runtime->dma_addr = bufp->addr;
            runtime->dma_bytes = bufp->bytes;
        } else {
            runtime->dma_buffer_p = NULL;
            runtime->dma_area = NULL;
            runtime->dma_addr = 0;
            runtime->dma_bytes = 0;
        }
    }
    把substream->dma_buffer的数值拷贝到substream->runtime的相关字段中（.dma_area, .dma_addr,
    .dma_bytes），这样以后就可以通过substream->runtime获得这些地址和大小信息了;

    dma buffer获得后，即是获得了dma操作的源地址，其目的地址是在dai中，也就是的snd_soc_dai结构
    的playback_dma_data和capture_dma_data字段中，而这两个字段的值也是在hw_params阶段，
    由snd_soc_dai_driver结构的ops->hw_params回调，利用api：snd_soc_dai_set_dma_data进行设置的;
    紧随其后，snd_soc_platform_driver结构的ops->hw_params回调利用api：snd_soc_dai_get_dma_data获得
    这些dai的dma信息，其中就包括了dma的目的地址信息;
    这些dma信息通常还会被保存在substream->runtime->private_data中，
    以便在substream的整个生命周期中可以随时获得这些信息，从而完成对dma的配置和操作;


### dma buffer管理
    环形缓冲区: 缓冲区通常都是一段连续的地址;
    通常都会把这个大小为Count的缓冲区虚拟成一个大小为n*Count的逻辑缓冲区，相当于理想状态下的圆形绕了n圈之后，
    然后把这段总的距离拉平为一段直线，每一圈对应直线中的一段，因为n比较大，所以大多数情况下不会出现读写指针的
    换位的情况（如果不对buffer进行扩展，指针到达末端后，回到起始端时，两个指针的前后相对位置会发生互换）;

    snd_pcm_runtime结构中，使用了四个相关的字段来完成这个逻辑缓冲区的管理：
    snd_pcm_runtime.hw_ptr_base  环形缓冲区每一圈的基地址，当读写指针越过一圈后，它按buffer size进行移动；
    snd_pcm_runtime.status->hw_ptr  硬件逻辑位置，播放时相当于读指针，录音时相当于写指针；
    snd_pcm_runtime.control->appl_ptr  应用逻辑位置，播放时相当于写指针，录音时相当于读指针；
    snd_pcm_runtime.boundary  扩展后的逻辑缓冲区大小，通常是(2^n)*size；

    example: 获得播放缓冲区的空闲空间
    /**
     * snd_pcm_playback_avail - Get the available (writable) space for playback
     * @runtime: PCM runtime instance
     *
     * Result is between 0 ... (boundary - 1)
     */
    static inline snd_pcm_uframes_t snd_pcm_playback_avail(struct snd_pcm_runtime *runtime)
    {
        snd_pcm_sframes_t avail = runtime->status->hw_ptr + runtime->buffer_size \
                                    - runtime->control->appl_ptr;
        if (avail < 0)
            avail += runtime->boundary;
        else if ((snd_pcm_uframes_t) avail >= runtime->boundary)
            avail -= runtime->boundary;
        return avail;
    }
