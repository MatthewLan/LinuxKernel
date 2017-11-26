
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
