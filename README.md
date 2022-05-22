
# Download the stm32-ecu project

 - cd ~/
 - git clone git@github.com:eshjordan/stm32-ecu.git
 - cd stm32-ecu
 - git submodule update --init --recursive


## Setup and flash the STM32 Starter Pack

Reference: https://wiki.st.com/stm32mpu/wiki/STM32MP15_Discovery_kits_-_Starter_Package

Create an account at https://www.st.com and log in. The site can be very difficult to use, it will often refuse to log you in, or you can be logged in in one tab, but not in another. Try a different browser, clearing cache, restarting, etc. if it does this.

Download STM32CubeProgrammer
 - https://www.st.com/en/development-tools/stm32cubeprog.html

Unzip
 - cd ~/Downloads
 - unzip en.stm32cubeprg-lin_v2-10-0_v2.10.0.zip -d STM32CubeProgrammer

Execute the Linux installer, which guides you through the installation process
 - cd ./STM32CubeProgrammer
 - ./SetupSTM32CubeProgrammer-2.9.0.linux

Leave the default path:
 - /home/${USER}/STMicroelectronics/STM32Cube/STM32CubeProgrammer

Add to PATH:
 - echo "export PATH=/home/${USER}/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin:\${PATH}" >> ~/.bashrc
 - source ~/.bashrc

Install drivers to connect to the board via USB
 - sudo apt update && sudo apt install -y libusb-1.0-0
 - cd ~/STMicroelectronics/STM32Cube/STM32CubeProgrammer/Drivers/rules
 - sudo cp *.* /etc/udev/rules.d/


Setup STM32MP15 Ecosystem structure
 - mkdir -p ~/STM32MP15-Ecosystem-v3.1.0
 - cd ~/STM32MP15-Ecosystem-v3.1.0
 - mkdir -p Starter-Package Developer-Package Distribution-Package

Download Starter Pack image and flash to the board:

 - Log into https://www.st.com before downloading
 - https://st.com/content/ccc/resource/technical/software/firmware/group1/c0/ec/ec/3b/4d/54/4e/a9/STM32MP15_OpenSTLinux_Starter_Package/files/FLASH-stm32mp1-openstlinux-5-10-dunfell-mp1-21-11-17_tar.xz/_jcr_content/translations/en.FLASH-stm32mp1-openstlinux-5-10-dunfell-mp1-21-11-17_tar.xz

 - cd ~/STM32MP15-Ecosystem-v3.1.0/Starter-Package
 - tar xvf ~/Downloads/en.FLASH-stm32mp1-openstlinux-5-10-dunfell-mp1-21-11-17_tar_v3.1.0.xz

The trusted boot chain is the default solution delivered by STMicroelectronics. Thus, the steps below use the image for the trusted boot chain

ENSURE YOU CONNECT THE OTG USB-C TO EITHER USB-C OR USB-3 ON THE HOST PC!! FLASHING WILL FAIL WITHOUT THIS!!
Let's flash the downloaded image on the microSD card:
  - Set both boot switches (1) to the off position
  - Connect the USB Type-C™ (OTG) port (2) to the host PC that contains the downloaded image
  - Insert the delivered microSD card into the dedicated slot (3)
  - Connect the delivered power supply to the USB Type-C™ port (4)
  - Press the reset button (5) to reset the board


 - cd ~/STM32MP15-Ecosystem-v3.1.0/Starter-Package/stm32mp1-openstlinux-5.10-dunfell-mp1-21-11-17/images/stm32mp1
 - STM32_Programmer_CLI -l usb
 - Read the output of the last command, and replace port=<USB_NUM> in the following command with the correct port. E.g, USB1:
   - STM32_Programmer_CLI -c port=usb1 -w flashlayout_st-image-weston/extensible/FlashLayout_sdcard_stm32mp157d-dk1-extensible.tsv

 - Flashing may take some time, ~5+ min
 - Check it flashed correctly, look for `Flashing service completed successfully` in the output

 - Disconnect the power from the board
 - Set both boot switches to the on position
 - Connect the power to the board
 - Wait a few minutes, the kernel is resizing all of the filesystems

 - In the meantime:
 - Download minicom (or another serial terminal program)
 - sudo apt update && sudo apt install -y minicom
 - Connect the ST-LINK/V2-1 USB micro-B port to the host PC

Attach a terminal to the board. One of the following commands should work:
 - minicom -D /dev/ttyACM0
 - minicom -D /dev/ttyS0

 - When the connection is open, hit the reset button on the board, you should see the boot log from the device
 - If the log mentions resizing the filesystem, wait for the terminal to be ready, might take up to 5 mins
 - Look for `root@stm32mp1:~#` to know it's finished
 - To exit minicom, press `CTRL-A`, then `X`, then press enter

If you want to connect over ethernet in future, use this serial connection to get the device IP address
 - Open a serial connection
 - Connect the board to ethernet
 - Press the reset button on the board
 - Wait for the terminal to become available
 - Run `ifconfig` to grab the IP address
 - You can close the serial connection and SSH in like usual. Probably a good idea to set a static IP address for it
 - E.g. `ssh root@192.168.0.4`



