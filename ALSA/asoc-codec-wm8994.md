
# smdk_wm8894
    linux-4.14
    - sound/soc/codecs/wm8994.c     /* platform driver */
    - sound/soc/soc-core.c
    - drivers/mfd/wm8994-core.c     /* platform device */
    - include/linux/regmap.h        /* regmap机制 */


### module_platform_driver(), xxx_probe()
    static int wm8994_probe(struct platform_device *pdev)
    {
        struct wm8994_priv *wm8994;
        wm8994 = devm_kzalloc(&pdev->dev, sizeof(struct wm8994_priv), GFP_KERNEL);
        platform_set_drvdata(pdev, wm8994);
        wm8994->wm8994 = dev_get_drvdata(pdev->dev.parent);

        return snd_soc_register_codec(&pdev->dev, &soc_codec_dev_wm8994,
            wm8994_dai, ARRAY_SIZE(wm8994_dai));
    }

    static struct platform_driver wm8994_codec_driver = {
        .driver = {
            .name = "wm8994-codec",
            ...
        },
        .probe = wm8994_probe,
        .remove = wm8994_remove,
    };
    module_platform_driver(wm8994_codec_driver);

    *** 只注册platform driver，需要相应的platform device，在后面详述；

### struct snd_soc_codec_driver, struct snd_soc_dai_driver, struct snd_soc_dai_ops
    static const struct snd_soc_codec_driver soc_codec_dev_wm8994 = {
        .probe =    wm8994_codec_probe,
        .remove =   wm8994_codec_remove,
        .suspend =  wm8994_codec_suspend,
        .resume =   wm8994_codec_resume,
        .get_regmap =   wm8994_get_regmap,
        .set_bias_level = wm8994_set_bias_level,
    };

    static struct snd_soc_dai_driver wm8994_dai[] = {
        {
            .name = "wm8994-aif1",
            .id = 1,
            .playback = {
                .stream_name = "AIF1 Playback",
                .channels_min = 1,
                .channels_max = 2,
                .rates = WM8994_RATES,
                .formats = WM8994_FORMATS,
                .sig_bits = 24,
            },
            .capture = {
                .stream_name = "AIF1 Capture",
                .channels_min = 1,
                .channels_max = 2,
                .rates = WM8994_RATES,
                .formats = WM8994_FORMATS,
                .sig_bits = 24,
             },
            .ops = &wm8994_aif1_dai_ops,
        },
        ...
    };

    static const struct snd_soc_dai_ops wm8994_aif1_dai_ops = {
        .set_sysclk     = wm8994_set_dai_sysclk,
        .set_fmt        = wm8994_set_dai_fmt,
        .hw_params      = wm8994_hw_params,
        .digital_mute   = wm8994_aif_mute,
        .set_pll        = wm8994_set_fll,
        .set_tristate   = wm8994_set_tristate,
    };

### snd_soc_register_codec()
    int snd_soc_register_codec(struct device *dev,
                   const struct snd_soc_codec_driver *codec_drv,
                   struct snd_soc_dai_driver *dai_drv,
                   int num_dai)
    {
        struct snd_soc_dapm_context *dapm;
        struct snd_soc_codec *codec;
        struct snd_soc_dai *dai;

        codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
        codec->component.codec = codec;
        snd_soc_component_initialize(&codec->component, &codec_drv->component_driver, dev);

        if (codec_drv->probe)
            codec->component.probe = snd_soc_codec_drv_probe;
        if (codec_drv->remove)
            codec->component.remove = snd_soc_codec_drv_remove;
        if (codec_drv->suspend)
            codec->component.suspend = snd_soc_codec_drv_suspend;
        if (codec_drv->resume)
            codec->component.resume = snd_soc_codec_drv_resume;
        if (codec_drv->write)
            codec->component.write = snd_soc_codec_drv_write;
        if (codec_drv->read)
            codec->component.read = snd_soc_codec_drv_read;
        if (codec_drv->set_sysclk)
            codec->component.set_sysclk = snd_soc_codec_set_sysclk_;
        if (codec_drv->set_pll)
            codec->component.set_pll = snd_soc_codec_set_pll_;
        if (codec_drv->set_jack)
            codec->component.set_jack = snd_soc_codec_set_jack_;
        codec->component.ignore_pmdown_time = codec_drv->ignore_pmdown_time;

        dapm = snd_soc_codec_get_dapm(codec);
        dapm->idle_bias_off = codec_drv->idle_bias_off;
        dapm->suspend_bias_off = codec_drv->suspend_bias_off;
        if (codec_drv->seq_notifier)
            dapm->seq_notifier = codec_drv->seq_notifier;
        if (codec_drv->set_bias_level)
            dapm->set_bias_level = snd_soc_codec_set_bias_level;
        codec->dev = dev;
        codec->driver = codec_drv;
        codec->component.val_bytes = codec_drv->reg_word_size;

        if (codec_drv->get_regmap)
            codec->component.regmap = codec_drv->get_regmap(dev);
        for (i = 0; i < num_dai; i++) {
            fixup_codec_formats(&dai_drv[i].playback);
            fixup_codec_formats(&dai_drv[i].capture);
        }

        snd_soc_register_dais(&codec->component, dai_drv, num_dai, false);
        list_for_each_entry(dai, &codec->component.dai_list, list)
            dai->codec = codec;
        ...
        snd_soc_component_add_unlocked(&codec->component);
        list_add(&codec->list, &codec_list);
    }

