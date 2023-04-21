#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/leds.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/pinctrl/consumer.h>
#include <linux/err.h>

#include "ledanim.h"
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/spinlock.h>
#include <linux/ctype.h>


#define LED_PIN_COUNT 2

#define IR_SENSOR_LD0 22
#define LED_GPIO_1 120
#define LED_GPIO_2 122

#define LED_PWR_DEV_MAJOR 248

#define DEBUG 1

int IR_GPIO = IR_SENSOR_LD0;
int LED1_GPIO = LED_GPIO_1;
int LED2_GPIO = LED_GPIO_2;

enum {
	LED_1 = 0,
	LED_2,
};


struct gpio_led_priv {
	struct device *dev;

	int gpios[LED_PIN_COUNT];
	unsigned char led_brightness[LED_PIN_COUNT];
	unsigned char led_brightness_old[LED_PIN_COUNT];
};


static int gpio_led_get_led_brightness(struct device *dev, int index, unsigned char * value)
{	
	struct gpio_led_priv * priv = dev_get_drvdata(dev);
	if(index < 0 || index >= LED_PIN_COUNT)
		return -1;
	*value = priv->led_brightness[index];
	return 0;
}


static int gpio_led_set_led_brightness(struct device *dev, int index, unsigned char value)
{	
	struct gpio_led_priv * priv = dev_get_drvdata(dev);
	if(index < 0 || index >= LED_PIN_COUNT)
		return -1;

	priv->led_brightness[index] = value ? 1 : 0;
	return 0;
}

static int gpio_led_led_update(struct device *dev)
{
	int i;
	struct gpio_led_priv * priv = dev_get_drvdata(dev);

	for(i = 0 ; i < LED_PIN_COUNT; i++){
		if(priv->led_brightness_old[i] != priv->led_brightness[i]){
			gpio_direction_output(priv->gpios[i], priv->led_brightness[i]);
			priv->led_brightness_old[i] = priv->led_brightness[i];
		}
	}
	return 0;
}


static int gpio_led_led_power_enable(struct device *dev, int enable)
{
	return 0;
}

static int gpio_led_get_max_current(struct device *dev, unsigned char *value)
{
	return 0;
}

static int gpio_led_set_max_current(struct device *dev, unsigned char value)
{
	return 0;
}


static struct leddrv_func leddrv_ops = {
	.num_leds = LED_PIN_COUNT,
	.get_led_brightness = gpio_led_get_led_brightness,
	.set_led_brightness = gpio_led_set_led_brightness,
	.update = gpio_led_led_update,
	.power_enable = gpio_led_led_power_enable,
	.get_max_current = gpio_led_get_max_current,
	.set_max_current = gpio_led_set_max_current
};

static struct gpio_led_priv *gpio_led_create_of(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct gpio_led_priv *priv;	
	char name[24] = {0,};
	int i, rc;	

	priv = devm_kzalloc(&pdev->dev, sizeof(struct gpio_led_priv), GFP_KERNEL);
	if (!priv)
		return ERR_PTR(-ENOMEM);

	priv->dev = &pdev->dev;
	priv->gpios[LED_1] = of_get_named_gpio(np, "infr,gpio-led_1", 0);
	priv->gpios[LED_2] = of_get_named_gpio(np, "infr,gpio-led_2", 0);

	
	for(rc = 0, i = 0 ; i < LED_PIN_COUNT; i++){
		if(priv->gpios[i] == 0 || !gpio_is_valid(priv->gpios[i])){
			rc = 1; break;
		}
	}
	if(rc){
		kfree(priv);
		return ERR_PTR(-ENODEV);
	}

	for(i = 0 ; i < LED_PIN_COUNT; i++){
		snprintf(name, sizeof(name) - 1, "gpio-led-%d", i);
		rc = gpio_request(priv->gpios[i], name);
		if(rc){
			dev_err(priv->dev, "request gpio-led-%d failed, rc=%d\n", i, rc);
			for(--i; i >= 0; i--){
				gpio_free(priv->gpios[i]);
				priv->gpios[i] = 0;
			}			
			break;
		}
	}
	if(rc){
		kfree(priv);
		return ERR_PTR(-ENODEV);
	}

	return priv;
}

