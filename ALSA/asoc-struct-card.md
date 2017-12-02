# ASoC (ALSA System on Chip)
    linux-4.14
    - include/sound/soc.h
    - sound/soc/


### struct snd_soc_card
    /* SoC card */
    struct snd_soc_card {
        const char *name;
        const char *long_name;
        const char *driver_name;
        char dmi_longname[80];

        struct device *dev;
        struct snd_card *snd_card;
        struct module *owner;

        struct mutex mutex;
        struct mutex dapm_mutex;

        bool instantiated;

        int (*probe)(struct snd_soc_card *card);
        int (*late_probe)(struct snd_soc_card *card);
        int (*remove)(struct snd_soc_card *card);

        /* the pre and post PM functions are used to do any PM work before and
         * after the codec and DAI's do any PM work. */
        int (*suspend_pre)(struct snd_soc_card *card);
        int (*suspend_post)(struct snd_soc_card *card);
        int (*resume_pre)(struct snd_soc_card *card);
        int (*resume_post)(struct snd_soc_card *card);

        /* callbacks */
        int (*set_bias_level)(struct snd_soc_card *,
                      struct snd_soc_dapm_context *dapm,
                      enum snd_soc_bias_level level);
        int (*set_bias_level_post)(struct snd_soc_card *,
                       struct snd_soc_dapm_context *dapm,
                       enum snd_soc_bias_level level);

        int (*add_dai_link)(struct snd_soc_card *,
                    struct snd_soc_dai_link *link);
        void (*remove_dai_link)(struct snd_soc_card *,
                    struct snd_soc_dai_link *link);

        long pmdown_time;

        /* CPU <--> Codec DAI links  */
        struct snd_soc_dai_link *dai_link;  /* predefined links only */
        int num_links;                      /* predefined links only */
        struct list_head dai_link_list;     /* all links */
        int num_dai_links;

        struct list_head rtd_list;
        int num_rtd;

        /* optional codec specific configuration */
        struct snd_soc_codec_conf *codec_conf;
        int num_configs;

        /*
         * optional auxiliary devices such as amplifiers or codecs with DAI
         * link unused
         */
        struct snd_soc_aux_dev *aux_dev;
        int num_aux_devs;
        struct list_head aux_comp_list;

        const struct snd_kcontrol_new *controls;
        int num_controls;

        /*
         * Card-specific routes and widgets.
         * Note: of_dapm_xxx for Device Tree; Otherwise for driver build-in.
         */
        const struct snd_soc_dapm_widget *dapm_widgets;
        int num_dapm_widgets;
        const struct snd_soc_dapm_route *dapm_routes;
        int num_dapm_routes;
        const struct snd_soc_dapm_widget *of_dapm_widgets;
        int num_of_dapm_widgets;
        const struct snd_soc_dapm_route *of_dapm_routes;
        int num_of_dapm_routes;
        bool fully_routed;

        struct work_struct deferred_resume_work;

        /* lists of probed devices belonging to this card */
        struct list_head component_dev_list;

        struct list_head widgets;
        struct list_head paths;
        struct list_head dapm_list;
        struct list_head dapm_dirty;

        /* attached dynamic objects */
        struct list_head dobj_list;

        /* Generic DAPM context for the card */
        struct snd_soc_dapm_context dapm;
        struct snd_soc_dapm_stats dapm_stats;
        struct snd_soc_dapm_update *update;

    #ifdef CONFIG_DEBUG_FS
        struct dentry *debugfs_card_root;
        struct dentry *debugfs_pop_time;
    #endif
        u32 pop_time;

        void *drvdata;
    };


### struct snd_soc_pcm_runtime
    /* SoC machine DAI configuration, glues a codec and cpu DAI together */
    struct snd_soc_pcm_runtime {
        struct device *dev;
        struct snd_soc_card *card;
        struct snd_soc_dai_link *dai_link;
        struct mutex pcm_mutex;
        enum snd_soc_pcm_subclass pcm_subclass;
        struct snd_pcm_ops ops;

        /* Dynamic PCM BE runtime data */
        struct snd_soc_dpcm_runtime dpcm[2];
        int fe_compr;

        long pmdown_time;

        /* runtime devices */
        struct snd_pcm *pcm;
        struct snd_compr *compr;
        struct snd_soc_codec *codec;
        struct snd_soc_platform *platform;  /* will be removed */
        struct snd_soc_dai *codec_dai;
        struct snd_soc_dai *cpu_dai;

        struct snd_soc_dai **codec_dais;
        unsigned int num_codecs;

        struct delayed_work delayed_work;
    #ifdef CONFIG_DEBUG_FS
        struct dentry *debugfs_dpcm_root;
    #endif

        unsigned int num;                   /* 0-based and monotonic increasing */
        struct list_head list;              /* rtd list of the soc card */
        struct list_head component_list;    /* list of connected components */

        /* bit field */
        unsigned int dev_registered:1;
        unsigned int pop_wait:1;
    };


