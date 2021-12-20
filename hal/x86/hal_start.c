/**********************************************************
        开始入口文件hal_start.c
***********************************************************
                彭东
**********************************************************/
#include "cosmostypes.h"
#include "cosmosmctrl.h"
//#include "halinc/halinit.h"

void hal_start()
{
    init_hal();
    init_krl();
    return;
}