## Setup and flash the STM32 Developer Pack

# Setup the Developer Pack SDK
Ensure the Starter Pack is flashed to the board first

 - Ensure you are already logged into https://st.com
 - Download the STM32MP1 OpenSTLinux Developer Package (Source) and Yocto SDK from:
 - https://st.com/content/ccc/resource/technical/software/sw_development_suite/group0/63/94/71/38/24/c4/4f/7f/stm32mp1dev_yocto_sdk/files/SDK-x86_64-stm32mp1-openstlinux-5.10-dunfell-mp1-21-11-17.tar.xz/_jcr_content/translations/en.SDK-x86_64-stm32mp1-openstlinux-5.10-dunfell-mp1-21-11-17.tar.xz

 - cd ~/Downloads

 - tar xvf ~/Downloads/en.SDK-x86_64-stm32mp1-openstlinux-5.10-dunfell-mp1-21-11-17.tar_v3.1.0.xz

 - chmod +x stm32mp1-openstlinux-5.10-dunfell-mp1-21-11-17/sdk/st-image-weston-openstlinux-weston-stm32mp1-x86_64-toolchain-3.1.11-openstlinux-5.10-dunfell-mp1-21-11-17.sh

 - ./stm32mp1-openstlinux-5.10-dunfell-mp1-21-11-17/sdk/st-image-weston-openstlinux-weston-stm32mp1-x86_64-toolchain-3.1.11-openstlinux-5.10-dunfell-mp1-21-11-17.sh -d ~/STM32MP15-Ecosystem-v3.1.0/Developer-Package/SDK

Setup an alias `get_stm` to source the cross-compilation environment:
 - echo "alias get_stm=\\"unset LD_LIBRARY_PATH; source ~/STM32MP15-Ecosystem-v3.1.0/Developer-Package/SDK/environment-setup-cortexa7t2hf-neon-vfpv4-ostl-linux-gnueabi\\"" >> ~/.bashrc

 - source ~/.bashrc

 - get_stm

 - Running `echo $ARCH` should return `arm`
 - Running `echo $CROSS_COMPILE` should return `arm-ostl-linux-gnueabi-`
 - Running `$CC --version` should return `arm-ostl-linux-gnueabi-gcc (GCC) 9.3.0`
 - Running `echo $OECORE_SDK_VERSION` should return `3.1.11-openstlinux-5.10-dunfell-mp1-21-11-17`
 

# Setup the Linux kernel
Ensure you downloaded the STM32MP1 OpenSTLinux Developer Package (Source) from https://www.st.com earlier
 - https://st.com/content/ccc/resource/technical/software/sw_development_suite/group0/63/94/71/38/24/c4/4f/7f/stm32mp1dev_yocto_sdk/files/SDK-x86_64-stm32mp1-openstlinux-5.10-dunfell-mp1-21-11-17.tar.xz/_jcr_content/translations/en.SDK-x86_64-stm32mp1-openstlinux-5.10-dunfell-mp1-21-11-17.tar.xz

 - cd ~/STM32MP15-Ecosystem-v3.1.0/Developer-Package
 - tar xvf ~/Downloads/en.SOURCES-stm32mp1-openstlinux-5.10-dunfell-mp1-21-11-17_tar_v3.1.0.xz

 - cd stm32mp1-openstlinux-5.10-dunfell-mp1-21-11-17/sources/arm-ostl-linux-gnueabi/linux-stm32mp-5.10.61-stm32mp-r2-r0

 - tar xvf linux-5.10.61.tar.xz


# Setup the U-Boot

- Ensure you are already logged into https://st.com
 - Download the STM32MP1 OpenSTLinux U-Boot from:
 - https://st.com/content/ccc/resource/technical/sw-updater/firmware2/group0/ce/a3/5c/9e/0d/55/4c/43/stm32cube_Standard_A7_BSP_components_u-boot/files/SOURCES-u-boot-stm32mp1-openstlinux-5-10-dunfell-mp1-21-11-17_tar.xz/_jcr_content/translations/en.SOURCES-u-boot-stm32mp1-openstlinux-5-10-dunfell-mp1-21-11-17_tar.xz

 - cd ~/STM32MP15-Ecosystem-v3.1.0/Developer-Package
 - tar xvf ~/Downloads/en.SOURCES-u-boot-stm32mp1-openstlinux-5-10-dunfell-mp1-21-11-17_tar.xz

 - cd stm32mp1-openstlinux-5.10-dunfell-mp1-21-11-17/sources/arm-ostl-linux-gnueabi/u-boot-stm32mp-v2020.10-stm32mp-r2-r0

 - tar xvf u-boot-stm32mp-v2020.10-stm32mp-r2-r0.tar.gz


# Setup the TF-A

