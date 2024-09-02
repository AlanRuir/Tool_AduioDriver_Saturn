#ifndef __AUDIO_DRIVER_H__
#define __AUDIO_DRIVER_H__

#include <iostream>
#include <fstream>
#include <memory>
#include <alsa/asoundlib.h>

class AudioDriver
{
public:
    AudioDriver(const std::string& device_name, int channels, int sample_rate);
    ~AudioDriver();

    void GetPcmDevicesList();
    bool GetPcmFrames();

private:
    snd_pcm_t*               pcm_handle_;
    int                      rc_;
    int                      size_;
    snd_pcm_uframes_t        frames_;
    std::shared_ptr<uint8_t> buffer_;

private:
    constexpr static int CHANNELS = 2;
    constexpr static int SAMPLE_SIZE = 32;
};

#endif  // __AUDIO_DRIVER_H__