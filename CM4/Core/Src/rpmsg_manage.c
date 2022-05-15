#include "main.h"
#include "cmsis_os.h"
#include "openamp.h"
#include "openamp/rpmsg.h"
#include "rpmsg_manage.h"
#include "Interproc_Msg.h"

#define SHUTDOWN_CHANNEL "shutdown"
#define STM32ECU_CHANNEL "stm32ecu"

#define reply_str "CM4 RECV OK!"

extern osTimerId wdg_refresh_tmrHandle;
extern WWDG_HandleTypeDef hwwdg1;

static uint8_t message_received = 0;
static struct rpmsg_endpoint rp_stm32ecu_endpoint = { 0 };
static Interproc_Msg_t interproc_msg = { 0 };

static int send_ack(struct rpmsg_endpoint *ept)
{
	Interproc_Msg_t reply = interproc_msg_make(ACK_CMD, NULL);
	return OPENAMP_send(ept, &reply, sizeof(reply));
}

static int stm32ecu_recv_cb(struct rpmsg_endpoint *ept, void *data, size_t len,
			    uint32_t src, void *priv)
{
	if (len == sizeof(Interproc_Msg_t) && interproc_msg_check(data) > 0) {
		char rply[128] = { 0 };
		interproc_msg = *((Interproc_Msg_t *)data);

		switch (interproc_msg.command) {
		case PING_CMD: {
			send_ack(ept);
			break;
		}

		case PARAM_GET_CMD: {
			break;
		}

		case PARAM_SET_CMD: {
			break;
		}

		case FIRMWARE_UPDATE_CMD:
		case PROGRAM_UPDATE_CMD:
		case VALUE_CMD:
		case UNKNOWN_CMD:
		case ECHO_CMD: {
			break;
		}

		case RESET_CMD: {
			send_ack(ept);
			break;
		}

		case STATUS_CMD:
		case SYNC_CMD:
		case ACK_CMD:
		default: {
			break;
		}
		}

		sprintf(rply, reply_str " - %d", interproc_msg.command);
		OPENAMP_send(ept, rply, strlen(rply));

		message_received = 1;
	}

	return 0;
}

static void stm32ecu_unbind_cb(struct rpmsg_endpoint *ept)
{
	printf("stm32ecu rpmsg unbind\n");
}

void run_rpmsg(void const *argument)
{
	if (!IS_ENGINEERING_BOOT_MODE()) {
		osDelay(5000);
		int status = OPENAMP_create_endpoint(
			&rp_stm32ecu_endpoint, STM32ECU_CHANNEL, RPMSG_ADDR_ANY,
			stm32ecu_recv_cb, stm32ecu_unbind_cb);

		if (status != RPMSG_SUCCESS) {
			Error_Handler();
		}

		while (1) {
			while (!message_received) {
				OPENAMP_check_for_message();

				osDelay(1);

				// HAL_WWDG_Refresh(&hwwdg1);
			}

			message_received = 0;

			osDelay(1);
		}
	}

	while (1) {
		osDelay(1);
	}
}