- Ensure you are already logged into https://st.com
 - Download the STM32MP1 OpenSTLinux TF-A from:
 - https://st.com/content/ccc/resource/technical/sw-updater/firmware2/group0/2b/04/61/eb/4c/83/4d/3d/stm32cube_standard_a7_bsp_components_tf_a/files/SOURCES-tf-a-stm32mp1-openstlinux-5-10-dunfell-mp1-21-11-17_tar.xz/_jcr_content/translations/en.SOURCES-tf-a-stm32mp1-openstlinux-5-10-dunfell-mp1-21-11-17_tar.xz

 - cd ~/STM32MP15-Ecosystem-v3.1.0/Developer-Package

 - tar xvf ~/Downloads/en.SOURCES-tf-a-stm32mp1-openstlinux-5-10-dunfell-mp1-21-11-17_tar.xz
 - cd stm32mp1-openstlinux-5.10-dunfell-mp1-21-11-17/sources/arm-ostl-linux-gnueabi/tf-a-stm32mp-v2.4-stm32mp-r2-r0

 - tar xvf tf-a-stm32mp-v2.4-stm32mp-r2-r0.tar.gz


# Setup the OP-TEE

- Ensure you are already logged into https://st.com
 - Download the STM32MP1 OpenSTLinux OP-TEE from:
 - https://st.com/content/ccc/resource/technical/sw-updater/firmware2/group0/68/5a/bc/bf/bd/48/46/22/stm32cube_Standard_A7_BSP_components_optee/files/SOURCES-optee-stm32mp1-openstlinux-5.10-dunfell-mp1-21-11-17.tar.xz/_jcr_content/translations/en.SOURCES-optee-stm32mp1-openstlinux-5.10-dunfell-mp1-21-11-17.tar.xz

 - cd ~/STM32MP15-Ecosystem-v3.1.0/Developer-Package

 - tar xvf ~/Downloads/en.SOURCES-optee-stm32mp1-openstlinux-5.10-dunfell-mp1-21-11-17.tar.xz
 - cd stm32mp1-openstlinux-5.10-dunfell-mp1-21-11-17/sources/arm-ostl-linux-gnueabi/optee-os-stm32mp-3.12.0-stm32mp-r2-r0

 - tar xvf optee-os-stm32mp-3.12.0-stm32mp-r2-r0.tar.gz

# Setup debug symbols

- Ensure you are already logged into https://st.com
 - Download the STM32MP1 OpenSTLinux Debug Symbols from:
 - https://st.com/content/ccc/resource/technical/sw-updater/firmware2/group0/83/29/25/6d/37/df/4c/58/stm32cube_Standard_A7_BSP_components_Debug/files/DEBUG-stm32mp1-openstlinux-5-10-dunfell-mp1-21-11-17_tar.xz/_jcr_content/translations/en.DEBUG-stm32mp1-openstlinux-5-10-dunfell-mp1-21-11-17_tar.xz


 - cd ~/STM32MP15-Ecosystem-v3.1.0/Developer-Package

 - tar xvf ~/Downloads/en.DEBUG-stm32mp1-openstlinux-5-10-dunfell-mp1-21-11-17_tar.xz


# Installing STM32CubeIDE

- Ensure you are already logged into https://st.com
 - Download the STM32CubeIDE from:
 - https://www.st.com/en/development-tools/stm32cubeide.html

 - unzip ~/Downloads/en.st-stm32cubeide_1.9.0_12015_20220302_0855_amd64.deb_bundle.sh_v1.9.0.zip -d ~/Downloads/

 - chmod +x ~/Downloads/st-stm32cubeide_1.9.0_12015_20220302_0855_amd64.deb_bundle.sh
 - sudo ~/Downloads/st-stm32cubeide_1.9.0_12015_20220302_0855_amd64.deb_bundle.sh


# Installing STM32Cube MPU Package

- Ensure you are already logged into https://st.com
 - Download the STM32Cube MPU Package from:
 - https://st.com/content/ccc/resource/technical/software/firmware/group1/c9/79/b4/34/35/93/43/55/stm32cubemp1/files/STM32Cube_FW_MP1_V1-5-0.zip/_jcr_content/translations/en.STM32Cube_FW_MP1_V1-5-0.zip

 - cd ~/STM32MP15-Ecosystem-v3.1.0/Developer-Package

 - unzip ~/Downloads/en.STM32Cube_FW_MP1_V1-5-0_v1.5.0.zip -d ~/STM32MP15-Ecosystem-v3.1.0/Developer-Package


# Setting up the project
Start STM32CubeIDE for the first time
 - Leave the default workspace directory:
 - ${HOME}/STM32CubeIDE/workspace_1.9.0
 - Import the stm32-ecu project and relevant sub-projects
 - sudo apt update && sudo apt install -y bear

# ESP32
Install the ESP-IDF extension:
Id: espressif.esp-idf-extension
VS Marketplace Link: https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension

`echo "alias get_idf='. $HOME/esp/esp-idf/export.sh'" >> ~/.bashrc`

`sudo cp ${HOME}/.espressif/tools/openocd-esp32/v0.11.0-esp32-20211220/openocd-esp32/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d`
