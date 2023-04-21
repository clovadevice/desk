/*
 *
 * ATTINY1617  Touch Volume driver header file.
 *
 * Copyright (c) 2017  
 * Verison: V1.0
 *
 */
 
#ifndef __LINUX_ATTINY1717_TV_H__
#define __LINUX_ATTINY1717_TV_H__


struct attiny1617_tv_platform_data {
	const char *name;
	const char *fw_name;
	const char *chip_part_number;
	u32 irqflags;
	u32 irq_gpio;    //touch int
	u32 irq_gpio_flags;
	u32 reset_gpio;  //touch debug gpio 
	u32 reset_gpio_flags;
	u32 boot_mode_gpio;
	u32 boot_mode_gpio_flags;
	int (*power_init)(bool);
	int (*power_on)(bool);
};



#endif
