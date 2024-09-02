#include <iostream>
#include <memory>
#include <csignal>

#include <audio_driver.h>

void signalHandler(int sig)
{
    struct sigaction new_action;
    new_action.sa_handler = SIG_DFL;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;

    sigaction(sig, &new_action, NULL);
    raise(sig);
}

int main(int argc, char* argv[])
{
    struct sigaction action;
    action.sa_handler = signalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);
    
    std::shared_ptr<AudioDriver> audio_driver = std::make_shared<AudioDriver>("default", 2, 44100);
    audio_driver->GetPcmDevicesList();

    while (true)
    {
        audio_driver->GetPcmFrames();
    }

    return 0;
}