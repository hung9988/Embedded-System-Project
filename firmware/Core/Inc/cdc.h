#ifndef __CDC_H
#define __CDC_H

#include "keyboard.h"

void cdc_task(void);
void cdc_performance_measure(uint32_t started_at);
#endif /* __CDC_H */
