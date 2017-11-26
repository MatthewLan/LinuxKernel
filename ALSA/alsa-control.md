
# ALSA (Advanced Linux Sound Architecture)
    linux-4.14
    - include/sound/
    - sound/

    ALSA - control:
    include/sound/control.h         --> pcm core
    sound/core/
    include/uapi/sound/asound.h     --> iface, access,

### 回调函数:
    typedef int (snd_kcontrol_info_t)(struct snd_kcontrol * kcontrol,
                                      struct snd_ctl_elem_info * uinfo);
    typedef int (snd_kcontrol_get_t)(struct snd_kcontrol * kcontrol,
                                     struct snd_ctl_elem_value * ucontrol);
    typedef int (snd_kcontrol_put_t)(struct snd_kcontrol * kcontrol,
                                     struct snd_ctl_elem_value * ucontrol);
    typedef int (snd_kcontrol_tlv_rw_t)(struct snd_kcontrol *kcontrol,
                                        int op_flag, /* SNDRV_CTL_TLV_OP_XXX */
                                        unsigned int size,
                                        unsigned int __user *tlv);

### struct snd_kcontrol_new
    struct snd_kcontrol_new {
        snd_ctl_elem_iface_t iface;     /* interface identifier */
        unsigned int device;            /* device/client number */
        unsigned int subdevice;         /* subdevice (substream) number */
        const unsigned char *name;      /* ASCII name of item */
        unsigned int index;             /* index of item */
        unsigned int access;            /* access rights */
        unsigned int count;             /* count of same elements */
        snd_kcontrol_info_t *info;
        snd_kcontrol_get_t *get;
        snd_kcontrol_put_t *put;
        union {
            snd_kcontrol_tlv_rw_t *c;
            const unsigned int *p;
        } tlv;
        unsigned long private_value;
    };

    face字段指出了control的类型
        #define SNDRV_CTL_ELEM_IFACE_CARD           /* global control */
        #define SNDRV_CTL_ELEM_IFACE_HWDEP          /* hardware dependent device */
        #define SNDRV_CTL_ELEM_IFACE_MIXER          /* virtual mixer device */
        #define SNDRV_CTL_ELEM_IFACE_PCM            /* PCM device */
        #define SNDRV_CTL_ELEM_IFACE_RAWMIDI        /* RawMidi device */
        #define SNDRV_CTL_ELEM_IFACE_TIMER          /* timer device */
        #define SNDRV_CTL_ELEM_IFACE_SEQUENCER      /* sequencer client */

    name字段是该control的名字，为control的作用是按名字来归类的
        1. control的名字需要遵循一些标准，通常可以分成3部分来定义control的名字：源--方向--功能。
        1.1 源   - control的输入端，预定义常用的源，例如：Master，PCM，CD，Line等等
        1.2 方向 - control的数据流向，例如：Playback，Capture，Bypass，Bypass Capture等等，
                  也可以不定义方向，这时表示该Control是双向的（playback和capture）;
        1.3 功能 - 根据control的功能，可以是以下字符串：Switch，Volume，Route等等;
        2. 命名上的特例：
        2.1 全局的capture和playback
            "Capture Source"，"Capture Volume"，"Capture Switch"
            "Playback Volume"，"Playback Switch"
        2.2 Tone-controles
            音调控制的开关和音量命名为：Tone Control - XXX
            例如，"Tone Control - Switch"，"Tone Control - Bass"，"Tone Control - Center"
        2.3 3D controls
            3D控件的命名规则："3D Control - Switch"，"3D Control - Center"，"3D Control - Space"
        2.4 Mic boost
            麦克风音量加强控件命名为："Mic Boost"或"Mic Boost(6dB)"。

    index字段用于保存该control的在该卡中的编号
        如果声卡中有不止一个codec，每个codec中有相同名字的control，可以通过index来区分这些controls;
        当index为0时，则可以忽略这种区分策略;

    access字段包含了该control的访问类型,每一个bit代表一种访问类型
        默认的访问类型是：SNDDRV_CTL_ELEM_ACCESS_READWRITE，表明该control支持读和写操作;
        如果access字段没有定义（.access==0），此时也认为是READWRITE类型;
        #define SNDRV_CTL_ELEM_ACCESS_READ
        #define SNDRV_CTL_ELEM_ACCESS_WRITE
        #define SNDRV_CTL_ELEM_ACCESS_READWRITE
        #define SNDRV_CTL_ELEM_ACCESS_VOLATILE      /* control value may be changed
                                                       without a notification */
        #define SNDRV_CTL_ELEM_ACCESS_TIMESTAMP     /* when was control changed */
        #define SNDRV_CTL_ELEM_ACCESS_TLV_READ      /* TLV read is possible */
        #define SNDRV_CTL_ELEM_ACCESS_TLV_WRITE     /* TLV write is possible */
        #define SNDRV_CTL_ELEM_ACCESS_TLV_READWRITE
        #define SNDRV_CTL_ELEM_ACCESS_TLV_COMMAND   /* TLV command is possible */
        #define SNDRV_CTL_ELEM_ACCESS_INACTIVE      /* control does actually nothing,
                                                       but may be updated */
        #define SNDRV_CTL_ELEM_ACCESS_LOCK          /* write lock */
        #define SNDRV_CTL_ELEM_ACCESS_OWNER         /* write lock owner */
        #define SNDRV_CTL_ELEM_ACCESS_TLV_CALLBACK  /* kernel use a TLV callback */
        #define SNDRV_CTL_ELEM_ACCESS_USER          /* user space element */

    private_value字段包含了一个任意的长整数类型值
        该值可以通过info，get，put这几个回调函数访问;
        可以自己决定如何使用该字段，例如可以把它拆分成多个位域，又或者是一个指针，指向某一个数据结构;

    tlv字段为该control提供元数据;

