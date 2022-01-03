# v6.0
####################################################################
######################## debug config. #############################
####################################################################
# Set debug phase:
#	1: Attach at TF-A BL2
#	2: Attach at TF-A BL32 or OP-TEE
#	3: Attach at SSBL (U-Boot)
#	4: Attach at Linux kernel
set $debug_phase = 4

# Set debug mode:
#	0: Attach at boot
#	1: Attach running target
set $debug_mode = 1

# Set debug trusted bootchain:
#	0: TF-A BL2 / TF-A BL32 / U-Boot / Linux kernel
#	1: TF-A BL2 / OP-TEE / U-Boot / Linux kernel
set $debug_trusted_bootchain = 1

####################################################################

# Force reset mode in case where debug_phase is 1(TF-A BL2) or 2(TF-A BL32 or OP-TEE)
if $debug_phase <= 2
    set $debug_mode = 0
end

# Set environment configuration
source Path_env.py

define symadd_bl32
    if $debug_trusted_bootchain == 0
        python gdb.execute("add-symbol-file " + path_elf_tf_a_bl32 + " -s ro " + bl32_load_addr)
    else
        python gdb.execute("add-symbol-file " + path_elf_op_tee_bl32)
    end
end
document symadd_bl32
    Load BL32 symbols on top of current symbols
end

define symload_bl2
    python gdb.execute("symbol-file " + path_elf_tf_a_bl2)
end
document symload_bl2
    Load BL2 symbols
end

define symload_uboot
    python gdb.execute("symbol-file " + path_elf_u_boot)
end
document symload_uboot
    Load U-Boot symbols
end

define symadd_uboot
    set $offset_gdb = ((gd_t *)$r9)->relocaddr
    python offset_py = gdb.convenience_variable("offset_gdb")
    python gdb.execute("add-symbol-file " + path_elf_u_boot + " " + str(offset_py))
    python gdb.set_convenience_variable("offset_gdb", None)
end
document symadd_uboot
    Load U-Boot relocated symbols on top of current symbols
end

define symload_vmlinux
    python gdb.execute("symbol-file " + path_elf_vmlinux)
end
document symload_vmlinux
    Load Linux symbols
end

####################################################################
########################## functions ###############################
####################################################################
define break_boot_bl32
	if $debug_trusted_bootchain == 0
		thbreak sp_min_entrypoint
	else
		thbreak entry_a32.S:_start
	end
	c
end

define break_boot_uboot
	thbreak vectors.S:_start
	c
end

define break_boot_linuxkernel
	thbreak stext
	c
end

define mem_enable
	mem auto
end

define mem_disable
	mem 0 4 ro 8 nocache
end
####################################################################


######################## common config. ############################
# Disables confirmation requests
set confirm off
set mem inaccessible-by-default
mem_disable
# set debug remote 1

# Connection to the host gdbserver port for Cortex-A7 SMP
target extended-remote localhost:3333

# Configure GDB for OpenOCD
set remote hardware-breakpoint-limit 6
set remote hardware-watchpoint-limit 4

# Switch to Core0, no SMP for the moment. We'll re-enable it in kernel
monitor cortex_a smp off
monitor targets stm32mp15x.cpu0

####################################################################

# Reset the system and halt in bootrom to attach at boot
if $debug_mode == 0
	symload_bl2

	monitor reset halt
	monitor gdb_sync
	stepi
	monitor if {[stm32mp15x.cpu1 curstate] == "halted"} {targets stm32mp15x.cpu1;resume;targets stm32mp15x.cpu0}

	# we halt at tf-a entry point. Nothing to do for $debug_phase == 1

	# Stop at TF-A BL32 or OP-TEE entry point
	if $debug_phase == 2
		symadd_bl32
		break_boot_bl32
	end

	# Stop at U-Boot entry point
	if $debug_phase == 3
		symload_uboot
		break_boot_uboot
	end

	# Stop at Linux kernel entry point
	if $debug_phase == 4
		symload_vmlinux
		break_boot_linuxkernel
		# in kernel re-enable SMP
		monitor cortex_a smp on
	end
end

# Target state is now aligned with gdb, enable memory read
mem_enable

####################################################################

# Set hardware breakpoint mode for TF-A OP-TEE and U-Boot
if $debug_phase <= 3
	monitor gdb_breakpoint_override hard
end

####################################################################

# Attach running target
if $debug_mode == 1
	# Halt in U-Boot
	if $debug_phase == 3
		monitor if {[stm32mp15x.cpu1 curstate] == "halted"} {targets stm32mp15x.cpu1;resume;targets stm32mp15x.cpu0}
		symload_uboot
		# Relocate U-Boot symbols
		symadd_uboot
		symadd_bl32
	end

	# Halt in Linux kernel
	if $debug_phase == 4
		symload_vmlinux
		symadd_bl32
		# in kernel re-enable SMP
		monitor cortex_a smp on
	end
	monitor gdb_sync
	stepi
end

add-auto-load-safe-path /home/jordan/Documents/2021/stm32-ecu/CA7/linux-5.10.10/build
source /home/jordan/Documents/2021/stm32-ecu/CA7/linux-5.10.10/build/vmlinux-gdb.py
cd /home/jordan/Documents/2021/stm32-ecu/CA7/linux-5.10.10/build
b kernel/module.c:3901
directory ../linux-5.10.10/drivers/stm32ecu/

# continue
# source /home/jordan/Documents/2021/stm32-ecu/CA7/gdbscripts/moduleDbg.gdb
