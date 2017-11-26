
# ALSA (Advanced Linux Sound Architecture)
    linux-4.14
    - include/sound/
    - sound/

    ALSA - pcm:
    include/sound/pcm.h             --> pcm core
    include/sound/pcm_params.h      --> hw_param functions
    sound/core/

### pcm实例
    pcm实例数量,限制于linux设备号所占用的位大小;
    一个pcm实例由一个playback stream和一个capture stream组成，
    这两个stream又分别有一个或多个substreams组成;

### struct snd_pcm
    struct snd_pcm {
        struct snd_card *card;
        struct list_head list;
        int device;             /* device number */
        unsigned int info_flags;
        unsigned short dev_class;
        unsigned short dev_subclass;
        char id[64];
        char name[80];
        struct snd_pcm_str streams[2];
        struct mutex open_mutex;
        wait_queue_head_t open_wait;
        void *private_data;
        void (*private_free)(struct snd_pcm *pcm);
        bool internal;          /* pcm is for internal use only */
        bool nonatomic;         /* whole PCM operations are in non-atomic context */
    #if IS_ENABLED(CONFIG_SND_PCM_OSS)
        struct snd_pcm_oss oss;
    #endif
    };

    snd_pcm 是挂在snd_card下面的一个snd_device, 其中
    streams[2]: 分别代表playback stream和capture stream

### struct snd_pcm_str
    struct snd_pcm_str {
        int stream;                         /* stream (direction) */
        struct snd_pcm *pcm;
        /* -- substreams -- */
        unsigned int substream_count;
        unsigned int substream_opened;
        struct snd_pcm_substream *substream;
    #if IS_ENABLED(CONFIG_SND_PCM_OSS)
        /* -- OSS things -- */
        struct snd_pcm_oss_stream oss;
    #endif
    #ifdef CONFIG_SND_VERBOSE_PROCFS
        struct snd_info_entry *proc_root;
        struct snd_info_entry *proc_info_entry;
    #ifdef CONFIG_SND_PCM_XRUN_DEBUG
        unsigned int xrun_debug;            /* 0 = disabled, 1 = verbose, 2 = stacktrace */
        struct snd_info_entry *proc_xrun_debug_entry;
    #endif
    #endif
        struct snd_kcontrol *chmap_kctl;    /* channel-mapping controls */
        struct device dev;
    };

    substream: 指向snd_pcm_substream结构，是pcm中间层的核心，绝大部分任务都是在substream中处理;

### struct snd_pcm_substream
    struct snd_pcm_substream {
        struct snd_pcm *pcm;
        struct snd_pcm_str *pstr;
        void *private_data;                         /* copied from pcm->private_data */
        int number;
        char name[32];                              /* substream name */
        int stream;                                 /* stream (direction) */
        struct pm_qos_request latency_pm_qos_req;   /* pm_qos request */
        size_t buffer_bytes_max;                    /* limit ring buffer size */
        struct snd_dma_buffer dma_buffer;
        size_t dma_max;
        /* -- hardware operations -- */
        const struct snd_pcm_ops *ops;
        /* -- runtime information -- */
        struct snd_pcm_runtime *runtime;
            /* -- timer section -- */
        struct snd_timer *timer;                    /* timer */
        unsigned timer_running: 1;                  /* time is running */
        /* -- next substream -- */
        struct snd_pcm_substream *next;
        /* -- linked substreams -- */
        struct list_head link_list;                 /* linked list member */
        struct snd_pcm_group self_group;            /* fake group for non linked substream
                                                       (with substream lock inside) */
        struct snd_pcm_group *group;                /* pointer to current group */
        /* -- assigned files -- */
        void *file;
        int ref_count;
        atomic_t mmap_count;
        unsigned int f_flags;
        void (*pcm_release)(struct snd_pcm_substream *);
        struct pid *pid;
    #if IS_ENABLED(CONFIG_SND_PCM_OSS)
        /* -- OSS things -- */
        struct snd_pcm_oss_substream oss;
    #endif
    #ifdef CONFIG_SND_VERBOSE_PROCFS
        struct snd_info_entry *proc_root;
        struct snd_info_entry *proc_info_entry;
        struct snd_info_entry *proc_hw_params_entry;
        struct snd_info_entry *proc_sw_params_entry;
        struct snd_info_entry *proc_status_entry;
        struct snd_info_entry *proc_prealloc_entry;
        struct snd_info_entry *proc_prealloc_max_entry;
    #ifdef CONFIG_SND_PCM_XRUN_DEBUG
        struct snd_info_entry *proc_xrun_injection_entry;
    #endif
    #endif /* CONFIG_SND_VERBOSE_PROCFS */
        /* misc flags */
        unsigned int hw_opened: 1;
    };

    ops    (snd_pcm_ops)    : 许多user空间的应用程序通过alsa-lib对驱动程序的请求都是由该结构中的函数处理;
    runtime(snd_pcm_runtime): 记录这substream的一些重要的软件和硬件运行环境和参数;

