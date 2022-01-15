#ifndef _STM32_OPENAMP_WRAPPER_H
#define _STM32_OPENAMP_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stddef.h"
#include "openamp_conf.h"

struct rpmsg_endpoint;
struct rpmsg_device;

extern void rpmsg_destroy_ept(struct rpmsg_endpoint *ept);
static int rpmsg_send(struct rpmsg_endpoint *ept, const void *data, int len);

typedef void (*rpmsg_ns_bind_cb)(struct rpmsg_device *rdev, const char *name, uint32_t dest);
typedef int (*rpmsg_ept_cb)(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv);
typedef void (*rpmsg_ns_unbind_cb)(struct rpmsg_endpoint *ept);

#define OPENAMP_send  rpmsg_send
#define OPENAMP_destroy_ept rpmsg_destroy_ept

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Initialize the openamp framework*/
int w_MX_OPENAMP_Init(int RPMsgRole, rpmsg_ns_bind_cb ns_bind_cb);

/* Deinitialize the openamp framework*/
void w_OPENAMP_DeInit(void);

/* Create and register the endpoint */
int w_OPENAMP_create_endpoint(struct rpmsg_endpoint *ept, const char *name,
                            uint32_t dest, rpmsg_ept_cb cb,
							rpmsg_ns_unbind_cb unbind_cb);

/* Check for new rpmsg reception */
void w_OPENAMP_check_for_message(void);

/* Wait loop on endpoint ready ( message dest address is know)*/
void w_OPENAMP_Wait_EndPointready(struct rpmsg_endpoint *rp_ept);

//void stm32_log_dbg(const char *__restrict fmt, ...);
//void stm32_log_info(const char *__restrict fmt, ...);
//void stm32_log_warn(const char *__restrict fmt, ...);
//void stm32_log_err(const char *__restrict fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
