#include "share_memory.h"

int main()
{
    // u8 buffer[] = {0x11, 0x12, 0x13, 0x14, 0x15};
    double buffer[] = {320.0, 239.0, 0.0, 0.0};
    u32 length = sizeof(buffer);
    printf("length: %ud\n", length);

    CShareMemory csm("obj", 1024);

    int flag = 1;
    while (1)
    {
        if (buffer[0] >= 640)
            flag = -1;
        if (buffer[0] <= 0)
            flag = 1;
        buffer[0] += flag;

        csm.PutToShareMem(buffer, length);
        usleep(10000);
    }
    return 0;
}
