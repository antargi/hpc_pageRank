#include <iostream>
#define restrict __restrict__

extern "C" {
    #include "bsp.h"
}

void bsp_main() {
    int pid = bsp_pid();
    int nprocs = bsp_nprocs();
    int tag;

    bsp_begin( bsp_nprocs() );

    int tagSize = sizeof(int);
    bsp_set_tagsize(&tagSize);

    if (pid == 0) {
        int mensaje = 42;
        for (int i = 1; i < nprocs; ++i) {
            bsp_send(i, &tag, &mensaje, sizeof(mensaje));
        }
    }

    bsp_sync();
    std::cout << "Proceso " << pid << std::endl;


    int status, currentTag;
    bsp_qsize(&status, &currentTag);

    if (pid != 0) {
        bsp_get_tag(&status, &currentTag);
        int recibido;
        bsp_move(&recibido, sizeof(int));
        std::cout << "Proceso " << pid << " recibió: " << recibido << std::endl;
    }
    bsp_end();
}

int main(int argc, char** argv) {
    void bsp_main();
    bsp_init(bsp_main, argc, argv);
    bsp_main();
    return 0;
}