### snd_soc_component_initialize()
    static int snd_soc_component_initialize(struct snd_soc_component *component,
        const struct snd_soc_component_driver *driver, struct device *dev)
    {
        struct snd_soc_dapm_context *dapm;

        component->name = fmt_single_name(dev, &component->id);
        component->dev = dev;
        component->driver = driver;
        component->probe = component->driver->probe;
        component->remove = component->driver->remove;
        component->suspend = component->driver->suspend;
        component->resume = component->driver->resume;
        component->set_sysclk = component->driver->set_sysclk;
        component->set_pll = component->driver->set_pll;
        component->set_jack = component->driver->set_jack;

        dapm = snd_soc_component_get_dapm(component);
        dapm->dev = dev;
        dapm->component = component;
        dapm->bias_level = SND_SOC_BIAS_OFF;
        dapm->idle_bias_off = true;
        if (driver->seq_notifier)
            dapm->seq_notifier = snd_soc_component_seq_notifier;
        if (driver->stream_event)
            dapm->stream_event = snd_soc_component_stream_event;
        ...
    }

### snd_soc_register_dais()
    static int snd_soc_register_dais(struct snd_soc_component *component,
        struct snd_soc_dai_driver *dai_drv, size_t count, bool legacy_dai_naming)
    {
        struct snd_soc_dai *dai;
        component->dai_drv = dai_drv;
        for (i = 0; i < count; i++) {
            dai = soc_add_dai(component, dai_drv + i, count == 1 && legacy_dai_naming);
        }
    }


### mfd设备
    WM8994本身具备多种功能，除了codec外，它还有作为LDO和GPIO使用，这几种功能共享一些IO和中断资源，
    linux为这种设备提供了一套标准的实现方法：mfd设备;
    其基本思想是为这些功能的公共部分实现一个父设备，以便共享某些系统资源和功能，然后每个子功能实现为它的子设备，
    这样既共享了资源和代码，又能实现合理的设备层次结构;
    主要利用到的API就是：
                    mfd_add_devices()
                    mfd_remove_devices()
                    mfd_cell_enable()
                    mfd_cell_disable()
                    mfd_clone_cell()

### module_i2c_driver(), xxx_probe()
    static int wm8994_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
    {
        struct wm8994 *wm8994;
        wm8994 = devm_kzalloc(&i2c->dev, sizeof(struct wm8994), GFP_KERNEL);
        i2c_set_clientdata(i2c, wm8994);    /* codec作为它的子设备，将会取出并使用这个driver_data */
        wm8994->dev = &i2c->dev;
        wm8994->irq = i2c->irq;
        ...
        wm8994->regmap = devm_regmap_init_i2c(i2c, &wm8994_base_regmap_config);
        return wm8994_device_init(wm8994, i2c->irq);
    }

    static const struct of_device_id wm8994_of_match[] = {
        { .compatible = "wlf,wm1811", .data = (void *)WM1811 },
        { .compatible = "wlf,wm8994", .data = (void *)WM8994 },
        { .compatible = "wlf,wm8958", .data = (void *)WM8958 },
        { }
    };
    MODULE_DEVICE_TABLE(of, wm8994_of_match);

    static struct i2c_driver wm8994_i2c_driver = {
        .driver = {
            .name = "wm8994",
            ...
            .of_match_table = of_match_ptr(wm8994_of_match),
        },
        .probe = wm8994_i2c_probe,
        .remove = wm8994_i2c_remove,
        ...
    };
    module_i2c_driver(wm8994_i2c_driver);

### wm8994_device_init()
    static int wm8994_device_init(struct wm8994 *wm8994, int irq)
    {
        ...
        /* Add the on-chip regulators first for bootstrapping */
        /* 为两个LDO添加mfd子设备 */
        ret = mfd_add_devices(wm8994->dev, 0, wm8994_regulator_devs,
                        ARRAY_SIZE(wm8994_regulator_devs), NULL, 0, NULL);
        ...
        wm8994_irq_init(wm8994);    /* 初始化irq */
        /* 添加codec子设备和gpio子设备,触发probe回调完成codec驱动的初始化工作 */
        mfd_add_devices(wm8994->dev, -1, wm8994_devs,
                        ARRAY_SIZE(wm8994_devs), NULL, 0, NULL);
        ...
    }


###  regmap-io
    对codec进行控制，通常都是通过读写它的内部寄存器完成的，读写的接口通常是I2C或者是SPI接口，
    不过每个codec芯片寄存器的比特位组成都有所不同，寄存器地址的比特位也有所不同;
    内核引入了一套regmap机制和相关的API，这样就可以用统一的操作来实现对这些多样的寄存器的控制;

    regmap使用:
    1. 为codec定义一个regmap_config结构实例，指定codec寄存器的地址和数据位等信息；
    2. 根据codec的控制总线类型，调用以下其中一个函数，得到一个指向regmap结构的指针：
    struct regmap *regmap_init_i2c(struct i2c_client *i2c, const struct regmap_config *config)
    struct regmap *regmap_init_spi(struct spi_device *dev, const struct regmap_config *config)

    3. 把获得的regmap结构指针赋值给codec->control_data；
    4, 调用soc-io的api：snd_soc_codec_set_cache_io使得soc-io和regmap进行关联；
    完成以上步骤后，codec驱动就可以使用诸如snd_soc_read、snd_soc_write、snd_soc_update_bits等API,
    对codec的寄存器进行读写;
