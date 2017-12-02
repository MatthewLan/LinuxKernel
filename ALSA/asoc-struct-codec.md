
# ASoC (ALSA System on Chip)
    linux-4.14
    - include/sound/soc.h
    - include/sound/soc-dai.h
    - sound/soc/


### struct snd_soc_codec
    /* SoC Audio Codec device */
    struct snd_soc_codec {
        struct device *dev;                         /* 指向Codec设备的指针 */
        const struct snd_soc_codec_driver *driver;  /* 指向该codec的驱动的指针 */

        struct list_head list;

        /* runtime */
        unsigned int cache_init:1;  /* codec cache has been initialized */

        /* codec IO */
        void *control_data;     /* codec control (i2c/3wire) data */
                                /* 该指针指向的结构用于对codec的控制，通常和read，write字段联合使用 */
        hw_write_t hw_write;
        void *reg_cache;

        /* component */
        struct snd_soc_component component;
    };

### struct snd_soc_codec_driver
    /* codec driver */
    struct snd_soc_codec_driver {
        /* driver ops */
        int (*probe)(struct snd_soc_codec *);       /* codec驱动的probe函数 */
        int (*remove)(struct snd_soc_codec *);
        int (*suspend)(struct snd_soc_codec *);     /* 电源管理 */
        int (*resume)(struct snd_soc_codec *);      /* 电源管理 */
        struct snd_soc_component_driver component_driver;

        /* codec wide operations */
        int (*set_sysclk)(struct snd_soc_codec *codec,
                  int clk_id, int source, unsigned int freq, int dir);
        int (*set_pll)(struct snd_soc_codec *codec, int pll_id, int source,
            unsigned int freq_in, unsigned int freq_out);
        int (*set_jack)(struct snd_soc_codec *codec,
                struct snd_soc_jack *jack,  void *data);

        /* codec IO */
        struct regmap *(*get_regmap)(struct device *);
        unsigned int (*read)(struct snd_soc_codec *, unsigned int);      /* 读取codec寄存器函数 */
        int (*write)(struct snd_soc_codec *, unsigned int, unsigned int);/* 写入codec寄存器函数 */
        unsigned int reg_cache_size;
        short reg_cache_step;
        short reg_word_size;
        const void *reg_cache_default;

        /* codec bias level */  /* 偏置电压配置函数 */
        int (*set_bias_level)(struct snd_soc_codec *, enum snd_soc_bias_level level);
        bool idle_bias_off;
        bool suspend_bias_off;

        void (*seq_notifier)(struct snd_soc_dapm_context *, enum snd_soc_dapm_type, int);

        bool ignore_pmdown_time;    /* Doesn't benefit from pmdown delay */
    };


### struct snd_soc_component
    struct snd_soc_component {
        const char *name;           /* Codec的名字*/
        int id;
        const char *name_prefix;
        struct device *dev;
        struct snd_soc_card *card;  /* 指向Machine驱动的card实例 */

        unsigned int active;

        unsigned int ignore_pmdown_time:1;  /* pmdown_time is ignored at stop */
        unsigned int registered_as_component:1;
        unsigned int suspended:1;           /* is in suspend PM state */

        struct list_head list;
        struct list_head card_aux_list;     /* for auxiliary bound components */
        struct list_head card_list;

        struct snd_soc_dai_driver *dai_drv;
        int num_dai;            /* 该Codec数字接口的个数，目前越来越多的Codec带有多个I2S或者是PCM接口 */

        const struct snd_soc_component_driver *driver;

        struct list_head dai_list;

        /* 读取Codec寄存器的函数 */
        int (*read)(struct snd_soc_component *, unsigned int, unsigned int *);
        /* 写入Codec寄存器的函数 */
        int (*write)(struct snd_soc_component *, unsigned int, unsigned int);

        struct regmap *regmap;
        int val_bytes;

        struct mutex io_mutex;

        /* attached dynamic objects */
        struct list_head dobj_list;

    #ifdef CONFIG_DEBUG_FS
        struct dentry *debugfs_root;
    #endif

        /*
        * DO NOT use any of the fields below in drivers, they are temporary and
        * are going to be removed again soon. If you use them in driver code the
        * driver will be marked as BROKEN when these fields are removed.
        */

        /* Don't use these, use snd_soc_component_get_dapm() */
        struct snd_soc_dapm_context dapm;       /* 用于DAPM控件 */

        struct snd_soc_codec *codec;            /* 指向struct snd_soc_codec实例 */

        int (*probe)(struct snd_soc_component *);
        void (*remove)(struct snd_soc_component *);
        int (*suspend)(struct snd_soc_component *);
        int (*resume)(struct snd_soc_component *);

        int (*set_sysclk)(struct snd_soc_component *component,
                  int clk_id, int source, unsigned int freq, int dir);
        int (*set_pll)(struct snd_soc_component *component, int pll_id,
                   int source, unsigned int freq_in, unsigned int freq_out);
        int (*set_jack)(struct snd_soc_component *component,
                struct snd_soc_jack *jack,  void *data);

        /* machine specific init */
        int (*init)(struct snd_soc_component *component);

    #ifdef CONFIG_DEBUG_FS
        void (*init_debugfs)(struct snd_soc_component *component);
        const char *debugfs_prefix;
    #endif
    };

