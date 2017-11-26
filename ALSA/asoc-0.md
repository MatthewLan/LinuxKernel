
# ASoC (ALSA System on Chip)
    linux-4.14
    - include/sound/soc*.h
    - sound/soc/

    是建立在标准ALSA驱动层上，为了更好地支持嵌入式处理器和移动设备中的音频Codec的一套软件体系;

### 硬件架构
    嵌入式设备的音频系统可以被划分为板载硬件（Machine）、Soc（Platform）、Codec三大部分;

    Machine
        是指某一款机器，可以是某款设备，某款开发板，又或者是某款智能手机，由此可以看出Machine几乎是不可重用的，
        每个Machine上的硬件实现可能都不一样，CPU不一样，Codec不一样，音频的输入、输出设备也不一样，
        Machine为CPU、Codec、输入输出设备提供了一个载体;
    Platform
        一般是指某一个SoC平台，比如pxaxxx,s3cxxxx,omapxxx等，与音频相关的通常包含该SoC中的时钟、DMA、I2S、
        PCM等等，只要指定了SoC，那么可以认为它会有一个对应的Platform，它只与SoC相关，与Machine无关，
        这样就可以把Platform抽象出来，使得同一款SoC不用做任何的改动，就可以用在不同的Machine中;
        实际上，把Platform认为是某个SoC更好理解;
    Codec
        字面上的意思就是编解码器，Codec里面包含了I2S接口、D/A、A/D、Mixer、PA（功放），通常包含多种输入
        （Mic、Line-in、I2S、PCM）和多个输出（耳机、喇叭、听筒，Line-out），Codec和Platform一样，
        是可重用的部件，同一个Codec可以被不同的Machine使用;
        嵌入式Codec通常通过I2C对内部的寄存器进行控制。

### 软件架构
    在软件层面，ASoC也把嵌入式设备的音频系统同样分为3大部分，Machine，Platform和Codec;

    Codec驱动
        ASoC中的一个重要设计原则就是要求Codec驱动是平台无关的，它包含了一些音频的控件（Controls），
        音频接口，DAMP（动态音频电源管理）的定义和某些Codec IO功能;
        为了保证硬件无关性，任何特定于平台和机器的代码都要移到Platform和Machine驱动中;
        所有的Codec驱动都要提供以下特性：
            Codec DAI 和 PCM的配置信息；
            Codec的IO控制方式（I2C，SPI等）；
            Mixer和其他的音频控件；
            Codec的ALSA音频操作接口；
        必要时，也可以提供以下功能：
            DAPM描述信息；
            DAPM事件处理程序；
            DAC数字静音控制;

    Platform驱动
        它包含了该SoC平台的音频DMA和音频接口的配置和控制（I2S，PCM，AC97等等）；
        它也不能包含任何与板子或机器相关的代码;

    Machine驱动
        Machine驱动负责处理机器特有的一些控件和音频事件（例如，当播放音频时，需要先行打开一个放大器）；
        单独的Platform和Codec驱动是不能工作的，必须由Machine驱动把它们结合在一起,
        才能完成整个设备的音频处理工作;
