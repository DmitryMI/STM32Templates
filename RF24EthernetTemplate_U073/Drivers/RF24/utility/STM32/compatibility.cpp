#include "compatibility.h"

uint32_t rf24_get_time_us()
{
	uint32_t time = 1000 * HAL_GetTick() + 1000 - (SysTick->VAL / (SystemCoreClock / 1000000));
	return time;
};

void __usleep(int32_t usecs)
{
    uint32_t now = rf24_get_time_us();
    uint32_t blocked_until = now + usecs;
    while (blocked_until > rf24_get_time_us()) {
    }
};