static int gpio_led_probe(struct platform_device *pdev)
{
	struct gpio_led_priv *priv;
	struct pinctrl *pinctrl;

	pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
	if (IS_ERR(pinctrl))
		dev_warn(&pdev->dev, "devm_pinctrl_get_select_default failed\n");

	priv = gpio_led_create_of(pdev);
	if (IS_ERR(priv))
		return PTR_ERR(priv);


	platform_set_drvdata(pdev, priv);

	gpio_direction_output(IR_GPIO,0);
	gpio_set_value(IR_GPIO,1);

	gpio_led_set_led_brightness(&pdev->dev,1,1);
	gpio_led_set_led_brightness(&pdev->dev,0,1);
	gpio_led_led_update(&pdev->dev);
	leddrv_ops.dev = priv->dev;	
	#if 0
	if(register_backled_leddrv_func(&leddrv_ops) != 0) {
		dev_err(priv->dev, "register_leddrv_func() failed\n"); 
	}
	#endif
	return 0;
}

static int gpio_led_remove(struct platform_device *pdev)
{
	struct gpio_led_priv *priv = platform_get_drvdata(pdev);
	if(priv){

 		gpio_direction_output(IR_GPIO,0);
		gpio_set_value(IR_GPIO,0);

		gpio_led_set_led_brightness(&pdev->dev,1,0);
		gpio_led_set_led_brightness(&pdev->dev,0,0);
		gpio_led_led_update(&pdev->dev);
		
		dev_warn(&pdev->dev, "System Off IR/LED OFF\n");
		#if 0
		int i;
		for(i = 0 ; i < LED_PIN_COUNT; i++){
			if(priv->gpios[i] != 0){
				gpio_free(priv->gpios[i]);
				priv->gpios[i] = 0;
			}
		}
		#endif
		kfree(priv);
		priv = NULL;
	}
	platform_set_drvdata(pdev, NULL);
	return 0;
}


static const struct of_device_id of_gpio_led_match[] = {
	{ .compatible = "infr,gpio-led", },
	{},
};

//static int g_priority = LED_DEFAULT;

static long ledgpio_pwr_ioctl(struct file *file, unsigned int req, unsigned long arg)
{
 
	//int err = -EBADRQC;
	//int prio = LED_DEFAULT;
	//int ret = 0;


	//struct led_priority_t *led_p = file->private_data;

#ifdef DEBUG
	//dev_err("%s: ioctl: req 0x%x\n", __func__, req);
#endif

	switch (req) {
	case LED_GET_BRIGHTNESS:
	//	err = ledanim_ioctl_get_brightness(arg);
		break;
	case LED_SET_BRIGHTNESS:
	//	err = ledanim_ioctl_set_brightness(arg);
		printk("%s: LED_SET_BRIGHTNESS\n", __func__);

		gpio_direction_output(LED_GPIO_1,0);
		gpio_direction_output(LED_GPIO_2,0);

		gpio_set_value(LED_GPIO_1,arg);
		gpio_set_value(LED_GPIO_2,arg);

		break;
	}

	return 0;
}


static int ledgpio_open(struct inode *inode, struct file *file) {
	struct led_priority_t *led_p = NULL;
	led_p = kzalloc(sizeof(struct led_priority_t), GFP_KERNEL);
	led_p->priority = LED_DEFAULT;
	file->private_data = led_p;
	return 0;
}

static int ledgpio_release(struct inode *inode, struct file *file) {

	struct led_priority_t *led_p = file->private_data;
	file->private_data = NULL;
	kfree(led_p);
	return 0;
}

struct file_operations gpio_led_fops={	
	.owner		= THIS_MODULE,	
	.unlocked_ioctl		= ledgpio_pwr_ioctl,	
	.open		= ledgpio_open,	
	.release 		= ledgpio_release,
	};
#if 1
static struct miscdevice gpio_led_miscdev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "ledgpio",
	.fops	= &gpio_led_fops,
};
#endif
static struct platform_driver gpio_led_driver = {
	.probe		= gpio_led_probe,
	.remove		= gpio_led_remove,
	.driver		= {
		.name	= "gpio-led",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(of_gpio_led_match),
	},
};
static int __init ledgpio_init(void)
{
#ifdef DEBUG
	printk("%s: init\n", __func__);
#endif
	return misc_register(&gpio_led_miscdev);
}
device_initcall(ledgpio_init);

module_platform_driver(gpio_led_driver);

MODULE_AUTHOR("");
MODULE_DESCRIPTION("GPIO LED control driver");
MODULE_LICENSE("GPL v2");