### info回调函数
    用于获取control的详细信息;它的主要工作就是填充通过参数传入的snd_ctl_elem_info对象;

    struct snd_ctl_elem_info {
        struct snd_ctl_elem_id id;  /* W: element ID */
        snd_ctl_elem_type_t type;   /* R: value type - SNDRV_CTL_ELEM_TYPE_* */
        unsigned int access;        /* R: value access (bitmask) - SNDRV_CTL_ELEM_ACCESS_* */
        unsigned int count;         /* count of values */
        __kernel_pid_t owner;       /* owner's PID of this control */
        union {
            struct {
                long min;           /* R: minimum value */
                long max;           /* R: maximum value */
                long step;          /* R: step (0 variable) */
            } integer;
            struct {
                long long min;      /* R: minimum value */
                long long max;      /* R: maximum value */
                long long step;     /* R: step (0 variable) */
            } integer64;
            struct {
                unsigned int items; /* R: number of items */
                unsigned int item;  /* W: item number */
                char name[64];      /* R: value name */
                __u64 names_ptr;    /* W: names list (ELEM_ADD only) */
                unsigned int names_length;
            } enumerated;
            unsigned char reserved[128];
        } value;
        union {
            unsigned short d[4];        /* dimensions */
            unsigned short *d_ptr;      /* indirect - obsoleted */
        } dimen;
        unsigned char reserved[64-4*sizeof(unsigned short)];
    };

    type字段指出该control的值类型
        #define SNDRV_CTL_ELEM_TYPE_NONE        /* invalid */
        #define SNDRV_CTL_ELEM_TYPE_BOOLEAN     /* boolean type */
        #define SNDRV_CTL_ELEM_TYPE_INTEGER     /* integer type */
        #define SNDRV_CTL_ELEM_TYPE_ENUMERATED  /* enumerated type */
        #define SNDRV_CTL_ELEM_TYPE_BYTES       /* byte array */
        #define SNDRV_CTL_ELEM_TYPE_IEC958      /* IEC958 (S/PDIF) setup */
        #define SNDRV_CTL_ELEM_TYPE_INTEGER64   /* 64-bit integer type */
    count字段指出该control中包含有多少个元素单元
        比如，立体声的音量control左右两个声道的音量值，它的count字段等于2;
    value字段是一个联合体（union），value的内容和control的类型有关
        其中，boolean和integer类型是相同的;
        ENUMERATED类型有些特殊,它的value需要设定一个字符串和字符串的索引;
        example:
            static int snd_myctl_enum_info(struct snd_kcontrol *kcontrol,
                                           struct snd_ctl_elem_info *uinfo)
            {
                static char *texts[4] = {"First", "Second", "Third", "Fourth"};
                uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
                uinfo->count = 1;
                uinfo->value.enumerated.items = 4;
                if (uinfo->value.enumerated.item > 3)
                    uinfo->value.enumerated.item = 3;
                strcpy(uinfo->value.enumerated.name, texts[uinfo->value.enumerated.item]);
                return 0;
            }

    int snd_ctl_boolean_mono_info(struct snd_kcontrol *kcontrol,
                                  struct snd_ctl_elem_info *uinfo)
    {
        uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
        uinfo->count = 1;
        uinfo->value.integer.min = 0;
        uinfo->value.integer.max = 1;
        return 0;
    }

    int snd_ctl_boolean_stereo_info(struct snd_kcontrol *kcontrol,
                                    struct snd_ctl_elem_info *uinfo)
    {
        uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
        uinfo->count = 2;
        uinfo->value.integer.min = 0;
        uinfo->value.integer.max = 1;
        return 0;
    }

    int snd_ctl_enum_info(struct snd_ctl_elem_info *info, unsigned int channels,
                          unsigned int items, const char *const names[])
    {
        info->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
        info->count = channels;
        info->value.enumerated.items = items;
        if (!items)
            return 0;
        if (info->value.enumerated.item >= items)
            info->value.enumerated.item = items - 1;
        WARN(strlen(names[info->value.enumerated.item]) >= sizeof(info->value.enumerated.name),
             "ALSA: too long item name '%s'\n", names[info->value.enumerated.item]);
        strlcpy(info->value.enumerated.name,
                names[info->value.enumerated.item],
                sizeof(info->value.enumerated.name));
        return 0;
    }