### struct snd_soc_component_driver
    /* component interface */
    struct snd_soc_component_driver {
        const char *name;

        /* Default control and setup, added after probe() is run */
        const struct snd_kcontrol_new *controls;        /* 音频控件指针 */
        unsigned int num_controls;
        const struct snd_soc_dapm_widget *dapm_widgets; /* dapm部件指针 */
        unsigned int num_dapm_widgets;
        const struct snd_soc_dapm_route *dapm_routes;   /* dapm路由指针 */
        unsigned int num_dapm_routes;

        int (*probe)(struct snd_soc_component *);
        void (*remove)(struct snd_soc_component *);
        int (*suspend)(struct snd_soc_component *);
        int (*resume)(struct snd_soc_component *);

        /* component wide operations */
        int (*set_sysclk)(struct snd_soc_component *component,
                  int clk_id, int source, unsigned int freq, int dir);      /* 时钟配置函数 */
        int (*set_pll)(struct snd_soc_component *component, int pll_id,
                   int source, unsigned int freq_in, unsigned int freq_out);/* 锁相环配置函数 */
        int (*set_jack)(struct snd_soc_component *component,
                struct snd_soc_jack *jack,  void *data);

        /* DT */
        int (*of_xlate_dai_name)(struct snd_soc_component *component,
                     struct of_phandle_args *args,
                     const char **dai_name);
        int (*of_xlate_dai_id)(struct snd_soc_component *comment,
                       struct device_node *endpoint);
        void (*seq_notifier)(struct snd_soc_component *, enum snd_soc_dapm_type,
            int subseq);
        int (*stream_event)(struct snd_soc_component *, int event);

        /* probe ordering - for components with runtime dependencies */
        int probe_order;
        int remove_order;
    };


### struct snd_soc_dai_driver
    /*
     * Digital Audio Interface Driver.
     *
     * Describes the Digital Audio Interface in terms of its ALSA, DAI and AC97
     * operations and capabilities. Codec and platform drivers will register this
     * structure for every DAI they have.
     *
     * This structure covers the clocking, formating and ALSA operations for each
     * interface.
     */
    struct snd_soc_dai_driver {
        /* DAI description */
        const char *name;           /* dai驱动名字 */
        unsigned int id;
        unsigned int base;
        struct snd_soc_dobj dobj;

        /* DAI driver callbacks */
        int (*probe)(struct snd_soc_dai *dai);      /* dai驱动的probe函数 */
        int (*remove)(struct snd_soc_dai *dai);
        int (*suspend)(struct snd_soc_dai *dai);    /* 电源管理 */
        int (*resume)(struct snd_soc_dai *dai);
        /* compress dai */
        int (*compress_new)(struct snd_soc_pcm_runtime *rtd, int num);
        /* Optional Callback used at pcm creation*/
        int (*pcm_new)(struct snd_soc_pcm_runtime *rtd, struct snd_soc_dai *dai);
        /* DAI is also used for the control bus */
        bool bus_control;

        /* ops */
        const struct snd_soc_dai_ops *ops;          /* 指向本dai的snd_soc_dai_ops结构 */
        const struct snd_soc_cdai_ops *cops;

        /* DAI capabilities */
        struct snd_soc_pcm_stream capture;          /* 描述capture的能力 */
        struct snd_soc_pcm_stream playback;         /* 描述playback的能力 */
        unsigned int symmetric_rates:1;
        unsigned int symmetric_channels:1;
        unsigned int symmetric_samplebits:1;

        /* probe ordering - for components with runtime dependencies */
        int probe_order;
        int remove_order;
    };

