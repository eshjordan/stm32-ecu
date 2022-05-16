#pragma once

extern "C" {

#include "linuxComms.h"
#include "stm32OpenampWrapper.h"
#include <stdint.h>
#include "Interproc_Msg.h"
}

class RpmsgManager {
public:
  static int send_ack(struct rpmsg_endpoint *ept);

  static int stm32ecu_recv_cb(struct rpmsg_endpoint *ept, void *data,
                              unsigned int len, uint32_t src, void *priv);

  static inline void stm32ecu_unbind_cb(struct rpmsg_endpoint *ept) {
    printf("stm32ecu rpmsg unbind\n");
  }

  static void create_endpoint(const char *channel, const uint32_t address);

  static uint8_t message_received;
  static Interproc_Msg_t interproc_msg;
  static struct rpmsg_endpoint rp_stm32ecu_endpoint;
};
