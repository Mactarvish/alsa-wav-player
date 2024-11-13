#include <iostream>
#include "alsa_wav_player.hpp"
#include <string>
#include "../../utils/logger.h"

static auto logger = GetLogger("play_csdn");



int main ( int argc, char *argv[] ) {
    AlsaWavPlayer alsaWavPlayer;
    std::string input;

    while (std::cin >> input)
    {
        if (input == "quit")
        {
            break;
        }
        else if (input == "play")
        {
            logger->info("playing");
            alsaWavPlayer.Play(argv[1]);
        }
        else if (input == "stop")
        {
            logger->info("stopped");
            alsaWavPlayer.Stop();
        }
        else if (input == "pause")
        {
            logger->info("paused");
            alsaWavPlayer.Pause();
        }
        else if (input == "resume")
        {
            logger->info("resume...");
            alsaWavPlayer.Resume();
        }
        else
        {
            logger->info("Invalid input");
        }
    }

    return 0;
}