### struct snd_soc_dai
    /*
     * Digital Audio Interface runtime data.
     *
     * Holds runtime data for a DAI.
     */
    struct snd_soc_dai {
        const char *name;                   /* dai的名字 */
        int id;
        struct device *dev;                 /* 设备指针 */

        /* driver ops */
        struct snd_soc_dai_driver *driver;  /* 指向dai驱动结构的指针 */

        /* DAI runtime info */
        unsigned int capture_active:1;      /* stream is in use */
        unsigned int playback_active:1;     /* stream is in use */
        unsigned int symmetric_rates:1;
        unsigned int symmetric_channels:1;
        unsigned int symmetric_samplebits:1;
        unsigned int probed:1;

        unsigned int active;

        struct snd_soc_dapm_widget *playback_widget;
        struct snd_soc_dapm_widget *capture_widget;

        /* DAI DMA data */
        void *playback_dma_data;            /* 用于管理playback dma */
        void *capture_dma_data;             /* 用于管理capture dma */

        /* Symmetry data - only valid if symmetry is being enforced */
        unsigned int rate;
        unsigned int channels;
        unsigned int sample_bits;

        /* parent platform/codec */
        struct snd_soc_codec *codec;        /* dai指向所绑定的codec */
        struct snd_soc_component *component;

        /* CODEC TDM slot masks and params (for fixup) */
        unsigned int tx_mask;
        unsigned int rx_mask;

        struct list_head list;
    };

### struct snd_soc_dai_ops
    struct snd_soc_dai_ops {
        /*
         * DAI clocking configuration, all optional.
         * Called by soc_card drivers, normally in their hw_params.
         */
        /* 设置dai的主时钟 */
        int (*set_sysclk)(struct snd_soc_dai *dai, int clk_id, unsigned int freq, int dir);
        /* 设置PLL参数 */
        int (*set_pll)(struct snd_soc_dai *dai, int pll_id, int source,
            unsigned int freq_in, unsigned int freq_out);
        /* 设置分频系数 */
        int (*set_clkdiv)(struct snd_soc_dai *dai, int div_id, int div);
        int (*set_bclk_ratio)(struct snd_soc_dai *dai, unsigned int ratio);

        /*
         * DAI format configuration
         * Called by soc_card drivers, normally in their hw_params.
         */
        /* 设置分频系数 */
        int (*set_fmt)(struct snd_soc_dai *dai, unsigned int fmt);
        int (*xlate_tdm_slot_mask)(unsigned int slots,
            unsigned int *tx_mask, unsigned int *rx_mask);
        /* 如果dai支持时分复用，用于设置时分复用的slot */
        int (*set_tdm_slot)(struct snd_soc_dai *dai,
            unsigned int tx_mask, unsigned int rx_mask,
            int slots, int slot_width);
        /* 声道的时分复用映射设置 */
        int (*set_channel_map)(struct snd_soc_dai *dai,
            unsigned int tx_num, unsigned int *tx_slot,
            unsigned int rx_num, unsigned int *rx_slot);
        /* 设置dai引脚的状态，当与其他dai并联使用同一引脚时需要使用该回调 */
        int (*set_tristate)(struct snd_soc_dai *dai, int tristate);

        /*
         * DAI digital mute - optional.
         * Called by soc-core to minimise any pops.
         */
        int (*digital_mute)(struct snd_soc_dai *dai, int mute);
        int (*mute_stream)(struct snd_soc_dai *dai, int mute, int stream);

        /*
         * ALSA PCM audio operations - all optional.
         * Called by soc-core during audio PCM operations.
         */
        int (*startup)(struct snd_pcm_substream *, struct snd_soc_dai *);
        void (*shutdown)(struct snd_pcm_substream *, struct snd_soc_dai *);
        int (*hw_params)(struct snd_pcm_substream *,
            struct snd_pcm_hw_params *, struct snd_soc_dai *);
        int (*hw_free)(struct snd_pcm_substream *, struct snd_soc_dai *);
        int (*prepare)(struct snd_pcm_substream *, struct snd_soc_dai *);
        /*
         * NOTE: Commands passed to the trigger function are not necessarily
         * compatible with the current state of the dai. For example this
         * sequence of commands is possible: START STOP STOP.
         * So do not unconditionally use refcounting functions in the trigger
         * function, e.g. clk_enable/disable.
         */
        int (*trigger)(struct snd_pcm_substream *, int, struct snd_soc_dai *);
        int (*bespoke_trigger)(struct snd_pcm_substream *, int, struct snd_soc_dai *);
        /*
         * For hardware based FIFO caused delay reporting.
         * Optional.
         */
        snd_pcm_sframes_t (*delay)(struct snd_pcm_substream *, struct snd_soc_dai *);
    };


    /* Digital Audio Interface clocking API.*/
    int snd_soc_dai_set_sysclk(struct snd_soc_dai *dai, int clk_id,
        unsigned int freq, int dir);

    int snd_soc_dai_set_clkdiv(struct snd_soc_dai *dai,
        int div_id, int div);

    int snd_soc_dai_set_pll(struct snd_soc_dai *dai,
        int pll_id, int source, unsigned int freq_in, unsigned int freq_out);

    int snd_soc_dai_set_bclk_ratio(struct snd_soc_dai *dai, unsigned int ratio);

    /* Digital Audio interface formatting */
    int snd_soc_dai_set_fmt(struct snd_soc_dai *dai, unsigned int fmt);

    int snd_soc_dai_set_tdm_slot(struct snd_soc_dai *dai,
        unsigned int tx_mask, unsigned int rx_mask, int slots, int slot_width);

    int snd_soc_dai_set_channel_map(struct snd_soc_dai *dai,
        unsigned int tx_num, unsigned int *tx_slot,
        unsigned int rx_num, unsigned int *rx_slot);

    int snd_soc_dai_set_tristate(struct snd_soc_dai *dai, int tristate);

    /* Digital Audio Interface mute */
    int snd_soc_dai_digital_mute(struct snd_soc_dai *dai, int mute,
                     int direction);


