#include "main.h"
#include "cmsis_os.h"
#include "openamp.h"
#include "rpmsg_manage.h"

#define RPMSG_SERVICE_NAME "stm32mp1-rpmsg"
#define reply_str "CM4 RECV OK!"

static char received_data[64] = {0};
static uint8_t message_received = 0;
static struct rpmsg_endpoint rp_endpoint = {0};

static int rpmsg_recv_callback(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
  strncpy(received_data, (char*)data, len <= 64 ? len : 64);
  message_received=1;

  return 0;
}

void run_rpmsg(void const * argument)
{
	if(!IS_ENGINEERING_BOOT_MODE())
	{
		int status = OPENAMP_create_endpoint(&rp_endpoint, RPMSG_SERVICE_NAME, RPMSG_ADDR_ANY, rpmsg_recv_callback, NULL);

		if (status != RPMSG_SUCCESS)
		{
			Error_Handler();
		}

		while (1)
		{
			while (message_received == 0)
			{
				OPENAMP_check_for_message();
				osDelay(1);
			}
			message_received = 0;
			char rply[79] = {0};
			sprintf(rply, reply_str " - %s", received_data);
			status = OPENAMP_send(&rp_endpoint, rply, strlen(rply));

			if (status < 0)
			{
				Error_Handler();
			}

			osDelay(1);
		}
	}

	while (1)
	{
		osDelay(1);
	}
}