### get回调函数
    用于读取control的当前值，并返回给用户空间的应用程序;

    struct snd_ctl_elem_value {
        struct snd_ctl_elem_id id;          /* W: element ID */
        unsigned int indirect: 1;           /* W: indirect access - obsoleted */
        union {
            union {
                long value[128];
                long *value_ptr;            /* obsoleted */
            } integer;
            union {
                long long value[64];
                long long *value_ptr;       /* obsoleted */
            } integer64;
            union {
                unsigned int item[128];
                unsigned int *item_ptr;     /* obsoleted */
            } enumerated;
            union {
                unsigned char data[512];
                unsigned char *data_ptr;    /* obsoleted */
            } bytes;
            struct snd_aes_iec958 iec958;
        } value;                            /* RO */
        struct timespec tstamp;
        unsigned char reserved[128-sizeof(struct timespec)];
    };

    value字段的赋值依赖于control的类型（如同info回调）
        很多声卡的驱动利用它存储硬件寄存器的地址、bit-shift和bit-mask;
        如果control的count字段大于1，表示control有多个元素单元，get回调函数也应该为value填充多个数值

### put回调函数
    用于把应用程序的控制值设置到control中;
        当control的值被改变时，put回调必须要返回1;
        如果值没有被改变，则返回0;
        如果发生了错误，则返回一个负数的错误号;
    当control的count大于1时，put回调也要处理多个control中的元素值;


