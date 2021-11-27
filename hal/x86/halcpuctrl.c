#include "cosmostypes.h"
#include "cosmosmctrl.h"



void hal_sysdie(char_t * errmsg)
{
    kprint("cosmos system is die %s", errmsg);
    for (;;)
        ;
    return;
}
void system_error(char_t *errmsg)
{
    hal_sysdie(errmsg);
    return;
}

#pragma GCC push options
#pragma GCC optimize("O0")

void die(u32_t dt)
{
    u32_t dttt =dt, dtt =dt;
    if (dt == 0) 
    {
        for(;;)
            ;
    }

    for(u32_t i = 0; i < dt;  i++) {
        for (u32_t j = 0; j < dtt; j++){
            for (u32_t k= 0; k < dttt; k++) {
                ;
            }
        }
    }

    return;
}
