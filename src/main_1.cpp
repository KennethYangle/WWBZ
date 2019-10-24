#include "share_memory.h"

int main()
{
    // u8 buffer[] = {0x11, 0x12, 0x13, 0x14, 0x15};
    double buffer[] = {1.11, 2.22, 3.33, 4.44, 5.55};
    u32 length = sizeof(buffer);
    printf("length: %ud\n", length);

    CShareMemory csm("txh", 1024);

    while (1)
    {
        for (int i = 0; i < 5; i++)
        {
            buffer[i] += 1;
        }

        csm.PutToShareMem(buffer, length);
        usleep(100000);
    }
    return 0;
}
