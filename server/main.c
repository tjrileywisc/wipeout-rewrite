

#include <network.h>

#include <stdbool.h>
#include <stdio.h>

int main(int argc, char** argv) {

    printf("welcome to the server!\n");

    bool should_quit = false;

    network_open_ip();

    while (!should_quit)
    {
        //orchestrate_frame();
    }

    return 0;
}