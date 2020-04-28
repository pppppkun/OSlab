#include "lib.h"
#include "types.h"

int uEntry(void)
{
    int dec = 0;
    int hex = 0;
    char str[6];
    char cha = 0;
    int ret = 0;
    while(1)
    {
        printf("Input:\" Test %%c Test %%6s %%d %%x\"\n");
        ret = scanf("Test %c Test %6s %d %x", &cha, str, &dec, &hex);
        printf("Ret: %d; %c; %s; %d, %x.\n", ret, cha, str, dec, hex);
        if(ret == 4) 
            break;
    }
    return 0;
}