### snd_soc_dai_set_fmt()
    fmt: 目前只是用了它的低16位

    bit 0-3 用于设置接口的格式：
        #define SND_SOC_DAIFMT_I2S          SND_SOC_DAI_FORMAT_I2S
        #define SND_SOC_DAIFMT_RIGHT_J      SND_SOC_DAI_FORMAT_RIGHT_J
        #define SND_SOC_DAIFMT_LEFT_J       SND_SOC_DAI_FORMAT_LEFT_J
        #define SND_SOC_DAIFMT_DSP_A        SND_SOC_DAI_FORMAT_DSP_A
        #define SND_SOC_DAIFMT_DSP_B        SND_SOC_DAI_FORMAT_DSP_B
        #define SND_SOC_DAIFMT_AC97         SND_SOC_DAI_FORMAT_AC97
        #define SND_SOC_DAIFMT_PDM          SND_SOC_DAI_FORMAT_PDM

    bit 4-7 用于设置接口时钟的开关特性：
        #define SND_SOC_DAIFMT_MSB          SND_SOC_DAIFMT_LEFT_J
        #define SND_SOC_DAIFMT_LSB          SND_SOC_DAIFMT_RIGHT_J

    bit 8-11 用于设置接口时钟的相位：
        #define SND_SOC_DAIFMT_NB_NF        (0 << 8) /* normal bit clock + frame */
        #define SND_SOC_DAIFMT_NB_IF        (2 << 8) /* normal BCLK + inv FRM */
        #define SND_SOC_DAIFMT_IB_NF        (3 << 8) /* invert BCLK + nor FRM */
        #define SND_SOC_DAIFMT_IB_IF        (4 << 8) /* invert BCLK + FRM */

    bit 12-15 用于设置接口主从格式：
        #define SND_SOC_DAIFMT_CBM_CFM      (1 << 12) /* codec clk & FRM master */
        #define SND_SOC_DAIFMT_CBS_CFM      (2 << 12) /* codec clk slave & FRM master */
        #define SND_SOC_DAIFMT_CBM_CFS      (3 << 12) /* codec clk master & frame slave */
        #define SND_SOC_DAIFMT_CBS_CFS      (4 << 12) /* codec clk & FRM slave */