### struct snd_pcm_ops
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

### struct snd_pcm_runtime
    struct snd_pcm_runtime {
        /* -- Status -- */
        struct snd_pcm_substream *trigger_master;
        struct timespec trigger_tstamp;         /* trigger timestamp */
        bool trigger_tstamp_latched;            /* trigger timestamp latched
                                                   in low-level driver/hardware */
        int overrange;
        snd_pcm_uframes_t avail_max;
        snd_pcm_uframes_t hw_ptr_base;          /* Position at buffer restart */
        snd_pcm_uframes_t hw_ptr_interrupt;     /* Position at interrupt time */
        unsigned long hw_ptr_jiffies;           /* Time when hw_ptr is updated */
        unsigned long hw_ptr_buffer_jiffies;    /* buffer time in jiffies */
        snd_pcm_sframes_t delay;                /* extra delay; typically FIFO size */
        u64 hw_ptr_wrap;                    /* offset for hw_ptr due to boundary wrap-around */

        /* -- HW params -- */
        snd_pcm_access_t access;                /* access mode */
        snd_pcm_format_t format;                /* SNDRV_PCM_FORMAT_* */
        snd_pcm_subformat_t subformat;          /* subformat */
        unsigned int rate;                      /* rate in Hz */
        unsigned int channels;                  /* channels */
        snd_pcm_uframes_t period_size;          /* period size */
        unsigned int periods;                   /* periods */
        snd_pcm_uframes_t buffer_size;          /* buffer size */
        snd_pcm_uframes_t min_align;            /* Min alignment for the format */
        size_t byte_align;
        unsigned int frame_bits;
        unsigned int sample_bits;
        unsigned int info;
        unsigned int rate_num;
        unsigned int rate_den;
        unsigned int no_period_wakeup: 1;

        /* -- SW params -- */
        int tstamp_mode;                        /* mmap timestamp is updated */
        unsigned int period_step;
        snd_pcm_uframes_t start_threshold;
        snd_pcm_uframes_t stop_threshold;
        snd_pcm_uframes_t silence_threshold;    /* Silence filling happens
                                                   when noise is nearest than this */
        snd_pcm_uframes_t silence_size;         /* Silence filling size */
        snd_pcm_uframes_t boundary;             /* pointers wrap point */

        snd_pcm_uframes_t silence_start;        /* starting pointer to silence area */
        snd_pcm_uframes_t silence_filled;       /* size filled with silence */

        union snd_pcm_sync_id sync;             /* hardware synchronization ID */

        /* -- mmap -- */
        struct snd_pcm_mmap_status *status;
        struct snd_pcm_mmap_control *control;

        /* -- locking / scheduling -- */
        snd_pcm_uframes_t twake;                /* do transfer (!poll) wakeup if non-zero */
        wait_queue_head_t sleep;                /* poll sleep */
        wait_queue_head_t tsleep;               /* transfer sleep */
        struct fasync_struct *fasync;

        /* -- private section -- */
        void *private_data;
        void (*private_free)(struct snd_pcm_runtime *runtime);

        /* -- hardware description -- */
        struct snd_pcm_hardware hw;
        struct snd_pcm_hw_constraints hw_constraints;

        /* -- timer -- */
        unsigned int timer_resolution;          /* timer resolution */
        int tstamp_type;                        /* timestamp type */

        /* -- DMA -- */
        unsigned char *dma_area;                /* DMA area */
        dma_addr_t dma_addr;        /* physical bus address (not accessible from main CPU) */
        size_t dma_bytes;                       /* size of DMA area */

        struct snd_dma_buffer *dma_buffer_p;    /* allocated buffer */

        /* -- audio timestamp config -- */
        struct snd_pcm_audio_tstamp_config audio_tstamp_config;
        struct snd_pcm_audio_tstamp_report audio_tstamp_report;
        struct timespec driver_tstamp;

    #if IS_ENABLED(CONFIG_SND_PCM_OSS)
        /* -- OSS things -- */
        struct snd_pcm_oss_runtime oss;
    #endif
    };

