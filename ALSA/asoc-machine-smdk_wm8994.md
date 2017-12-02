
# smdk_wm8994
    linux-4.14
    - sound/soc/samsumg/smdk_wm8994.c
    - sound/soc/soc-devres.c
    - sound/soc/soc-core.c
    - sound/soc/soc-pcm.c


### module_platform_driver(), xxx_probe()
    static int smdk_audio_probe(struct platform_device *pdev)
    {
        ...
        struct snd_soc_card *card = &smdk;
        ...
        card->dev = &pdev->dev;
        ...
        devm_snd_soc_register_card(&pdev->dev, card);
        ...
    }

    static struct platform_driver smdk_audio_driver = {
        ...
        .probe = smdk_audio_probe,
    };
    module_platform_driver(smdk_audio_driver);

### struct snd_soc_ops, struct snd_soc_dai_link, struct snd_soc_card
    static struct snd_soc_ops smdk_ops = {
        .hw_params = smdk_hw_params,
    };

    static struct snd_soc_dai_link smdk_dai[] = {
        { /* Primary DAI i/f */
            .name = "WM8994 AIF1",
            .stream_name = "Pri_Dai",
            .cpu_dai_name = "samsung-i2s.0",
            .codec_dai_name = "wm8994-aif1",
            .platform_name = "samsung-i2s.0",
            .codec_name = "wm8994-codec",
            .init = smdk_wm8994_init_paiftx,
            .dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
                SND_SOC_DAIFMT_CBM_CFM,
            .ops = &smdk_ops,
        }, { /* Sec_Fifo Playback i/f */
            .name = "Sec_FIFO TX",
            .stream_name = "Sec_Dai",
            .cpu_dai_name = "samsung-i2s-sec",
            .codec_dai_name = "wm8994-aif1",
            .platform_name = "samsung-i2s-sec",
            .codec_name = "wm8994-codec",
            .dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
                SND_SOC_DAIFMT_CBM_CFM,
            .ops = &smdk_ops,
        },
    };

    static struct snd_soc_card smdk = {
        .name = "SMDK-I2S",
        .owner = THIS_MODULE,
        .dai_link = smdk_dai,
        .num_links = ARRAY_SIZE(smdk_dai),
    };

    1. smdk_dai 中，指定了Platform、Codec、codec_dai、cpu_dai的名字;
    2. 稍后Machine驱动将会利用这些名字去匹配已经在系统中注册的platform，codec，dai;
    3. 这些注册的部件都是在另外相应的Platform驱动和Codec驱动的代码文件中定义的;
    4. smdk_ops, 连接Platform和Codec的dai_link对应的ops实现;


### devm_snd_soc_register_card()
    int devm_snd_soc_register_card(struct device *dev, struct snd_soc_card *card)
    {
        ...
        snd_soc_register_card(card);
        ...
    }

    int snd_soc_register_card(struct snd_soc_card *card)
    {
        struct snd_soc_pcm_runtime *rtd;

        for (i = 0; i < card->num_links; i++) {
            struct snd_soc_dai_link *link = &card->dai_link[i];
            soc_init_dai_link(card, link);
            ...
        }
        dev_set_drvdata(card->dev, card);
        ...
        snd_soc_instantiate_card(card);
        ...
    }

### soc_init_dai_link()
    static int soc_init_dai_link(struct snd_soc_card *card, struct snd_soc_dai_link *link)
    {
        snd_soc_init_multicodec(card, link);
        ...
    }

    static int snd_soc_init_multicodec(struct snd_soc_card *card,
                                       struct snd_soc_dai_link *dai_link)
    {
        if (dai_link->codec_name || dai_link->codec_of_node || dai_link->codec_dai_name) {
            dai_link->num_codecs = 1;
            dai_link->codecs = devm_kzalloc(card->dev,
                                            sizeof(struct snd_soc_dai_link_component),
                                            GFP_KERNEL);
            ...
            dai_link->codecs[0].name = dai_link->codec_name;
            dai_link->codecs[0].of_node = dai_link->codec_of_node;
            dai_link->codecs[0].dai_name = dai_link->codec_dai_name;
        }
    }


