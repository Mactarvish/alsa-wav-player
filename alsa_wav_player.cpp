#include "alsa_wav_player.hpp"


//下面这四个结构体是为了分析wav头的
typedef struct {
    u_int magic;   /* 'RIFF' */
    u_int length;  /* filelen */
    u_int type;    /* 'WAVE' */
} WaveHeader;

typedef struct {
    u_short format;       /* see WAV_FMT_* */
    u_short channels;
    u_int sample_fq;     /* frequence of sample */
    u_int byte_p_sec;
    u_short byte_p_spl;  /* samplesize; 1 or 2 bytes */
    u_short bit_p_spl;   /* 8, 12 or 16 bit */
} WaveFmtBody;

typedef struct {
    WaveFmtBody format;
    u_short ext_size;
    u_short bit_p_spl;
    u_int channel_mask;
    u_short guid_format; /* WAV_FMT_* */
    u_char guid_tag[14]; /* WAV_GUID_TAG */
} WaveFmtExtensibleBody;

typedef struct {
    u_int type; /* 'data' */
    u_int length; /* samplecount */
} WaveChunkHeader;

#define COMPOSE_ID(a,b,c,d) ((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))
#define WAV_RIFF COMPOSE_ID('R','I','F','F')
#define WAV_WAVE COMPOSE_ID('W','A','V','E')
#define WAV_FMT COMPOSE_ID('f','m','t',' ')
#define WAV_DATA COMPOSE_ID('d','a','t','a')
//分析wav的头，并解析出alsa的param需要的三个参数
int check_wavfile(int fd, HWParams* hw_params)
{
    int ret;
    int i, len;
    WaveHeader* header;
    WaveFmtBody* fmt;
    WaveChunkHeader* chunk_header;
    unsigned char* pbuf = (unsigned char*)malloc(128); 
    if(nullptr == pbuf)
    {
        // logger->info("pbuf malloc error");
        return -1;
    }
    //1. check Wave Header
    len = sizeof(WaveHeader);
    if( (ret=read(fd, pbuf, len)) != len)
    {
        // logger->info("read error");
        return -1;
    }
    header = (WaveHeader*)pbuf;
    if( (header->magic!=WAV_RIFF) || (header->type!=WAV_WAVE))
    {
        // logger->info("not a wav file");
        return -1;
    }
    //2. check Wave Fmt
    len = sizeof(WaveChunkHeader)+sizeof(WaveFmtBody);
    if( (ret=read(fd, pbuf, len)) != len)
    {
        // logger->info("read error");
        return -1;
    }
    chunk_header = (WaveChunkHeader*)pbuf;
    if( chunk_header->type!=WAV_FMT)
    {
        // logger->info("fmt body error");
        return -1;
    }
    fmt = (WaveFmtBody*)(pbuf+sizeof(WaveChunkHeader));
    if(fmt->format != 0x0001) //WAV_FMT_PCM
    {
        // logger->info("format is not pcm");
        return -1;
    }
    // logger->info("format=0x%x, channels=0x%x,sample_fq=%d,byte_p_sec=%d,byte_p_sample=%d,bit_p_sample=%d",
            // fmt->format, fmt->channels,fmt->sample_fq, fmt->byte_p_sec,
            // fmt->byte_p_spl, fmt->bit_p_spl);
    //copy params
    hw_params->channels = fmt->channels;
    hw_params->rate = fmt->sample_fq;
    switch(fmt->bit_p_spl)
    {
        case 8:
            hw_params->format = SND_PCM_FORMAT_U8;
            break;
        case 16:
            hw_params->format = SND_PCM_FORMAT_S16_LE;
            break;
        default:
            // logger->info("FIXME: add more format");
            break;
    }
    //3. check data chunk
    len = sizeof(WaveChunkHeader);
    if( (ret=read(fd, pbuf, len)) != len)
    {
        // logger->info("read error");
        return -1;
    }
    chunk_header = (WaveChunkHeader*)pbuf;
    if(chunk_header->type != WAV_DATA)
    {
        // logger->info("not data chunk");
        return -1;
    }
    // logger->info("pcm_data_size=0x%x",chunk_header->length);

    free(pbuf);
    pbuf = nullptr;
    return -1;
}

AlsaWavPlayer::AlsaWavPlayer() : stopped(false), paused(false), buffer(nullptr), handle(nullptr)
{

}

AlsaWavPlayer::~AlsaWavPlayer() {
    if (playThread.joinable()) {
        playThread.join();
    }
    if (handle != nullptr) {
        snd_pcm_drain(handle);
        snd_pcm_close(handle);
        handle = nullptr;
    }
    if (buffer != nullptr) {
        free(buffer);
        buffer = nullptr;
    }
    // return EXIT_SUCCESS;
}

void AlsaWavPlayer::ReadWavFile(const std::string &wav_file_path) {
    fd = open(wav_file_path.c_str(), O_RDWR);
    if(fd<0)
    {
        // logger->info("file open error");
        // return -1;
    }
    check_wavfile(fd, &hw_params);   //从wav头中分析出的参数，保存在hw_param中
    //1. 打开alsa
    if( (ret = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0)
    {
        // logger->info("open pcm device error:%s", snd_strerror(ret));
        // return -1;
    }
    //2. 给参数分配空间,并用hw_param(从wav头中分析出的参数)初始化
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle, params);
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    //snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE); //last param get from wav file
    snd_pcm_hw_params_set_format(handle, params, hw_params.format);

    //snd_pcm_hw_params_set_channels(handle, params, 2);
    snd_pcm_hw_params_set_channels(handle, params, hw_params.channels); //last param get from wav file
    // logger->info("hw_params: format=%d, channels=%d", hw_params.format, hw_params.channels);

    val = hw_params.rate; //44100;
    snd_pcm_hw_params_set_rate_near(handle,params, &val, &dir);

    frames = 32*4;
    snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);
    //3. set param to driver
    if((ret=snd_pcm_hw_params(handle, params)) < 0)
    {
        // logger->info("set hw params error:%s", snd_strerror(ret));
        // return -1;
    }

    snd_pcm_hw_params_get_period_size(params, &frames, &dir);
    size = frames*4; //2byte/smaple, 2 channels
    buffer = (char*)malloc(size);

    snd_pcm_hw_params_get_period_time(params, &val, &dir);
}

void AlsaWavPlayer::Play() {
    playThread = std::thread([&]() {
        while((!stopped) && (!paused))
        {
            ret = read(fd, buffer, size);                //3.从wav文件中读取数据
            if(ret==0)
            {
                // logger->info("end of file");
                // return 0;
                break;
            }else if (ret!=size)
            {
                // logger->info("short read");
            }

            ret = snd_pcm_writei(handle, buffer, frames);   //4.将读取数据写到driver中进行播放
            if(ret == -EPIPE)
            {
                //logger->info("-EPIPE");
                snd_pcm_prepare(handle);
            }
            // logger->info("time count = {} ms", get_current_time() - t_);
        }
        snd_pcm_drop(handle);
    });
}

void AlsaWavPlayer::Stop() {
    stopped = true;
}
