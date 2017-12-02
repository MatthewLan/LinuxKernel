
# ASoC (ALSA System on Chip)
    linux-4.14
    - include/sound/soc.h
    - sound/soc/


### struct snd_soc_platform
    struct snd_soc_platform {
        struct device *dev;
        const struct snd_soc_platform_driver *driver;

        struct list_head list;

        struct snd_soc_component component;
    };

### struct snd_soc_platform_driver
    /* SoC platform interface */
    struct snd_soc_platform_driver {

        int (*probe)(struct snd_soc_platform *);
        int (*remove)(struct snd_soc_platform *);
        struct snd_soc_component_driver component_driver;

        /* pcm creation and destruction */
        int (*pcm_new)(struct snd_soc_pcm_runtime *);
        void (*pcm_free)(struct snd_pcm *);

        /* platform stream pcm ops */
        const struct snd_pcm_ops *ops;

        /* platform stream compress ops */
        const struct snd_compr_ops *compr_ops;
    };


### struct snd_soc_component
### struct snd_soc_component_driver
    在 asoc-codec-struct 中，共通部分;

### struct snd_pcm_ops
    在 alsa-pcm.md 中，共通部分;
    struct snd_pcm_ops {
        int (*open)(struct snd_pcm_substream *substream);
        int (*close)(struct snd_pcm_substream *substream);
        int (*ioctl)(struct snd_pcm_substream * substream,
                     unsigned int cmd, void *arg);
        int (*hw_params)(struct snd_pcm_substream *substream,
                         struct snd_pcm_hw_params *params);
        int (*hw_free)(struct snd_pcm_substream *substream);
        int (*prepare)(struct snd_pcm_substream *substream);
        int (*trigger)(struct snd_pcm_substream *substream, int cmd);
        snd_pcm_uframes_t (*pointer)(struct snd_pcm_substream *substream);
        int (*get_time_info)(struct snd_pcm_substream *substream,
                             struct timespec *system_ts, struct timespec *audio_ts,
                             struct snd_pcm_audio_tstamp_config *audio_tstamp_config,
                             struct snd_pcm_audio_tstamp_report *audio_tstamp_report);
        int (*fill_silence)(struct snd_pcm_substream *substream, int channel,
                            unsigned long pos, unsigned long bytes);
        int (*copy_user)(struct snd_pcm_substream *substream, int channel,
                         unsigned long pos, void __user *buf, unsigned long bytes);
        int (*copy_kernel)(struct snd_pcm_substream *substream, int channel,
                           unsigned long pos, void *buf, unsigned long bytes);
        struct page *(*page)(struct snd_pcm_substream *substream, unsigned long offset);
        int (*mmap)(struct snd_pcm_substream *substream, struct vm_area_struct *vma);
        int (*ack)(struct snd_pcm_substream *substream);
    };

    open():
        当应用程序打开一个pcm设备时，该函数会被调用;
        通常，该函数会使用snd_soc_set_runtime_hwparams()
        设置substream中的snd_pcm_runtime结构里面的hw_params相关字段，
        然后为snd_pcm_runtime的private_data字段申请一个私有结构，用于保存该平台的dma参数;

    hw_params()
        驱动的hw_params阶段，该函数会被调用;
        通常，该函数会通过snd_soc_dai_get_dma_data函数获得对应的dai的dma参数，
        获得的参数一般都会保存在snd_pcm_runtime结构的private_data字段;
        然后通过snd_pcm_set_runtime_buffer函数设置snd_pcm_runtime结构中的dma buffer的地址和大小等参数;
        要注意的是，该回调可能会被多次调用，具体实现时要小心处理多次申请资源的问题;

    prepare()
        正式开始数据传送之前会调用该函数，该函数通常会完成dma操作的必要准备工作;

    trigger()
        数据传送的开始，暂停，恢复和停止时，该函数会被调用;

    pointer()
        该函数返回传送数据的当前位置;
