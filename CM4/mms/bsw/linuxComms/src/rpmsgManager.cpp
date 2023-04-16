
#include "rpmsgManager.hpp"
#include "System.hpp"
#include "string.h"

#define SHUTDOWN_CHANNEL "shutdown"
#define STM32ECU_CHANNEL "stm32ecu"

const char reply_str[] = "CM4 RECV OK!";

uint8_t RpmsgManager::message_received = 0;
Interproc_Msg_t RpmsgManager::interproc_msg = {};
struct rpmsg_endpoint RpmsgManager::rp_stm32ecu_endpoint = {};

int RpmsgManager::send_ack(struct rpmsg_endpoint *ept) {
  Interproc_Msg_t reply = interproc_msg_make(CMD_ACK, NULL, 0);
  return OPENAMP_send(ept, &reply, sizeof(reply));
}

int RpmsgManager::stm32ecu_recv_cb(struct rpmsg_endpoint *ept, void *data,
                                   unsigned int len, uint32_t src, void *priv) {
  if (len == sizeof(Interproc_Msg_t) &&
      interproc_msg_check((Interproc_Msg_t *)data) > 0) {
    char rply[32] = {0};
    interproc_msg = *((Interproc_Msg_t *)data);

    switch (interproc_msg.command) {
    case CMD_PING: {
      RpmsgManager::send_ack(ept);
      break;
    }

    case CMD_PARAM_GET: {
      auto param_type = *(System::Impl::ParameterType*)interproc_msg.data;
      char param_name[12] = {0};
      strncpy(param_name, (const char*)interproc_msg.data+1, 10);
      switch(param_type)
      {
        case System::Impl::ParameterType::PARAMETER_BOOL: {
          bool param_value = System::get_parameter_value<bool>(param_name);
          Interproc_Msg_t reply = {.command = CMD_ACK};
          *(bool*)reply.data = param_value;
          interproc_msg_calc_checksum(&reply);
          OPENAMP_send(ept, &reply, sizeof(reply));
          break;
        }
        case System::Impl::ParameterType::PARAMETER_INTEGER: {
          int64_t param_value = System::get_parameter_value<int64_t>(param_name);
          Interproc_Msg_t reply = {.command = CMD_ACK};
          *(int64_t*)reply.data = param_value;
          interproc_msg_calc_checksum(&reply);
          OPENAMP_send(ept, &reply, sizeof(reply));
          break;
        }
        case System::Impl::ParameterType::PARAMETER_DOUBLE: {
          double param_value = System::get_parameter_value<double>(param_name);
          Interproc_Msg_t reply = {.command = CMD_ACK};
          *(double*)reply.data = param_value;
          interproc_msg_calc_checksum(&reply);
          OPENAMP_send(ept, &reply, sizeof(reply));
          break;
        }
        case System::Impl::ParameterType::PARAMETER_STRING: {
          const char* param_value = System::get_parameter_value<System::Impl::ParameterType::PARAMETER_STRING>(param_name);
          Interproc_Msg_t reply = {.command = CMD_ACK};
          strncpy((char*)reply.data, param_value, 11);
          interproc_msg_calc_checksum(&reply);
          OPENAMP_send(ept, &reply, sizeof(reply));
          break;
        }
        case System::Impl::ParameterType::PARAMETER_BYTE_ARRAY: {
          const uint8_t *param_value = System::get_parameter_value<const uint8_t*>(param_name);
          Interproc_Msg_t reply = {.command = CMD_ACK};
          memcpy(reply.data, param_value, 11);
          interproc_msg_calc_checksum(&reply);
          OPENAMP_send(ept, &reply, sizeof(reply));
          break;
        }
        case System::Impl::ParameterType::PARAMETER_NOT_SET:
        default:
        {
        	Interproc_Msg_t reply = {.command = Interproc_Command_t::CMD_UNKNOWN};
        	strncpy((char*)reply.data, "E_PARAMT", 11);
        	interproc_msg_calc_checksum(&reply);
        	OPENAMP_send(ept, &reply, sizeof(reply));
        	break;
        }
      }
      break;
    }

    case CMD_PARAM_SET: {
      break;
    }

    case CMD_FIRMWARE_UPDATE:
    case CMD_PROGRAM_UPDATE:
    case CMD_VALUE:
    case CMD_UNKNOWN:
    case CMD_ECHO: {
      break;
    }

    case CMD_RESET: {
      RpmsgManager::send_ack(ept);
      break;
    }

    case CMD_STATUS:
    case CMD_SYNC:
    case CMD_ACK:
    default: {
      break;
    }
    }

//    sprintf(rply, "%s - %d", reply_str, interproc_msg.command);
//    OPENAMP_send(ept, rply, strlen(rply));

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

void runLinuxComms(void *argument) {
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
