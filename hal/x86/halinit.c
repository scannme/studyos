
#include "cosmostypes.h"
#include "cosmosmctrl.h"

void init_hal()
{
    //初始化平台
    //初始化内存
    //初始化中断
    init_halplaltform();
    init_halmm();
    return;
}