### snd_soc_instantiate_card()
static int snd_soc_instantiate_card(struct snd_soc_card *card)
{
    struct snd_soc_codec *codec;
    struct snd_soc_pcm_runtime *rtd;
    struct snd_soc_dai_link *dai_link;

    /* bind DAIs */
    for (i = 0; i < card->num_links; i++) {
        soc_bind_dai_link(card, &card->dai_link[i]);
    }
    ...
    /* add predefined DAI links to the list */
    for (i = 0; i < card->num_links; i++)
        snd_soc_add_dai_link(card, card->dai_link+i);
    /* initialize the register cache for each available codec */
    list_for_each_entry(codec, &codec_list, list) {
        if (codec->cache_init)
            continue;
        snd_soc_init_codec_cache(codec);
    }
    /* initialize the register cache for each available codec */
    list_for_each_entry(codec, &codec_list, list) {
        if (codec->cache_init)
            continue;
        snd_soc_init_codec_cache(codec);
    }
    snd_card_new(card->dev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1,
                 card->owner, 0, &card->snd_card);
    ...
    /* initialise the sound card only once */
    if (card->probe) {
        card->probe(card);
    }
    /* probe all components used by DAI links on this card */
    for (order = SND_SOC_COMP_ORDER_FIRST; order <= SND_SOC_COMP_ORDER_LAST; order++) {
        list_for_each_entry(rtd, &card->rtd_list, list) {
             soc_probe_link_components(card, rtd, order);
        }
    }
    ...
    /* Find new DAI links added during probing components and bind them.
     * Components with topology may bring new DAIs and DAI links. */
    list_for_each_entry(dai_link, &card->dai_link_list, list) {
        if (soc_is_dai_link_bound(card, dai_link))
            continue;
        soc_init_dai_link(card, dai_link);
        soc_bind_dai_link(card, dai_link);
    }
    /* probe all DAI links on this card */
    for (order = SND_SOC_COMP_ORDER_FIRST; order <= SND_SOC_COMP_ORDER_LAST; order++) {
        list_for_each_entry(rtd, &card->rtd_list, list) {
            soc_probe_link_dais(card, rtd, order);
        }
    }
    ...
    if (card->controls)
        snd_soc_add_card_controls(card, card->controls, card->num_controls);
    ...
    snprintf(card->snd_card->shortname, sizeof(card->snd_card->shortname),
            "%s", card->name);
    snprintf(card->snd_card->longname, sizeof(card->snd_card->longname),
            "%s", card->long_name ? card->long_name : card->name);
    snprintf(card->snd_card->driver, sizeof(card->snd_card->driver),
            "%s", card->driver_name ? card->driver_name : card->name);
    ...
    if (card->late_probe) {
        card->late_probe(card);
    }
    ...
    snd_card_register(card->snd_card);
    ...
}

### soc_probe_link_dais()
    static int soc_probe_link_dais(struct snd_soc_card *card,
                                   struct snd_soc_pcm_runtime *rtd, int order)
    {
        struct snd_soc_dai_link *dai_link = rtd->dai_link;
        struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
        ...
        soc_probe_dai(cpu_dai, order);
        /* probe the CODEC DAI */
        for (i = 0; i < rtd->num_codecs; i++) {
            soc_probe_dai(rtd->codec_dais[i], order);
        }
        /* do machine specific initialization */
        if (dai_link->init) {
            dai_link->init(rtd);
        }
        ...
        if (cpu_dai->driver->compress_new) {
            ...
        } else {
            if (!dai_link->params) {
                /* create the pcm */
                soc_new_pcm(rtd, rtd->num);
                soc_link_dai_pcm_new(&cpu_dai, 1, rtd);
                soc_link_dai_pcm_new(rtd->codec_dais, rtd->num_codecs, rtd);
            } else {
                ...
            }
        }
    }

