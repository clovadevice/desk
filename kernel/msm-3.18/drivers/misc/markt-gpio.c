#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/regulator/consumer.h>
#include <linux/string.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>

#define LDO_EN_GPIO 22
#define RESTE_GPIO 23

static int infogpio_gpio_probe(struct platform_device *pdev)
{
	printk("%s\n",__func__);
	
	gpio_request(LDO_EN_GPIO, "info-ldo-en");
	gpio_request(RESTE_GPIO, "info-reset");

	gpio_direction_output(LDO_EN_GPIO, 1);

	gpio_direction_output(RESTE_GPIO, 1);
	msleep(10);
	gpio_direction_output(RESTE_GPIO, 0);
	msleep(1);
	gpio_direction_output(RESTE_GPIO, 1);

	return 0;
}

static struct platform_driver info_plat_driver = {
	.driver = {
		.name = "markt-gpio",
		.owner = THIS_MODULE,
	},
	.probe = infogpio_gpio_probe,
};
static struct platform_device info_plat_device = {
	.name = "markt-gpio",
};

static int __init infomark_gpio_init(void)
{
	platform_device_register(&info_plat_device);
	return platform_driver_register(&info_plat_driver);
}
static void __exit infomark_gpio_exit(void)
{
	platform_driver_unregister(&info_plat_driver);
}
module_init(infomark_gpio_init);
module_exit(infomark_gpio_exit);
MODULE_DESCRIPTION("IR TEST");
MODULE_LICENSE("GPL v2");


