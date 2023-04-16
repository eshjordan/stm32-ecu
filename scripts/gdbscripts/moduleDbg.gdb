
cd /home/jordan/stm32-ecu/CA7/linux-5.10.61/build

lx-symbols drivers/stm32ecu/stm32ecu.ko

b stm32ecu.c:stm32ecu_init
b stm32ecu.c:ioctl_call
# b stm32ecu.c:232
# b stm32ecu.c:272
# b stm32ecu.c:357
# b stm32ecu.c:363
# b stm32ecu.c:stm32ecu_exit
# b stm32ecu.c:mydrvr_ioctl
# b stm32ecu.c:device_open
# b stm32ecu.c:device_release
# b stm32ecu.c:device_read
# b stm32ecu.c:device_write
# b stm32ecu.c:rpmsg_sample_probe
# b stm32ecu.c:rpmsg_sample_remove
# b stm32ecu.c:rpmsg_sample_cb
