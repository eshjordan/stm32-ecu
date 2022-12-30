#include <linux/ioctl.h>
#include <linux/stm32ecu/shared/Interproc_Msg.h>

#define STM32ECU_RESET _IO('k', 0)
#define STM32ECU_OFFLINE _IO('k', 1)
#define STM32ECU_GET_STATE _IOR('k', 2, int32_t *)
#define STM32ECU_SET_STATE _IOW('k', 3, int32_t *)
#define STM32ECU_PING_RPROC _IO('k', 4)
#define STM32ECU_SEND_MSG _IOW('k', 5, Interproc_Msg_t *)