### struct snd_pcm_hardware
    struct snd_pcm_hardware {
        unsigned int info;          /* SNDRV_PCM_INFO_* */
        u64 formats;                /* SNDRV_PCM_FMTBIT_* */
        unsigned int rates;         /* SNDRV_PCM_RATE_* */
        unsigned int rate_min;      /* min rate */
        unsigned int rate_max;      /* max rate */
        unsigned int channels_min;  /* min channels */
        unsigned int channels_max;  /* max channels */
        size_t buffer_bytes_max;    /* max buffer size */
        size_t period_bytes_min;    /* min period size */
        size_t period_bytes_max;    /* max period size */
        unsigned int periods_min;   /* min # of periods */
        unsigned int periods_max;   /* max # of periods */
        size_t fifo_size;           /* fifo size in bytes */
    };

### struct snd_pcm_file
    struct snd_pcm_file {
        struct snd_pcm_substream *substream;
        int no_compat_mmap;
        unsigned int user_pversion; /* supported protocol version */
    };


### snd_pcm_new()
    int snd_pcm_new(struct snd_card *card, const char *id, int device, \
                   int playback_count, int capture_count, struct snd_pcm **rpcm)
    {
        return _snd_pcm_new(card, id, device, playback_count, capture_count, false, rpcm);
    }

    static int _snd_pcm_new(struct snd_card *card, const char *id, int device, \
                            int playback_count, int capture_count, bool internal, \
                            struct snd_pcm **rpcm)
    {
        struct snd_pcm *pcm;
        static struct snd_device_ops ops = {
            .dev_free = snd_pcm_dev_free,
            .dev_register = snd_pcm_dev_register,
            .dev_disconnect = snd_pcm_dev_disconnect,
        };
        pcm = kzalloc(sizeof(*pcm), GFP_KERNEL);
        pcm->card = card;
        pcm->device = device;
        pcm->internal = internal;
        init_waitqueue_head(&pcm->open_wait);
        if (id)
            strlcpy(pcm->id, id, sizeof(pcm->id));
        snd_pcm_new_stream(pcm, SNDRV_PCM_STREAM_PLAYBACK, playback_count);
        snd_pcm_new_stream(pcm, SNDRV_PCM_STREAM_CAPTURE, capture_count);
        snd_device_new(card, SNDRV_DEV_PCM, pcm, &ops);
    }

    device        : 表示目前创建的是该声卡下的第几个pcm，第一个pcm设备从0开始
    playback_count: 表示该pcm将会有几个playback substream
    capture_count : 表示该pcm将会有几个capture substream

### snd_pcm_set_ops
    void snd_pcm_set_ops(struct snd_pcm *pcm, int direction, const struct snd_pcm_ops *ops)
    {
        struct snd_pcm_str *stream = &pcm->streams[direction];
        struct snd_pcm_substream *substream;
        for (substream = stream->substream; substream != NULL; substream = substream->next)
            substream->ops = ops;
    }

### snd_pcm_dev_register()
    在 alsa-register.md 中
