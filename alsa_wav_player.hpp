#pragma once
#include <string>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <alsa/asoundlib.h>


//需要从wav文件中读取的三个参数
typedef struct {
    snd_pcm_format_t format;
    unsigned int channels;
    unsigned int rate;
}HWParams;


enum PlayerStatus {
    PLAYING,
    STOPPED,
    PAUSED
};

class AlsaWavPlayer {
public:
    explicit AlsaWavPlayer();
    virtual ~AlsaWavPlayer();
    void Play(std::string srcWavPath);
    void Stop();
    void Pause();
    void Resume();

private:
    void ReadWavFile(std::string wav_file_path);
    void CreatePlayThread();

    int i, fd;
    int ret, dir, size;
    unsigned int val, val2;
    char* buffer;
    snd_pcm_t* handle;
    snd_pcm_hw_params_t* params;
    snd_pcm_uframes_t periodsize;
    snd_pcm_uframes_t frames;
    HWParams hw_params;
    bool orderStop;
    bool orderPause;
    PlayerStatus status;

    std::mutex mtx;  // 互斥锁，保护共享资源
    std::condition_variable cv;  // 条件变量
    bool cvReady;

    std::thread playThread;
};