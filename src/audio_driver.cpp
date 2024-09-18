#include <audio_driver.h>

AudioDriver::AudioDriver(const std::string& device_name, int channels, int sample_rate)
    : pcm_handle_(nullptr)
    , rc_(0)
    , size_(0)
    , frames_(1024)
    , callback_(nullptr)
{
    snd_pcm_hw_params_t* params = nullptr;
    unsigned int         rate   = sample_rate;
    int                  dir;

    // 打开PCM设备
    rc_ = snd_pcm_open(&pcm_handle_, device_name.c_str(), SND_PCM_STREAM_CAPTURE, 0);
    if (rc_ < 0)
    {
        throw std::runtime_error("无法打开PCM设备: " + std::string(snd_strerror(rc_)));
    }

    // 分配参数对象
    snd_pcm_hw_params_alloca(&params);

    // 填充默认参数
    snd_pcm_hw_params_any(pcm_handle_, params);

    // 设置为交错模式
    rc_ = snd_pcm_hw_params_set_access(pcm_handle_, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (rc_ < 0)
    {
        snd_pcm_close(pcm_handle_);
        throw std::runtime_error("无法设置访问类型: " + std::string(snd_strerror(rc_)));
    }

    // 设置数据格式（32-bit Float, Little Endian）
    rc_ = snd_pcm_hw_params_set_format(pcm_handle_, params, SND_PCM_FORMAT_FLOAT_LE); // 设置数据格式为float，SND_PCM_FORMAT_S16_LE则为int16
    if (rc_ < 0)
    {
        snd_pcm_close(pcm_handle_);
        throw std::runtime_error("无法设置数据格式: " + std::string(snd_strerror(rc_)));
    }

    // 设置通道数
    rc_ = snd_pcm_hw_params_set_channels(pcm_handle_, params, CHANNELS);
    if (rc_ < 0)
    {
        snd_pcm_close(pcm_handle_);
        throw std::runtime_error("无法设置通道数: " + std::string(snd_strerror(rc_)));
    }

    // 设置采样率
    rc_ = snd_pcm_hw_params_set_rate_near(pcm_handle_, params, &rate, &dir);
    if (rc_ < 0)
    {
        snd_pcm_close(pcm_handle_);
        throw std::runtime_error("无法设置采样率: " + std::string(snd_strerror(rc_)));
    }

    // 设置周期大小
    rc_ = snd_pcm_hw_params_set_period_size_near(pcm_handle_, params, &frames_, &dir);
    if (rc_ < 0)
    {
        snd_pcm_close(pcm_handle_);
        throw std::runtime_error("无法设置周期大小: " + std::string(snd_strerror(rc_)));
    }

    // 应用参数到设备
    rc_ = snd_pcm_hw_params(pcm_handle_, params);
    if (rc_ < 0)
    {
        snd_pcm_close(pcm_handle_);
        throw std::runtime_error("无法设置硬件参数: " + std::string(snd_strerror(rc_)));
    }

    // 获取实际的周期大小（帧数）
    snd_pcm_hw_params_get_period_size(params, &frames_, &dir);

    // 计算缓冲区大小
    size_   = frames_ * CHANNELS * SAMPLE_SIZE / 8;
    buffer_ = std::shared_ptr<uint8_t>(new uint8_t[size_](), std::default_delete<uint8_t[]>());
    if (!buffer_.get())
    {
        snd_pcm_close(pcm_handle_);
        throw std::runtime_error("无法分配缓冲区");
    }

    // 获取周期时间（微秒）
    unsigned int period_time;
    snd_pcm_hw_params_get_period_time(params, &period_time, &dir);
}

AudioDriver::~AudioDriver()
{
    snd_pcm_drain(pcm_handle_);
    snd_pcm_close(pcm_handle_);
}

void AudioDriver::GetPcmDevicesList()
{
    int                  card = -1;
    snd_ctl_t*           ctl_handle;
    snd_pcm_info_t*      pcm_info;
    snd_ctl_card_info_t* card_info;

    // 分配内存
    snd_pcm_info_alloca(&pcm_info);
    snd_ctl_card_info_alloca(&card_info);

    // 遍历所有声卡
    while (snd_card_next(&card) >= 0 && card >= 0)
    {
        char card_name[32];
        snprintf(card_name, sizeof(card_name), "hw:%d", card);

        if (snd_ctl_open(&ctl_handle, card_name, 0) < 0)
        {
            std::cerr << "无法打开控制接口: " << card_name << std::endl;
            continue;
        }

        if (snd_ctl_card_info(ctl_handle, card_info) < 0)
        {
            std::cerr << "无法获取卡信息: " << card_name << std::endl;
            snd_ctl_close(ctl_handle);
            continue;
        }

        std::cout << "声卡: " << snd_ctl_card_info_get_id(card_info) << " - "
                  << snd_ctl_card_info_get_name(card_info) << std::endl;

        int device = -1;

        // 遍历所有设备
        while (snd_ctl_pcm_next_device(ctl_handle, &device) >= 0 && device >= 0)
        {
            snd_pcm_info_set_device(pcm_info, device);
            snd_pcm_info_set_subdevice(pcm_info, 0);
            snd_pcm_info_set_stream(pcm_info, SND_PCM_STREAM_CAPTURE); // 或 SND_PCM_STREAM_PLAYBACK

            if (snd_ctl_pcm_info(ctl_handle, pcm_info) < 0)
            {
                std::cerr << "无法获取PCM信息: " << card_name << ", device: " << device << std::endl;
                continue;
            }

            std::cout << "  设备: hw:" << card << "," << device
                      << " - " << snd_pcm_info_get_id(pcm_info)
                      << " (" << snd_pcm_info_get_name(pcm_info) << ")" << std::endl;
        }

        snd_ctl_close(ctl_handle);
    }
}

bool AudioDriver::GetPcmFrames()
{
    rc_ = snd_pcm_readi(pcm_handle_, buffer_.get(), frames_);
    if (rc_ == -EPIPE)
    {
        std::cerr << "缓冲区溢出!" << std::endl;
        snd_pcm_prepare(pcm_handle_);
        return false;
    }
    else if (rc_ < 0)
    {
        std::cerr << "读取错误: " << snd_strerror(rc_) << std::endl;
        return false;
    }
    else if (rc_ != (int)frames_)
    {
        std::cerr << "短读取，读取了 " << rc_ << " 帧" << std::endl;
        return false;
    }

    if (callback_)
    {
        callback_(buffer_.get(), size_);
    }

    return true;
}

bool AudioDriver::InstantCallback(AudioBufferCallback callback)
{
    if (!callback)
    {
        return false;
    }

    callback_ = callback;

    return true;
}
