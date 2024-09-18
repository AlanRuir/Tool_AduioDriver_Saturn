#ifndef __AUDIO_DRIVER_H__
#define __AUDIO_DRIVER_H__

#include <iostream>
#include <fstream>
#include <memory>
#include <functional>
#include <alsa/asoundlib.h>

class AudioDriver
{
    using AudioBufferCallback = std::function<void(uint8_t*, int)>;

public:
    AudioDriver(const std::string& device_name, int channels, int sample_rate);
    ~AudioDriver();

    void GetPcmDevicesList();
    bool GetPcmFrames();
    bool InstantCallback(AudioBufferCallback callback);

private:
    snd_pcm_t*               pcm_handle_;
    int                      rc_;
    int                      size_;
    snd_pcm_uframes_t        frames_;
    std::shared_ptr<uint8_t> buffer_;
    AudioBufferCallback      callback_;

private:
    constexpr static int CHANNELS    = 2;
    constexpr static int SAMPLE_SIZE = 32;
};

#endif // __AUDIO_DRIVER_H__