### struct snd_soc_dai_link
    struct snd_soc_dai_link {
        /* config - must be set by machine driver */
        const char *name;               /* Codec name */
        const char *stream_name;        /* Stream name */
        /*
         * You MAY specify the link's CPU-side device, either by device name,
         * or by DT/OF node, but not both. If this information is omitted,
         * the CPU-side DAI is matched using .cpu_dai_name only, which hence
         * must be globally unique. These fields are currently typically used
         * only for codec to codec links, or systems using device tree.
         */
        const char *cpu_name;
        struct device_node *cpu_of_node;
        /*
         * You MAY specify the DAI name of the CPU DAI. If this information is
         * omitted, the CPU-side DAI is matched using .cpu_name/.cpu_of_node
         * only, which only works well when that device exposes a single DAI.
         */
        const char *cpu_dai_name;
        /*
         * You MUST specify the link's codec, either by device name, or by
         * DT/OF node, but not both.
         */
        const char *codec_name;
        struct device_node *codec_of_node;
        /* You MUST specify the DAI name within the codec */
        const char *codec_dai_name;

        struct snd_soc_dai_link_component *codecs;
        unsigned int num_codecs;

        /*
         * You MAY specify the link's platform/PCM/DMA driver, either by
         * device name, or by DT/OF node, but not both. Some forms of link
         * do not need a platform.
         */
        const char *platform_name;
        struct device_node *platform_of_node;
        int id; /* optional ID for machine driver link identification */

        const struct snd_soc_pcm_stream *params;
        unsigned int num_params;

        unsigned int dai_fmt;                   /* format to set on init */

        enum snd_soc_dpcm_trigger trigger[2];   /* trigger type for DPCM */

        /* codec/machine specific init - e.g. add machine controls */
        int (*init)(struct snd_soc_pcm_runtime *rtd);

        /* optional hw_params re-writing for BE and FE sync */
        int (*be_hw_params_fixup)(struct snd_soc_pcm_runtime *rtd,
                struct snd_pcm_hw_params *params);

        /* machine stream operations */
        const struct snd_soc_ops *ops;
        const struct snd_soc_compr_ops *compr_ops;

        /* Mark this pcm with non atomic ops */
        bool nonatomic;

        /* For unidirectional dai links */
        unsigned int playback_only:1;
        unsigned int capture_only:1;

        /* Keep DAI active over suspend */
        unsigned int ignore_suspend:1;

        /* Symmetry requirements */
        unsigned int symmetric_rates:1;
        unsigned int symmetric_channels:1;
        unsigned int symmetric_samplebits:1;

        /* Do not create a PCM for this DAI link (Backend link) */
        unsigned int no_pcm:1;

        /* This DAI link can route to other DAI links at runtime (Frontend)*/
        unsigned int dynamic:1;

        /* DPCM capture and Playback support */
        unsigned int dpcm_capture:1;
        unsigned int dpcm_playback:1;

        /* DPCM used FE & BE merged format */
        unsigned int dpcm_merged_format:1;

        /* pmdown_time is ignored at stop */
        unsigned int ignore_pmdown_time:1;

        struct list_head list;      /* DAI link list of the soc card */
        struct snd_soc_dobj dobj;   /* For topology */
    };

### struct snd_soc_ops
    /* SoC audio ops */
    struct snd_soc_ops {
        int (*startup)(struct snd_pcm_substream *);
        void (*shutdown)(struct snd_pcm_substream *);
        int (*hw_params)(struct snd_pcm_substream *, struct snd_pcm_hw_params *);
        int (*hw_free)(struct snd_pcm_substream *);
        int (*prepare)(struct snd_pcm_substream *);
        int (*trigger)(struct snd_pcm_substream *, int);
    };
