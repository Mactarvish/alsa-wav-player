#include <iostream>
#include "alsa_wav_player.hpp"
#include <string>


int main ( int argc, char *argv[] ) {
    AlsaWavPlayer alsaWavPlayer;
    alsaWavPlayer.ReadWavFile(argv[1]);
    alsaWavPlayer.Play();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    // logger->info("Stop");
    // alsaWavPlayer.Stop();
    while (std::cin >> std::ws)
    {
        auto e = std::cin.get();
        if (e == 'q')
        {
            break;
        }
        else if (e == 'p')
        {
            alsaWavPlayer.Play();
        }
        else if (e == 's')
        {
            alsaWavPlayer.Stop();
        }
        else
        {
            // logger->info("Invalid input");
        }
    }

    return 0;
}