### soc_new_pcm()
int soc_new_pcm(struct snd_soc_pcm_runtime *rtd, int num)
{
    struct snd_soc_platform *platform = rtd->platform;
    struct snd_soc_dai *codec_dai;
    struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
    struct snd_pcm *pcm;
    char new_name[64];
    int playback = 0, capture = 0;

    if (rtd->dai_link->dynamic || rtd->dai_link->no_pcm) {
        playback = rtd->dai_link->dpcm_playback;
        capture = rtd->dai_link->dpcm_capture;
    } else {
        for (i = 0; i < rtd->num_codecs; i++) {
            codec_dai = rtd->codec_dais[i];
            if (codec_dai->driver->playback.channels_min)
                playback = 1;
            if (codec_dai->driver->capture.channels_min)
                capture = 1;
        }
        capture = capture && cpu_dai->driver->capture.channels_min;
        playback = playback && cpu_dai->driver->playback.channels_min;
    }
    if (rtd->dai_link->playback_only) {
        playback = 1;
        capture = 0;
    }
    if (rtd->dai_link->capture_only) {
        playback = 0;
        capture = 1;
    }

    /* create the PCM */
    if (rtd->dai_link->no_pcm) {
        snprintf(new_name, sizeof(new_name), "(%s)", rtd->dai_link->stream_name);
        snd_pcm_new_internal(rtd->card->snd_card, new_name, num, playback, capture, &pcm);
    } else {
        if (rtd->dai_link->dynamic)
            snprintf(new_name, sizeof(new_name), "%s (*)", rtd->dai_link->stream_name);
        else
            snprintf(new_name, sizeof(new_name), "%s %s-%d", rtd->dai_link->stream_name,
                     (rtd->num_codecs > 1) ? "multicodec" : rtd->codec_dai->name, num);
        snd_pcm_new(rtd->card->snd_card, new_name, num, playback, capture, &pcm);
    }
    ...
    rtd->pcm = pcm;
    pcm->private_data = rtd;        /* pcm的private_data字段设置为该runtime变量rtd */
    if (rtd->dai_link->no_pcm) {
        if (playback)
            pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream->private_data = rtd;
        if (capture)
            pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream->private_data = rtd;
        goto out;
    }

    /* ASoC PCM operations */
    if (rtd->dai_link->dynamic) {
        rtd->ops.open       = dpcm_fe_dai_open;
        rtd->ops.hw_params  = dpcm_fe_dai_hw_params;
        rtd->ops.prepare    = dpcm_fe_dai_prepare;
        rtd->ops.trigger    = dpcm_fe_dai_trigger;
        rtd->ops.hw_free    = dpcm_fe_dai_hw_free;
        rtd->ops.close      = dpcm_fe_dai_close;
        rtd->ops.pointer    = soc_pcm_pointer;
        rtd->ops.ioctl      = soc_pcm_ioctl;
    } else {
        rtd->ops.open       = soc_pcm_open;
        rtd->ops.hw_params  = soc_pcm_hw_params;
        rtd->ops.prepare    = soc_pcm_prepare;
        rtd->ops.trigger    = soc_pcm_trigger;
        rtd->ops.hw_free    = soc_pcm_hw_free;
        rtd->ops.close      = soc_pcm_close;
        rtd->ops.pointer    = soc_pcm_pointer;
        rtd->ops.ioctl      = soc_pcm_ioctl;
    }
    if (platform->driver->ops) {
        rtd->ops.ack          = platform->driver->ops->ack;
        rtd->ops.copy_user    = platform->driver->ops->copy_user;
        rtd->ops.copy_kernel  = platform->driver->ops->copy_kernel;
        rtd->ops.fill_silence = platform->driver->ops->fill_silence;
        rtd->ops.page         = platform->driver->ops->page;
        rtd->ops.mmap         = platform->driver->ops->mmap;
    }
    if (playback)
        snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &rtd->ops);
    if (capture)
        snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &rtd->ops);
    if (platform->driver->pcm_new) {
        platform->driver->pcm_new(rtd);
    }
    pcm->private_free = platform->driver->pcm_free;
}

