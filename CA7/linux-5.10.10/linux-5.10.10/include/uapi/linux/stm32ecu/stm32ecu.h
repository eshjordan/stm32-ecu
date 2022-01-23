#include <linux/ioctl.h>

#define STM32ECU_RESET _IO('k', 0)
#define STM32ECU_OFFLINE _IO('k', 1)
#define STM32ECU_GET_STATE _IOR('k', 2, int32_t *)
#define STM32ECU_SET_STATE _IOW('k', 3, int32_t *)
