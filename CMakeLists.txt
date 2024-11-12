project(pgs_demo)

add_executable(play_csdn
        play.cpp
        alsa_wav_player.cpp
)

target_link_libraries(play_csdn PRIVATE asound)
