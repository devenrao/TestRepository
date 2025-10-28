#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

extern "C" {
#include "libpdbg.h"
}

int main()
{
    constexpr auto devtree = "/var/lib/phosphor-software-manager/pnor/rw/DEVTREE";
    //constexpr auto devtree = "/tmp/Rainier-4U-MRW.dtb";

    // PDBG_DTB environment variable set to CEC device tree path
    if (setenv("PDBG_DTB", devtree, 1)) 
    {   
        std::cerr << "Failed to set PDBG_DTB: " << strerror(errno) << std::endl;
        return 0;
    }   

    pdbg_set_backend(PDBG_BACKEND_SBEFIFO, NULL);

    constexpr int NUM_RUNS = 10;
    double total_ms = 0.0;

    for (int i = 0; i < NUM_RUNS; ++i) {
        struct timespec start{}, end{};

        clock_gettime(CLOCK_MONOTONIC, &start);

        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            if (!pdbg_targets_init(NULL)) {
                std::cerr << "pdbg_targets_init failed\n";
                _exit(1);
            }
            _exit(0);
        } else if (pid > 0) {
            int status = 0;
            waitpid(pid, &status, 0);
            clock_gettime(CLOCK_MONOTONIC, &end);

            double elapsed_ms =
                (end.tv_sec - start.tv_sec) * 1000.0 +
                (end.tv_nsec - start.tv_nsec) / 1.0e6;

            total_ms += elapsed_ms;
            std::cout << "Run " << i + 1 << ": " << elapsed_ms << " ms" << std::endl;
        } else {
            perror("fork");
            return 1;
        }
    }

    std::cout << "\nAverage time: " << (total_ms / NUM_RUNS) << " ms\n";
    return 0;
}

