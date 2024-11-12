#pragma once
#include <thread>
#include <string>
#include <alsa/asoundlib.h>

//需要从wav文件中读取的三个参数
typedef struct {
    snd_pcm_format_t format;
    unsigned int channels;
    unsigned int rate;
}HWParams;

class AlsaWavPlayer {
public:
    explicit AlsaWavPlayer();
    virtual ~AlsaWavPlayer();
    void ReadWavFile(const std::string &wav_file_path);
    void Play();
    void Stop();

private:
    int i, fd;
    int ret, dir, size;
    unsigned int val, val2;
    char* buffer;
    snd_pcm_t* handle;
    snd_pcm_hw_params_t* params;
    snd_pcm_uframes_t periodsize;
    snd_pcm_uframes_t frames;
    HWParams hw_params;
    bool stopped;
    bool paused;

    std::thread playThread;
};