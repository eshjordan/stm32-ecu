
#include "rpmsgManager.hpp"

#define SHUTDOWN_CHANNEL "shutdown"
#define STM32ECU_CHANNEL "stm32ecu"

const char reply_str[] = "CM4 RECV OK!";

uint8_t RpmsgManager::message_received = 0;
Interproc_Msg_t RpmsgManager::interproc_msg = {};
struct rpmsg_endpoint RpmsgManager::rp_stm32ecu_endpoint = {};

int RpmsgManager::send_ack(struct rpmsg_endpoint *ept) {
  Interproc_Msg_t reply = interproc_msg_make(ACK_CMD, NULL);
  return OPENAMP_send(ept, &reply, sizeof(reply));
}

int RpmsgManager::stm32ecu_recv_cb(struct rpmsg_endpoint *ept, void *data,
                                   unsigned int len, uint32_t src, void *priv) {
  if (len == sizeof(Interproc_Msg_t) &&
      interproc_msg_check((Interproc_Msg_t *)data) > 0) {
    char rply[32] = {0};
    interproc_msg = *((Interproc_Msg_t *)data);

    switch (interproc_msg.command) {
    case PING_CMD: {
      RpmsgManager::send_ack(ept);
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
      RpmsgManager::send_ack(ept);
      break;
    }

    case STATUS_CMD:
    case SYNC_CMD:
    case ACK_CMD:
    default: {
      break;
    }
    }

    sprintf(rply, "%s - %d", reply_str, interproc_msg.command);
    OPENAMP_send(ept, rply, strlen(rply));

    message_received = 1;
  }

  return 0;
}

void RpmsgManager::create_endpoint(const char *channel,
                                   const uint32_t address) {
  int status = w_OPENAMP_create_endpoint(
      &RpmsgManager::rp_stm32ecu_endpoint, channel, address,
      RpmsgManager::stm32ecu_recv_cb, RpmsgManager::stm32ecu_unbind_cb);

  if (status != RPMSG_SUCCESS) {
    Error_Handler();
  }
}

extern "C" {

void runLinuxComms(void const *argument) {
  if (!IS_ENGINEERING_BOOT_MODE()) {
    osDelay(5000);
    RpmsgManager::create_endpoint(STM32ECU_CHANNEL, RPMSG_ADDR_ANY);

    while (1) {
      while (!RpmsgManager::message_received) {
        w_OPENAMP_check_for_message();

        osDelay(1);

        // HAL_WWDG_Refresh(&hwwdg1);
      }

      RpmsgManager::message_received = 0;

      osDelay(1);
    }
  }

  while (1) {
    osDelay(1);
  }
}

} // extern "C"
