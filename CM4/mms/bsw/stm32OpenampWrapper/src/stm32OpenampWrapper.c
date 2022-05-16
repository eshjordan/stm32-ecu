#include "stm32OpenampWrapper.h"

#include "openamp.h"
#include "stdarg.h"
#include "stm32mp15xx_disco.h"
#include "main.h"

/* Initialize the openamp framework*/
int w_MX_OPENAMP_Init(int RPMsgRole, rpmsg_ns_bind_cb ns_bind_cb)
{
	return MX_OPENAMP_Init(RPMsgRole, ns_bind_cb);
}

/* Deinitialize the openamp framework*/
void w_OPENAMP_DeInit(void)
{
	OPENAMP_DeInit();
}

/* Create and register the endpoint */
int w_OPENAMP_create_endpoint(struct rpmsg_endpoint *ept, const char *name,
                            uint32_t dest, rpmsg_ept_cb cb,
							rpmsg_ns_unbind_cb unbind_cb)
{
	return OPENAMP_create_endpoint(ept, name, dest, cb, unbind_cb);
}

/* Check for new rpmsg reception */
void w_OPENAMP_check_for_message(void)
{
	OPENAMP_check_for_message();
}

/* Wait loop on endpoint ready ( message dest address is know)*/
void w_OPENAMP_Wait_EndPointready(struct rpmsg_endpoint *rp_ept)
{
	OPENAMP_Wait_EndPointready(rp_ept);
}

//void stm32_log_dbg(const char *__restrict fmt, ...)
//{
//#if LOGLEVEL >= LOGDBG
//	va_list a_list;
//	va_start(a_list, fmt);
//	log_dbg(fmt, a_list);
//#endif
//}
//
//void stm32_log_info(const char *__restrict fmt, ...)
//{
//#if LOGLEVEL >= LOGINFO
//	va_list a_list;
//	va_start(a_list, fmt);
////	log_info(fmt, a_list);
//	printf("[%05ld.%03ld][INFO ]", fmt, HAL_GetTick()/1000, HAL_GetTick() % 1000, a_list);
//#endif
//}
//
//void stm32_log_warn(const char *__restrict fmt, ...)
//{
//#if LOGLEVEL >= LOGWARN
//	va_list a_list;
//	va_start(a_list, fmt);
//	log_warn(fmt, a_list);
//#endif
//}
//
//void stm32_log_err(const char *__restrict fmt, ...)
//{
//#if LOGLEVEL >= LOGERR
//	va_list a_list;
//	va_start(a_list, fmt);
//	log_err(fmt, a_list);
//#endif
//}
