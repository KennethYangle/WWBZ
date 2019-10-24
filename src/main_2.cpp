#include "share_memory.h"

int main()
{
    double buffer[4] = {0};
    unsigned int length = sizeof(buffer);

    CShareMemory csm("obj", 1024);

    while (1)
    {
        csm.GetFromShareMem(buffer, length);
        for (int i = 0; i < 4; i++)
        {
            printf("%.2lf ", buffer[i]);
        }
        printf("\n");
    }
    return 0;
}