### struct snd_kcontrol
    struct snd_kcontrol {
        struct list_head list;          /* list of controls */
        struct snd_ctl_elem_id id;
        unsigned int count;             /* count of same elements */
        snd_kcontrol_info_t *info;
        snd_kcontrol_get_t *get;
        snd_kcontrol_put_t *put;
        union {
            snd_kcontrol_tlv_rw_t *c;
            const unsigned int *p;
        } tlv;
        unsigned long private_value;
        void *private_data;
        void (*private_free)(struct snd_kcontrol *kcontrol);
        struct snd_kcontrol_volatile vd[0];     /* volatile data */
    };

### snd_ctl_new1()
    struct snd_kcontrol *snd_ctl_new1(const struct snd_kcontrol_new *ncontrol,
                                      void *private_data)
    {
        struct snd_kcontrol *kctl;
        unsigned int count;
        unsigned int access;
        count = ncontrol->count;
        if (count == 0)
            count = 1;
        access = ncontrol->access;
        if (access == 0)
            access = SNDRV_CTL_ELEM_ACCESS_READWRITE;
        access &= (SNDRV_CTL_ELEM_ACCESS_READWRITE \
                  | SNDRV_CTL_ELEM_ACCESS_VOLATILE \
                  | SNDRV_CTL_ELEM_ACCESS_INACTIVE \
                  | SNDRV_CTL_ELEM_ACCESS_TLV_READWRITE \
                  | SNDRV_CTL_ELEM_ACCESS_TLV_COMMAND \
                  | SNDRV_CTL_ELEM_ACCESS_TLV_CALLBACK);
        snd_ctl_new(&kctl, count, access, NULL);
        kctl->id.iface = ncontrol->iface;
        kctl->id.device = ncontrol->device;
        kctl->id.subdevice = ncontrol->subdevice;
            if (ncontrol->name) {
        strlcpy(kctl->id.name, ncontrol->name, sizeof(kctl->id.name));
        kctl->id.index = ncontrol->index;
        kctl->info = ncontrol->info;
        kctl->get = ncontrol->get;
        kctl->put = ncontrol->put;
        kctl->tlv.p = ncontrol->tlv.p;
        kctl->private_value = ncontrol->private_value;
        kctl->private_data = private_data;
        return kctl;
    }
    分配一个新的snd_kcontrol实例，并把ncontrol中相应的值复制到该实例中;

### snd_ctl_new()
    static int snd_ctl_new(struct snd_kcontrol **kctl, unsigned int count,
                           unsigned int access, struct snd_ctl_file *file)
    {
        unsigned int size;
        unsigned int idx;
        size  = sizeof(struct snd_kcontrol);
        size += sizeof(struct snd_kcontrol_volatile) * count;
        *kctl = kzalloc(size, GFP_KERNEL);
        for (idx = 0; idx < count; idx++) {
            (*kctl)->vd[idx].access = access;
            (*kctl)->vd[idx].owner = file;
        }
        (*kctl)->count = count;
        return 0;
    }

### snd_ctl_add()
    int snd_ctl_add(struct snd_card *card, struct snd_kcontrol *kcontrol)
    {
        struct snd_ctl_elem_id id;
        unsigned int idx;
        unsigned int count;
        id = kcontrol->id;
        down_write(&card->controls_rwsem);
        ...
        list_add_tail(&kcontrol->list, &card->controls);
        card->controls_count += kcontrol->count;
        kcontrol->id.numid = card->last_numid + 1;
        card->last_numid += kcontrol->count;
        id = kcontrol->id;
        count = kcontrol->count;
        up_write(&card->controls_rwsem);
        for (idx = 0; idx < count; idx++, id.index++, id.numid++)
            snd_ctl_notify(card, SNDRV_CTL_EVENT_MASK_ADD, &id);
        return 0;
    }
    把该control绑定到声卡对象card当中;

### 元数据（Metadata）
    include/sound/tlv.h
    include/uapi/sound/tlv.h

    很多mixer control需要提供以dB为单位的信息，可以使用DECLARE_TLV_xxx宏来定义一些包含这种信息的变量，
    然后把control的tlv.p字段指向这些变量，最后在access字段中加上SNDRV_CTL_ELEM_ACCESS_TLV_READ标志;

    #define DECLARE_TLV_DB_SCALE        SNDRV_CTL_TLVD_DECLARE_DB_SCALE
    #define SNDRV_CTL_TLVD_DECLARE_DB_SCALE(name, min, step, mute) \
            unsigned int name[] = { \
                SNDRV_CTL_TLVD_DB_SCALE_ITEM(min, step, mute) \
            }
    DECLARE_TLV_DB_SCALE宏定义的mixer control，它所代表的值按一个固定的dB值的步长变化
        第一个参数是要定义变量的名字;
        第二个参数是最小值，以0.01dB为单位;
        第三个参数是变化的步长，也是以0.01dB为单位;
        如果该control处于最小值时会做出mute时，需要把第四个参数设为1;

    #define DECLARE_TLV_DB_LINEAR       SNDRV_CTL_TLVD_DECLARE_DB_LINEAR
    #define SNDRV_CTL_TLVD_DECLARE_DB_LINEAR(name, min_dB, max_dB) \
            unsigned int name[] = { \
                SNDRV_CTL_TLVD_DB_LINEAR_ITEM(min_dB, max_dB) \
            }
    DECLARE_TLV_DB_LINEAR宏定义的mixer control，它的输出随值的变化而线性变化
        第一个参数是要定义变量的名字;
        第二个参数是最小值，以0.01dB为单位;
        第二个参数是最大值，以0.01dB为单位;
        如果该control处于最小值时会做出mute时，需要把第二个参数设为TLV_DB_GAIN_MUTE;

    example:
        static DECLARE_TLV_DB_SCALE(db_scale_my_control, -4050, 150, 0);
        static struct snd_kcontrol_new my_control __devinitdata = {
            ...
            .access = SNDRV_CTL_ELEM_ACCESS_READWRITE | SNDRV_CTL_ELEM_ACCESS_TLV_READ,
            ...
            .tlv.p = db_scale_my_control,
        };

### snd_ctl_create()
    Control设备的创建过程大体上和PCM设备的创建过程相同;

    int snd_card_new(struct device *parent, int idx, const char *xid,
                     struct module *module, int extra_size,
                     struct snd_card **card_ret)
    {
        ...
        snd_ctl_create(card);       /* SNDRV_DEV_CONTROL */
        ...
    }

    int snd_ctl_create(struct snd_card *card)
    {
        static struct snd_device_ops ops = {
            .dev_free = snd_ctl_dev_free,
            .dev_register = snd_ctl_dev_register,
            .dev_disconnect = snd_ctl_dev_disconnect,
        };
        snd_device_initialize(&card->ctl_dev, card);
        dev_set_name(&card->ctl_dev, "controlC%d", card->number);
        snd_device_new(card, SNDRV_DEV_CONTROL, card, &ops);
    }

### snd_ctl_dev_register()
    static int snd_ctl_dev_register(struct snd_device *device)
    {
        struct snd_card *card = device->device_data;
        return snd_register_device(SNDRV_DEVICE_TYPE_CONTROL, card, -1,
                                   &snd_ctl_f_ops, card, &card->ctl_dev);
    }
    snd_register_device() 在 alsa-register.md 中;

    static const struct file_operations snd_ctl_f_ops =
    {
        .owner          = THIS_MODULE,
        .read           = snd_ctl_read,
        .open           = snd_ctl_open,
        .release        = snd_ctl_release,
        .llseek         = no_llseek,
        .poll           = snd_ctl_poll,
        .unlocked_ioctl = snd_ctl_ioctl,
        .compat_ioctl   = snd_ctl_ioctl_compat,
        .fasync         = snd_ctl_fasync,
    };
