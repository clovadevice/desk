/*
 *
 * ATTINY1617  Touch Volume driver.
 *
 * Copyright (c) 2017  
 * 
 *
 */

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/firmware.h>
#include <linux/debugfs.h>
#include <linux/input/ft5x06_ts.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/timer.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>


#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#endif


#include <linux/completion.h>
#include <linux/atomic.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/wakelock.h>
#include "attiny1617_tv.h"
#include "attiny1617_fw.h"



/*MACRO*/
//#define ATTINY1617_LOOP_WAIT
#define ATTINY1617_FW_NEW
#define  TV_UEVENT



//#define ATTINY_WAKE_LOCK

#define PINCTRL_STATE_ACTIVE	"pmx_tv_active"
#define PINCTRL_STATE_SUSPEND	"pmx_tv_suspend"
#define PINCTRL_STATE_RELEASE	"pmx_tv_release"
#define PINTCTRL_STATE_BOOT_MODE_ACTIVE  "pm_tv_boot_active"
#define PINTCTRL_STATE_BOOT_MODE_ACTIVE1  "pmx_tv_bootmode_pullup_active"





/* register address*/

#define CMD_CHIP_PART_NUMBERR		0x01
#define CMD_FW_VERSION	0x02
#define CMD_SLIDE_POSITION	0x11
#define CMD_RD_DATA	0x11

#define TV_I2C_VTG_MIN_UV	1800000
#define TV_I2C_VTG_MAX_UV	1800000
#define TVMSG_MAX_SIZE    64


#define READ_FRAME_LEN_MAX    16
#define FW_PAGE_NUMBER_MAX    232

#define DATA_LENGTH	64

uint8_t comm_cnt = 0, tmp_ch_cnt = 0;	
uint8_t write_buffer[DATA_LENGTH];
uint16_t user_app_size = 0;
uint16_t user_app_packet = 0;
uint16_t tx_packet_cnt = 0;
//uint32_t page_cnt = 0;
unsigned char FW_INFOR[] = {'1','7','1','2','1','6','0','6'};
unsigned char CHPI_NUMBER[] = "TINY1617";



unsigned char send_chip_number_cmd[]={'S',1,1,'E'};
unsigned char send_fw_version_cmd[]={'S',1,2,'E'};
unsigned char send_slide_position_cmd[]={'S',1,0x11,'E'};
unsigned char send_judge_fw_cmd[]={'S',3,0x03,0xAB,0xCD,'E'};

unsigned char send_start_updatefw_cmd[]={'S',1,3,'E'};
unsigned char send_end_updatefw_cmd[]={'S',1,4,'E'};



#ifndef TV_UEVENT
static int tv_key_state = 0;
static int tv_pasuekey_state = 0;
static int tv_double_click_flag = 0;
#endif

#ifdef TV_UEVENT
static struct kobject * attiny1617_tv_obj;
#endif



struct attiny1617_tv_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	const struct attiny1617_tv_platform_data *pdata;
	struct regulator *vdd;
	struct regulator *vcc_i2c;
	struct mutex tv_clk_io_ctrl_mutex;
	char fw_verison[8];
	char chip_part_number[8];
	struct dentry *dir;
	u16 addr;
	bool suspended;
	char *tv_info;
	struct pinctrl *tv_pinctrl;
	struct pinctrl_state *pinctrl_state_active;
	struct pinctrl_state *pinctrl_state_suspend;
	struct pinctrl_state *pinctrl_state_release;
	struct pinctrl_state *pinctrl_state_bootomde_active;
	struct pinctrl_state *pinctrl_state_bootomde_pullup_active;
};

static u8 rdframe_buffer[READ_FRAME_LEN_MAX];
//static bool normal_working = false;
//static int touchint_clknum = 0;


uint32_t n_delay;
//uint32_t page_cnt = 0;

uint8_t touch_int_toggle = 0;
uint8_t t_toggle_cnt = 0;
#if 0
static int touch_count = 0;
static int touch_notifyflg = 0;
static u8 touch_pbuf[3]={0,0,0};
#endif 

struct wake_lock attiny_wake_lock;

struct timer_list attiny_timer;
struct mutex attiny_mutex;
//static int attiny1617_ts_start(struct device *dev);
//static int attiny1617_ts_stop(struct device *dev);
static irqreturn_t attiny1617_tv_interrupt(int irq, void *data);


static int attiny1617_i2c_read(struct i2c_client *client, char *writebuf,
			   int writelen, char *readbuf, int readlen)
{
	int ret;

	if (writelen > 0) {
		struct i2c_msg msgs[] = {
			{
				 .addr = client->addr,
				 .flags = 0,
				 .len = writelen,
				 .buf = writebuf,
			 },
			{
				 .addr = client->addr,
				 .flags = I2C_M_RD,
				 .len = readlen,
				 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret < 0)
			dev_err(&client->dev, "%s: i2c read error.\n",
				__func__);
	} else {
		struct i2c_msg msgs[] = {
			{
				 .addr = client->addr,
				 .flags = I2C_M_RD,
				 .len = readlen,
				 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret < 0)
			dev_err(&client->dev, "%s:i2c read error.\n", __func__);
	}
	return ret;
}

static int attiny1617_i2c_write(struct i2c_client *client, char *writebuf,
			    int writelen)
{
	int ret;
	
	struct i2c_msg msgs[] = {
		{
			 .addr = client->addr,
			 .flags = 0,
			 .len = writelen,
			 .buf = writebuf,
		 },
	};
	
	//pr_err("<><> start attiny1617_i2c_write\n");
	
	ret = i2c_transfer(client->adapter, msgs, 1);
	if (ret < 0)
		dev_err(&client->dev, "%s: i2c write error. -ret = %d\n", __func__,-ret);

	return ret;
}
 
/*when cmd = 0x12,if cmd1<0, pass 0x12;if cmd1>=0,pass cmd1*/
static int attiny1617_send_cmd(struct i2c_client *client, char* cmd,int writelen)
{
	
	
	#if 0
	int i = 0;
	pr_err("<><> attiny1617_send_cmd:length = %d\n",writelen);
	
	for(i=0;i<writelen;i++)
	{
		pr_err("%d ",cmd[i]);
	}
	pr_err("\n ");
	#endif

	return attiny1617_i2c_write(client, cmd, writelen);
}

static int attiny1617_read_frame(struct i2c_client *client, u8 cmd, u8 *rbuf)
{

	u8 readlen = 0;
	u8 writelen = 0;
	char buf[]={0};
	

	switch(cmd)
	{
		case 0x01:
			readlen = 11;
			break;
		case 0x02:
			readlen = 11;
			break;
		case 0x11:
		case 0x12:
			readlen = 5;
			break;
		case 0x03:
		case 0x04:
			readlen = 4;
			break;
		case 0xAB:
			readlen = 67;
			break;
		default:
			printk("%s attiny1617 donot support  cmd 0x%x",__FUNCTION__,cmd);
			return -1;
	}
	
	
	return attiny1617_i2c_read(client, buf, writelen, rbuf, readlen);
}

static void reset_attiny1617(struct attiny1617_tv_data *data)
{
	gpio_direction_output(data->pdata->reset_gpio,1);
	gpio_set_value(data->pdata->reset_gpio,1);
	mdelay(10);
	gpio_set_value(data->pdata->reset_gpio,0);
	mdelay(6);
	gpio_set_value(data->pdata->reset_gpio,1);
	
	return;

}


static void check_pausekey(unsigned long data)
{

	data = data;

	pr_err("%s <><> attiny_timer timer expire\n",__func__);
	#ifndef TV_UEVENT
	//del_timer(&attiny_timer);
	tv_double_click_flag=0 ;
	#endif

}

static irqreturn_t attiny1617_tv_interrupt(int irq, void *dev_id)
{
	struct attiny1617_tv_data *data = dev_id;
	//struct input_dev *ip_dev;
	int ret;
	u8 *buf;
	char *sendcmd;
	int cmd = 0;
	//int irq_gpio_value = -1;
	//int writelen;
	#ifndef TV_UEVENT
	unsigned int keycode;
	#endif
	
	#ifdef TV_UEVENT
	char tv_state_msg[TVMSG_MAX_SIZE];
	char tv_position_msg[TVMSG_MAX_SIZE];
	char *envp[] = { tv_state_msg, tv_position_msg, NULL };
	int res;
	//static int i = 0;
	static u8 tv_position = 0;
	static u8 tv_state = 0;
	//static u8 tv_ps1=  0;
	#endif

	//pr_err("<><> attiny1617_tv_interrupt\n");

	if (!data) {
		pr_err("%s: Invalid data\n", __func__);
		return IRQ_HANDLED;
	}
	
	mutex_lock(&attiny_mutex);
	//irq_gpio_value = gpio_get_value(data->pdata->irq_gpio);

	//ip_dev = data->input_dev;
	buf = rdframe_buffer;

	/*
	 * Read touch data start from device 0x11.
	 */
	
	sendcmd= send_slide_position_cmd;
	
	cmd = CMD_SLIDE_POSITION; 
	ret=attiny1617_send_cmd(data->client,sendcmd,sizeof(send_slide_position_cmd));
	if(ret<0)
	{
		pr_err("%s: send slide position cmd err\n", __func__);
		mutex_unlock(&attiny_mutex);
		return IRQ_HANDLED;
	}
	udelay(100);
	//msleep(1);
	ret=attiny1617_read_frame(data->client,cmd,buf);
	if (ret< 0) {
		dev_err(&data->client->dev, "%s: read data fail\n", __func__);
		mutex_unlock(&attiny_mutex);
		return IRQ_HANDLED;
	}	
	
	//pr_err("%s: touch status = %d, positon=%d\n", __func__,buf[2],buf[3]);
	

	#ifndef TV_UEVENT
	if((buf[3]<=255)&&(buf[3]>=245))
	{
		keycode = KEY_VOLUMEUP;
	}
	
	else if((buf[3]<=20)&&(buf[3]>=0))
	{
		keycode = KEY_VOLUMEDOWN;
	}
	else if ((buf[3]<=170)&&(buf[3]>=110))
	{
		keycode = KEY_PLAYPAUSE;
		
	}

	
	if((buf[2] == 0x02)&&(tv_key_state==0)&&((keycode == KEY_VOLUMEDOWN)||(keycode == KEY_VOLUMEUP))) //press
	{
		tv_key_state = 1;
		input_report_key(data->input_dev, keycode, 1);
		input_sync(data->input_dev);
	}
	else if(((buf[2] == 0x01)||(buf[2] == 0x00))&&(tv_key_state==1)&&((keycode == KEY_VOLUMEDOWN)||(keycode == KEY_VOLUMEUP)))//release
	{
		tv_key_state = 0;
		input_report_key(data->input_dev, keycode, 0);
		input_sync(data->input_dev);
	}
	
	if((buf[2] == 0x02)&&(tv_pasuekey_state==0)&&((keycode == KEY_PLAYPAUSE))) //press
	{
		tv_pasuekey_state = 1;
	
	}
	else if(((buf[2] == 0x01)||(buf[2] == 0x00))&&(tv_pasuekey_state==1)&&((keycode == KEY_PLAYPAUSE)))//release
	{
		tv_pasuekey_state = 2;
		// start timer
		attiny_timer.expires = jiffies + msecs_to_jiffies(2000);
		tv_double_click_flag = 1;
		add_timer(&attiny_timer);
	}
	else if((buf[2] == 0x02)&&(tv_pasuekey_state==2)&&((keycode == KEY_PLAYPAUSE))) //press
	{
		if(tv_double_click_flag ==1)
		{
			del_timer(&attiny_timer);
			tv_double_click_flag = 0;
			input_report_key(data->input_dev, keycode, 1);
			input_sync(data->input_dev);
			tv_pasuekey_state = 3;
		}
		else
		{
			del_timer(&attiny_timer);
			tv_pasuekey_state = 1;
		}
	
	}
	else if(((buf[2] == 0x01)||(buf[2] == 0x00))&&(tv_pasuekey_state==3)&&((keycode == KEY_PLAYPAUSE)))//release
	{
			tv_double_click_flag = 0;
			tv_pasuekey_state = 0;
			input_report_key(data->input_dev, keycode, 0);
			input_sync(data->input_dev);
	}
	#endif

	#ifdef TV_UEVENT
	tv_state = buf[2];
	tv_position = buf[3];
	res = snprintf(tv_state_msg, TVMSG_MAX_SIZE, "TV_STATE=%d",	tv_state);
	if (TVMSG_MAX_SIZE <= res) {
		pr_err("message too long (%d)", res);
		mutex_unlock(&attiny_mutex);
		return IRQ_HANDLED;
	}
	res = snprintf(tv_position_msg, TVMSG_MAX_SIZE, "TV_POSITION=%d",tv_position);
	if (TVMSG_MAX_SIZE <= res) {
		pr_err("message too long (%d)", res);
		mutex_unlock(&attiny_mutex);
		return IRQ_HANDLED;
	}
	kobject_uevent_env(attiny1617_tv_obj, KOBJ_CHANGE, envp);
	#if  0 
	if(buf[2]==2)
	{
		touch_count++;
		if(touch_count == 1)
			tv_ps1=buf[3];
		if(touch_count>1)
			touch_pbuf[i++] = buf[3];
		if(i==3)
		{
			i =0;
		}
		if(touch_count >4 )
		{
			tv_state = buf[2];
			tv_position = (touch_pbuf[0]+touch_pbuf[1]+touch_pbuf[2])/3;
			res = snprintf(tv_state_msg, TVMSG_MAX_SIZE, "TV_STATE=%d",	tv_state);
			if (TVMSG_MAX_SIZE <= res) {
				pr_err("message too long (%d)", res);
				memset(touch_pbuf,0,sizeof(touch_pbuf));
				touch_count = 0;
				mutex_unlock(&attiny_mutex);
	
				return IRQ_HANDLED;
			}
			res = snprintf(tv_position_msg, TVMSG_MAX_SIZE, "TV_POSITION=%d",tv_position);
			if (TVMSG_MAX_SIZE <= res) {
				pr_err("message too long (%d)", res);
				memset(touch_pbuf,0,sizeof(touch_pbuf));
				touch_count = 0;
				mutex_unlock(&attiny_mutex);
				return IRQ_HANDLED;
			}
			kobject_uevent_env(attiny1617_tv_obj, KOBJ_CHANGE, envp);
			touch_notifyflg = 1;
		}
	}
	else
	{
		if((touch_count <=3)&&(touch_count >=1))
		{
			tv_state = 2;
			if(touch_count == 1)
				tv_position =  tv_ps1;
			else
				tv_position = (touch_pbuf[0]+touch_pbuf[1]+touch_pbuf[2])/(touch_count-1);
			res = snprintf(tv_state_msg, TVMSG_MAX_SIZE, "TV_STATE=%d",	tv_state);
			if (TVMSG_MAX_SIZE <= res) {
				pr_err("message too long (%d)", res);
				memset(touch_pbuf,0,sizeof(touch_pbuf));
				touch_count = 0;
				mutex_unlock(&attiny_mutex);
	
				return IRQ_HANDLED;
			}
			res = snprintf(tv_position_msg, TVMSG_MAX_SIZE, "TV_POSITION=%d",tv_position);
			if (TVMSG_MAX_SIZE <= res) {
				pr_err("message too long (%d)", res);
				memset(touch_pbuf,0,sizeof(touch_pbuf));
				touch_count = 0;
				mutex_unlock(&attiny_mutex);
				return IRQ_HANDLED;
			}
			kobject_uevent_env(attiny1617_tv_obj, KOBJ_CHANGE, envp);
			touch_count = 0;
			touch_notifyflg = 1;
		}

		if(touch_notifyflg )
		{
			tv_state = 0;
			res = snprintf(tv_state_msg, TVMSG_MAX_SIZE, "TV_STATE=%d",	tv_state);
			if (TVMSG_MAX_SIZE <= res) {
				pr_err("message too long (%d)", res);
				memset(touch_pbuf,0,sizeof(touch_pbuf));
				touch_count = 0;
				mutex_unlock(&attiny_mutex);
				return IRQ_HANDLED;
			}
			res = snprintf(tv_position_msg, TVMSG_MAX_SIZE, "TV_POSITION=%d",tv_position);
			if (TVMSG_MAX_SIZE <= res) {
				pr_err("message too long (%d)", res);
				memset(touch_pbuf,0,sizeof(touch_pbuf));
				touch_count = 0;
				mutex_unlock(&attiny_mutex);
				return IRQ_HANDLED;
			}
			//pr_err("putting tvmsg: <%s> <%s>\n", tv_state_msg, tv_position_msg);
			kobject_uevent_env(attiny1617_tv_obj, KOBJ_CHANGE, envp);
			touch_count = 0;
			tv_ps1 =  0;
			memset(touch_pbuf,0,sizeof(touch_pbuf));
		}
		i = 0;
	}
	#endif
	#endif
	
	mutex_unlock(&attiny_mutex);
	return IRQ_HANDLED;
}

static int attiny1617_gpio_request(struct attiny1617_tv_data *data)
{
	int err = 0;
	
	pr_err("enter %s  \n",__func__);
	
	if (gpio_is_valid(data->pdata->irq_gpio)) {
		err = gpio_request(data->pdata->irq_gpio,
							"attiny1617_irq_gpio");
		if (err) {
			dev_err(&data->client->dev,
						"irq gpio request failed");
			goto err_irq_gpio_req;
		}
	
	}
	
	if (gpio_is_valid(data->pdata->reset_gpio)) {
		err = gpio_request(data->pdata->reset_gpio,
							"attiny1617_reset_gpio");
		if (err) {
			dev_err(&data->client->dev,
						"reset gpio request failed");
			goto err_irq_gpio_request;
		}
	
	}
	
	if (gpio_is_valid(data->pdata->boot_mode_gpio)) {
		err = gpio_request(data->pdata->boot_mode_gpio,
							"attiny1617_boot_mode_gpio");
		if (err) {
			dev_err(&data->client->dev,
						"boot mode  gpio request failed");
			goto err_reset_gpio_request;
		}
	
	}
	
	return 0;
		
	
	err_reset_gpio_request:
		if (gpio_is_valid(data->pdata->reset_gpio))
			gpio_free(data->pdata->reset_gpio);
	err_irq_gpio_request:
		if (gpio_is_valid(data->pdata->irq_gpio))
			gpio_free(data->pdata->irq_gpio);
	err_irq_gpio_req:
		return err;

}

#if 1
static int attiny1617_gpio_configure(struct attiny1617_tv_data *data)
{
	int err = 0;
	
	if (gpio_is_valid(data->pdata->irq_gpio)) {
		err = gpio_direction_input(data->pdata->irq_gpio);
		if (err) {
				dev_err(&data->client->dev,
					"set_direction for irq gpio failed\n");
				goto err_irq_gpio;
		}
	}

	if (gpio_is_valid(data->pdata->boot_mode_gpio)) {
		err = gpio_direction_input(data->pdata->boot_mode_gpio);
		//err=gpio_direction_output(data->pdata->reset_gpio,1);
		if (err) {
			dev_err(&data->client->dev,
			"set_direction for reset gpio failed\n");
			goto err_boot_mode_gpio;
		}
	}

	return 0;
	
err_irq_gpio:
	if (gpio_is_valid(data->pdata->irq_gpio))
		gpio_free(data->pdata->irq_gpio);

err_boot_mode_gpio:
	if (gpio_is_valid(data->pdata->boot_mode_gpio))
		gpio_free(data->pdata->boot_mode_gpio);
	
	return err;
}
#endif
static int attiny1617_power_on(struct attiny1617_tv_data *data, bool on)
{
	int rc;

	if (!on)
		goto power_off;

	rc = regulator_enable(data->vdd);
	if (rc) {
		dev_err(&data->client->dev,
			"Regulator vdd enable failed rc=%d\n", rc);
		return rc;
	}

	rc = regulator_enable(data->vcc_i2c);
	if (rc) {
		dev_err(&data->client->dev,
			"Regulator vcc_i2c enable failed rc=%d\n", rc);
		regulator_disable(data->vdd);
	}

	return rc;

power_off:
	rc = regulator_disable(data->vdd);
	if (rc) {
		dev_err(&data->client->dev,
			"Regulator vdd disable failed rc=%d\n", rc);
		return rc;
	}

	rc = regulator_disable(data->vcc_i2c);
	if (rc) {
		dev_err(&data->client->dev,
			"Regulator vcc_i2c disable failed rc=%d\n", rc);
		rc = regulator_enable(data->vdd);
		if (rc) {
			dev_err(&data->client->dev,
				"Regulator vdd enable failed rc=%d\n", rc);
		}
	}

	return rc;
}

static int attiny1617_power_init(struct attiny1617_tv_data *data, bool on)
{
	int rc;

	if (!on)
		goto pwr_deinit;

	data->vdd = regulator_get(&data->client->dev, "vdd");
	if (IS_ERR(data->vdd)) {
		rc = PTR_ERR(data->vdd);
		dev_err(&data->client->dev,
			"Regulator get failed vdd rc=%d\n", rc);
		return rc;
	}

	if (regulator_count_voltages(data->vdd) > 0) {
		rc = regulator_set_voltage(data->vdd, TV_I2C_VTG_MIN_UV,
					   TV_I2C_VTG_MAX_UV);
		if (rc) {
			dev_err(&data->client->dev,
				"Regulator set_vtg failed vdd rc=%d\n", rc);
			goto reg_vdd_put;
		}
	}

	data->vcc_i2c = regulator_get(&data->client->dev, "vcc_i2c");
	if (IS_ERR(data->vcc_i2c)) {
		rc = PTR_ERR(data->vcc_i2c);
		dev_err(&data->client->dev,
			"Regulator get failed vcc_i2c rc=%d\n", rc);
		goto reg_vdd_set_vtg;
	}

	if (regulator_count_voltages(data->vcc_i2c) > 0) {
		rc = regulator_set_voltage(data->vcc_i2c, TV_I2C_VTG_MIN_UV,
					   TV_I2C_VTG_MAX_UV);
		if (rc) {
			dev_err(&data->client->dev,
			"Regulator set_vtg failed vcc_i2c rc=%d\n", rc);
			goto reg_vcc_i2c_put;
		}
	}

	return 0;


reg_vcc_i2c_put:
	regulator_put(data->vcc_i2c);
reg_vdd_set_vtg:
	if (regulator_count_voltages(data->vdd) > 0)
		regulator_set_voltage(data->vdd, 0, TV_I2C_VTG_MAX_UV);
reg_vdd_put:
	regulator_put(data->vdd);
	return rc;

pwr_deinit:
	if (regulator_count_voltages(data->vdd) > 0)
		regulator_set_voltage(data->vdd, 0, TV_I2C_VTG_MAX_UV);

	regulator_put(data->vdd);

	if (regulator_count_voltages(data->vcc_i2c) > 0)
		regulator_set_voltage(data->vcc_i2c, 0, TV_I2C_VTG_MAX_UV);

	regulator_put(data->vcc_i2c);

	return 0;
}

static int attiny1617_tv_pinctrl_init(struct attiny1617_tv_data *attiny1617_data)
{
	int retval;
	pr_err("enter %s  \n",__func__);

	/* Get pinctrl if target uses pinctrl */
	attiny1617_data->tv_pinctrl = devm_pinctrl_get(&(attiny1617_data->client->dev));
	if (IS_ERR_OR_NULL(attiny1617_data->tv_pinctrl)) {
		retval = PTR_ERR(attiny1617_data->tv_pinctrl);
		pr_err("%s Target does not use pinctrl %d\n",__func__, retval);
		goto err_pinctrl_get;
	}

	attiny1617_data->pinctrl_state_active
		= pinctrl_lookup_state(attiny1617_data->tv_pinctrl,
				PINCTRL_STATE_ACTIVE);
	if (IS_ERR_OR_NULL(attiny1617_data->pinctrl_state_active)) {
		retval = PTR_ERR(attiny1617_data->pinctrl_state_active);
		dev_err(&attiny1617_data->client->dev,
			"Can not lookup %s pinstate %d\n",
			PINCTRL_STATE_ACTIVE, retval);
		goto err_pinctrl_lookup;
	}
	
	attiny1617_data->pinctrl_state_bootomde_active
		= pinctrl_lookup_state(attiny1617_data->tv_pinctrl,
				PINTCTRL_STATE_BOOT_MODE_ACTIVE);
	if (IS_ERR_OR_NULL(attiny1617_data->pinctrl_state_active)) {
		retval = PTR_ERR(attiny1617_data->pinctrl_state_active);
		dev_err(&attiny1617_data->client->dev,
			"Can not lookup %s pinstate %d\n",
			PINTCTRL_STATE_BOOT_MODE_ACTIVE, retval);
		goto err_pinctrl_lookup;
	}

	
	attiny1617_data->pinctrl_state_bootomde_pullup_active
		= pinctrl_lookup_state(attiny1617_data->tv_pinctrl,
				PINTCTRL_STATE_BOOT_MODE_ACTIVE1);
	if (IS_ERR_OR_NULL(attiny1617_data->pinctrl_state_active)) {
		retval = PTR_ERR(attiny1617_data->pinctrl_state_active);
		dev_err(&attiny1617_data->client->dev,
			"Can not lookup %s pinstate %d\n",
			PINTCTRL_STATE_BOOT_MODE_ACTIVE1, retval);
		goto err_pinctrl_lookup;
	}

	attiny1617_data->pinctrl_state_suspend
		= pinctrl_lookup_state(attiny1617_data->tv_pinctrl,
			PINCTRL_STATE_SUSPEND);
	if (IS_ERR_OR_NULL(attiny1617_data->pinctrl_state_suspend)) {
		retval = PTR_ERR(attiny1617_data->pinctrl_state_suspend);
		dev_err(&attiny1617_data->client->dev,
			"Can not lookup %s pinstate %d\n",
			PINCTRL_STATE_SUSPEND, retval);
		goto err_pinctrl_lookup;
	}

	attiny1617_data->pinctrl_state_release
		= pinctrl_lookup_state(attiny1617_data->tv_pinctrl,
			PINCTRL_STATE_RELEASE);
	if (IS_ERR_OR_NULL(attiny1617_data->pinctrl_state_release)) {
		retval = PTR_ERR(attiny1617_data->pinctrl_state_release);
		dev_dbg(&attiny1617_data->client->dev,
			"Can not lookup %s pinstate %d\n",
			PINCTRL_STATE_RELEASE, retval);
	}

	return 0;

err_pinctrl_lookup:
	devm_pinctrl_put(attiny1617_data->tv_pinctrl);
err_pinctrl_get:
	attiny1617_data->tv_pinctrl = NULL;
	return retval;
}


static int attiny1617_parse_dt(struct device *dev,
			struct attiny1617_tv_platform_data *pdata)
{
	//int rc;
	struct device_node *np = dev->of_node;
	//struct property *prop;

	pdata->name = "attiny1617";
	pdata->chip_part_number = "TINY1617";


	/* reset, irq gpio info */
	pdata->reset_gpio = of_get_named_gpio_flags(np, "attiny1617,reset-gpio",
				0, &pdata->reset_gpio_flags);
	if (pdata->reset_gpio < 0)
		return pdata->reset_gpio;

	pdata->irq_gpio = of_get_named_gpio_flags(np, "attiny1617,irq-gpio",
				0, &pdata->irq_gpio_flags);
	if (pdata->irq_gpio < 0)
		return pdata->irq_gpio;

	pdata->boot_mode_gpio= of_get_named_gpio_flags(np, "attiny1617,boot-mode-gpio",
				0, &pdata->boot_mode_gpio_flags);
	if (pdata->boot_mode_gpio < 0)
		return pdata->boot_mode_gpio;

	return 0;
}



static int  attiny1617_update_fw(struct attiny1617_tv_data *attiny1617_data)
{
	uint16_t i,j;
	uint16_t writelen;
	int ret = 0;
	int temp = 0;
	struct attiny1617_tv_data *data = attiny1617_data;
	
	
	user_app_size = sizeof( _acIF_S200N);
	user_app_packet = user_app_size/DATA_LENGTH;			
	//page_cnt = 0;
	tx_packet_cnt = 0;
	temp = user_app_size % DATA_LENGTH;
	
	pr_err("%s <><> attiny1617 upgrade FW user_app_size =0x%x,user_app_packet =%d temp=%d\n",
		__func__,user_app_size,user_app_packet,temp);


	if(!user_app_packet)
	{
		for(j = 0; j < user_app_size; j++)	write_buffer[j] =  _acIF_S200N[j];

		for(; j < DATA_LENGTH; j++)	write_buffer[j] = 0xFF;
				
		writelen = DATA_LENGTH;	
		ret = attiny1617_i2c_write(data->client,write_buffer,writelen);
		
		if(ret < 0) 
		{
			pr_err("%s <><> attiny1617 upgrade FW err, \n",__func__);
		}
		msleep(10);
		return  ret ;
	}
	else if(!(user_app_size % DATA_LENGTH))
	{	
		pr_err("%s <><> update fw 1 :::user_app_size mod data_length is null\n",__func__);
		
		for(i = 0; i < user_app_packet; i++)
		{
			tx_packet_cnt++;
			for(j = 0; j < DATA_LENGTH; j++)
			{
				write_buffer[j] = _acIF_S200N[j+(DATA_LENGTH*i)];						
			}
			writelen = DATA_LENGTH;	
			ret = attiny1617_i2c_write(data->client,write_buffer,writelen);
		
			if(ret < 0) 
			{
				pr_err("%s <><> attiny1617 upgrade FW err, tx_packet_cnt = %d\n",__func__,tx_packet_cnt);
				return ret ;
			}
			msleep(10);
			
		}								
	}
	else
	{
		pr_err("%s <><> update fw 2\n",__func__);
				
		for(i = 0; i <= user_app_packet; i++)
		{
			tx_packet_cnt++;		
			if(user_app_packet == i)
			{
				for(j = 0; j < (user_app_size % DATA_LENGTH); j++)
				{
					write_buffer[j] = _acIF_S200N[j+(DATA_LENGTH*i)];
				}
						
				for(; j < DATA_LENGTH; j++)	write_buffer[j] = 0xFF;
			}
			else
			{
				for(j = 0; j < DATA_LENGTH; j++)
				{
					write_buffer[j] = _acIF_S200N[j+(DATA_LENGTH*i)];
				}
			}
			writelen = DATA_LENGTH;	
			ret = attiny1617_i2c_write(data->client,write_buffer,writelen);
		
			if(ret < 0) 
			{
				pr_err("%s <><> attiny1617 upgrade FW err, tx_packet_cnt = %d\n",__func__,tx_packet_cnt);
				return ret;
			}
			msleep(10);
			

		}
			
	}
	for(i = tx_packet_cnt; i <FW_PAGE_NUMBER_MAX; i++)
	{
		tx_packet_cnt++;
		for(j = 0; j < DATA_LENGTH; j++)
		{
			write_buffer[j] = 0xFF;
		}
		if(tx_packet_cnt==FW_PAGE_NUMBER_MAX)
		{
			write_buffer[63]='A';
			write_buffer[62]='B';
		}
		writelen = DATA_LENGTH;	
		ret = attiny1617_i2c_write(data->client,write_buffer,writelen);
		
		if(ret < 0) 
		{
			pr_err("%s <><> attiny1617 upgrade FW err, tx_packet_cnt = %d\n",__func__,tx_packet_cnt);
			return ret ;
		}
		msleep(10);
	}
	pr_err("%s <><> attiny1617 upgrade FW  OK!!! \n",__func__);
	return ret ;
}

#ifdef ATTINY_WAKE_LOCK

static void check_normalworking(unsigned long data)
{
	int bootmode_gpio_value = -1;
	//int touchint_gpio_value = -1;
	u8 cmd;
	int cmd1;
	char chippartnumber[10];

	struct attiny1617_tv_data *tvdata = (struct attiny1617_tv_data *)data;

	pr_err("%s <><> check_normalworking\n",__func__);

	del_timer(&attiny_timer);
	pr_err(" touchint_clknum = %d\n",touchint_clknum);
	
	if(touchint_clknum>=3)
	{
		bootmode_gpio_value =  gpio_get_value(tvdata->pdata->boot_mode_gpio);
		while(gpio_get_value(tvdata->pdata->irq_gpio));
	
		attiny1617_update_fw();
	}
	else
	{
		
		bootmode_gpio_value =  gpio_get_value(tvdata->pdata->boot_mode_gpio);
		pr_err("%s : bootmode_gpio_value = %d\n",__func__,bootmode_gpio_value);
		if(bootmode_gpio_value == 1)
		{
			normal_working = true;
		}
		else if (bootmode_gpio_value == 0)
		{
			pr_err("%s : call attiny1617_update_fw\n",__func__);
			cmd = 0x01;
			cmd1=0;
			attiny1617_send_cmd(tvdata->client,cmd,cmd1);
			attiny1617_read_frame(tvdata->client,cmd,chippartnumber);
			chippartnumber[8]= '\0';
			pr_err(" chippartnumber: %s\n",chippartnumber);
			attiny1617_update_fw();
		}

	}
	wake_unlock(&attiny_wake_lock);
	
}
#endif
static int attiny1617_tv_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct attiny1617_tv_platform_data *pdata;
	struct attiny1617_tv_data *data;
	struct input_dev *input_dev;

	//int timeoutcnt;
	char *cmdframe;
	int cmd;
	char chippartnumber[20]={0};
	char readbuf[12]={0};
	
	#ifdef ATTINY1617_FW_NEW
	//char recbuf[70]={0};
	static int update_fw_count = 0;
	#endif
	
	int err,ret;
	#ifndef TV_UEVENT
	int check_workcnt = 2; //just convience for debug
	#endif
	int waittime_cnt = 20;
	int i ;
	//int len,retval;
	
	
	//int bootmode_gpio_value ;
	
	int irq_gpio_value= 0;
	
	pr_err("<><> attiny1617_tv_probe\n");

	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct attiny1617_tv_platform_data), GFP_KERNEL);
		if (!pdata)
			return -ENOMEM;

		err = attiny1617_parse_dt(&client->dev, pdata);
		if (err) {
			dev_err(&client->dev, "DT parsing failed\n");
			return err;
		}
	} else
		pdata = client->dev.platform_data;

	if (!pdata) {
		dev_err(&client->dev, "Invalid pdata\n");
		return -EINVAL;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C not supported\n");
		return -ENODEV;
	}

	data = devm_kzalloc(&client->dev,
			sizeof(struct attiny1617_tv_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;


	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&client->dev, "failed to allocate input device\n");
		return -ENOMEM;
	}

	data->input_dev = input_dev;
	data->client = client;
	data->pdata = pdata;

	input_dev->name = "attiny1617_tv";
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;

	input_set_drvdata(input_dev, data);
	i2c_set_clientdata(client, data);

	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(KEY_VOLUMEUP, input_dev->keybit);
	__set_bit(KEY_VOLUMEDOWN, input_dev->keybit);
	__set_bit(KEY_PLAYPAUSE, input_dev->keybit);
	//__set_bit(EV_ABS, input_dev->evbit);




	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev, "Input device registration failed\n");
		input_free_device(input_dev);
		return err;
	}
	#ifdef TV_UEVENT
	attiny1617_tv_obj = &client->dev.kobj;
	#endif
	mutex_init(&attiny_mutex);
	init_timer(&attiny_timer);
	
	#ifdef ATTINY_WAKE_LOCK
	attiny_timer.function = check_normalworking;
	#else
	attiny_timer.function = check_pausekey;
	#endif
	
	attiny_timer.data = (unsigned long)data;

	wake_lock_init(&attiny_wake_lock,
			WAKE_LOCK_SUSPEND, "attiny-wakelock");
	
	if (pdata->power_init) {
		err = pdata->power_init(true);
		if (err) {
			dev_err(&client->dev, "power init failed");
			goto unreg_inputdev;
		}
	} else {
		err = attiny1617_power_init(data, true);
		if (err) {
			dev_err(&client->dev, "power init failed");
			goto unreg_inputdev;
		}
	}

	if (pdata->power_on) {
		err = pdata->power_on(true);
		if (err) {
			dev_err(&client->dev, "power on failed");
			goto pwr_deinit;
		}
	} else {
		err = attiny1617_power_on(data, true);
		if (err) {
			dev_err(&client->dev, "power on failed");
			goto pwr_deinit;
		}
	}

	err = attiny1617_tv_pinctrl_init(data);
	if (!err && data->tv_pinctrl) {
		/*
		 * Pinctrl handle is optional. If pinctrl handle is found
		 * let pins to be configured in active state. If not
		 * found continue further without error.
		 */
		pr_err("%s pinctrl_select  \n",__func__);
		err = pinctrl_select_state(data->tv_pinctrl,
					data->pinctrl_state_active);
		if (err < 0) {
			dev_err(&client->dev,
				"failed to select pin to active state");
		}
		
		
	}
    

	pr_err("%s :::111 pinctrl_select  boot mode  pull up  \n",__func__); 
	err = pinctrl_select_state(data->tv_pinctrl,
					data->pinctrl_state_bootomde_pullup_active);
	if (err < 0)
	{
		dev_err(&client->dev,
				"failed to select pin to active state");
	}
	gpio_direction_output(data->pdata->boot_mode_gpio,1);
	gpio_set_value(data->pdata->boot_mode_gpio,1);
	pr_err("%s pull high boot_mode_gpio \n",__func__);
	msleep(10);
	err=attiny1617_gpio_request(data);
	if (err) {
		dev_err(&client->dev, "request gpio failed\n");
		goto unreg_inputdev;
	}
	
	err=attiny1617_gpio_configure(data);
	if (err) {
		dev_err(&client->dev, "config  gpio failed\n");
		goto unreg_inputdev;
	}
	gpio_direction_output(data->pdata->reset_gpio,1);
	//pr_err("%s start reset attiny1617\n",__func__);
	reset_attiny1617(data);
	pr_err("%s end reset attiny1617\n",__func__);
	
	#ifdef ATTINY1617_FW_NEW

attiny_start:
	msleep(1000);

	
	t_toggle_cnt = 0;
	touch_int_toggle = 0;
		
	n_delay = 1000000;
	
	irq_gpio_value = gpio_get_value(data->pdata->irq_gpio);
	pr_err("%s first check irq_gpio_value = %d\n",__func__,irq_gpio_value);

	if(irq_gpio_value)
	{
		printk("eneter normal work\n");
		goto normal_work;
	}
	else
	{
		msleep(100);
		waittime_cnt = 20;
		irq_gpio_value = gpio_get_value(data->pdata->irq_gpio);
		pr_err("%s check int clk irq_gpio_value = %d\n",__func__,irq_gpio_value);
		while((irq_gpio_value)&&(waittime_cnt--))
		{
			msleep(50);
			irq_gpio_value=  gpio_get_value(data->pdata->irq_gpio);
			pr_err("%s : irq_gpio_value = %d\n",__func__,irq_gpio_value);
		}
		
		if((waittime_cnt<=0)&&(irq_gpio_value))
		{
			pr_err("%s : fw update fail ,irq_gpio_value = %d\n",__func__,irq_gpio_value);
			//goto attiny_continue;
			goto normal_work;
			
		}
		else
		{
			update_fw_count++;
			if(update_fw_count > 2)
			{
				pr_err("%s update FW  2 failed \n",__func__);
				//goto attiny_continue;	
				goto normal_work;
			}
			err = attiny1617_update_fw(data);
			if(err < 0)
			{
				pr_err("  upgrade ATTINY1617 FW failed: %s\n",__func__);
				goto attiny_continue;
				
			}
			else
			{
				msleep(50);
				waittime_cnt = 50;
				irq_gpio_value = gpio_get_value(data->pdata->irq_gpio);
				pr_err("%s check int clk irq_gpio_value = %d\n",__func__,irq_gpio_value);
				while((!irq_gpio_value)&&(waittime_cnt--))
				{
					msleep(50);
					irq_gpio_value=  gpio_get_value(data->pdata->irq_gpio);
					pr_err("%s : irq_gpio_value = %d\n",__func__,irq_gpio_value);
				}		
				if((waittime_cnt<=0)&&(!irq_gpio_value))
				{
					pr_err("%s : after fw update  ,irq_gpio_value change to high fail\n",__func__);
					goto attiny_continue;	
					//goto normal_work;
				}

				pr_err("%s pinctrl_select  boot mode  pull  \n",__func__);
				err = pinctrl_select_state(data->tv_pinctrl,
					data->pinctrl_state_bootomde_pullup_active);
				if (err < 0) 
				{
					dev_err(&client->dev,"failed to select pin to active state");
				}
				gpio_direction_output(data->pdata->boot_mode_gpio,1);
				gpio_set_value(data->pdata->boot_mode_gpio,1);
				pr_err("%s pull high boot_mode_gpio after fw update\n",__func__);
				msleep(6);
				goto attiny_start;
			   //goto normal_work;
				
			}
		}
	}
normal_work:
	msleep(200);
	cmdframe = send_chip_number_cmd;
	cmd=0x01;
	ret = attiny1617_send_cmd(data->client,cmdframe,sizeof(send_chip_number_cmd));
	msleep(100);
	ret=attiny1617_read_frame(data->client,cmd,chippartnumber);
	pr_err(" chippartnumber: \n");
	for(i=0;i<11;i++)
	{
		if(i != 1)
			printk("%c ",chippartnumber[i]);
		else
			printk("%d ",chippartnumber[i]);
	}
	printk("\n");
	cmdframe = send_fw_version_cmd;
	cmd=0x02;
	ret = attiny1617_send_cmd(data->client,cmdframe,sizeof(send_fw_version_cmd));
	msleep(100);
	ret=attiny1617_read_frame(data->client,cmd,readbuf);
	pr_err(" fw version: \n");
	for(i=0;i<11;i++)
	{
		if(i != 1)
		{
			printk("%c ",readbuf[i]);
		}
		else
		{
			printk("%d ",readbuf[i]);
		}	
	}
	for(i=0;i<8;i++)
	{
		if(readbuf[2+i]!=FW_INFOR[i])
		{
			if(update_fw_count > 3)
			{
				pr_err("%s update FW  3 failed \n",__func__);
				goto attiny_continue;	
			}
			update_fw_count++;

			 	
			pr_err("%s FW infor is not right,need update fw \n",__func__);
			pr_err("%s pinctrl_select set boot mode  no pull  \n",__func__);
			err = pinctrl_select_state(data->tv_pinctrl,
					data->pinctrl_state_bootomde_active);
			if (err < 0) 
			{
				dev_err(&client->dev,"failed to select pin to active state");
			}
			gpio_direction_output(data->pdata->boot_mode_gpio,0);
			gpio_set_value(data->pdata->boot_mode_gpio,0);
			pr_err("%s pull low boot_mode_gpio to  update fw \n",__func__);
			msleep(200);
			err = attiny1617_update_fw(data);
			if(err < 0)
			{
				pr_err("  upgrade ATTINY1617 FW failed: %s\n",__func__);
				goto attiny_continue;
				
			}
			pr_err("%s pinctrl_select  boot mode no pull  \n",__func__);
			err = pinctrl_select_state(data->tv_pinctrl,
			data->pinctrl_state_bootomde_pullup_active);
			if (err < 0) 
			{
				dev_err(&client->dev,"failed to select pin to active state");
			}
			gpio_direction_output(data->pdata->boot_mode_gpio,1);
			gpio_set_value(data->pdata->boot_mode_gpio,1);
			pr_err("%s pull high boot_mode_gpio after fw update\n",__func__);
			msleep(6);

			goto attiny_start;
		}
	}
	printk("\n attiny enter normal working mode : fw is right \n");

	
	#elif defined ATTINY1617_LOOP_WAIT
	//just use debug ,wait new FW and bootloader
	msleep(100);
	while(check_workcnt--)
	{
		pr_err("%s check int clk ,check_workcnt= %d\n",__func__,check_workcnt);
	
		t_toggle_cnt = 0;
		touch_int_toggle = 0;
		
		n_delay = 200000;
		pr_err(" %s  attiny1617 check bootloader mode \n",__func__);
		while(n_delay && t_toggle_cnt < 4)
		{
			n_delay--;
			if((gpio_get_value(data->pdata->irq_gpio) == 0) && !touch_int_toggle)
			{
				//pr_err("%s touch_int_toggle =  %d, irq_gpio == 0\n",__func__,touch_int_toggle);
				touch_int_toggle = 1;				
			}
			else if((gpio_get_value(data->pdata->irq_gpio) == 1) && touch_int_toggle)
			{
				//pr_err("%s touch_int_toggle =  %d, irq_gpio == 1\n",__func__,touch_int_toggle);
				t_toggle_cnt++;
				touch_int_toggle = 0;
			}						
		}	
		pr_err(" %s  attiny1617 end check bootloader mode,t_toggle_cnt = %d \n",__func__,t_toggle_cnt);
		//break;  just use to test bootloader 
		
		if(t_toggle_cnt>=3)
		{
			pr_err(" %s  attiny1617 enter normal working \n",__func__);
			bootmode_gpio_value =  gpio_get_value(data->pdata->boot_mode_gpio);
			pr_err("%s : bootmode_gpio_value = %d\n",__func__,bootmode_gpio_value);
			if(bootmode_gpio_value == 0)
			{
				pr_err(" IC enter bootloader mode,  wait upgrade ATTINY1617 FW: %s\n",__func__);

				err = attiny1617_update_fw(data);
				if(err < 0)
				{
					pr_err("  upgrade ATTINY1617 FW failed: %s\n",__func__);
					continue;
				}
				
				msleep(50);
				waittime_cnt = 20;
				irq_gpio_value = gpio_get_value(data->pdata->irq_gpio);
				pr_err("%s check int clk irq_gpio_value = %d\n",__func__,irq_gpio_value);
				while((!irq_gpio_value)&&(waittime_cnt--))
				{
					msleep(50);
					irq_gpio_value=  gpio_get_value(data->pdata->irq_gpio);
					pr_err("%s : irq_gpio_value = %d\n",__func__,irq_gpio_value);
				}
				
				if((waittime_cnt<=0)&&(!irq_gpio_value))
				{
					pr_err("%s : fw update fail ,irq_gpio_value = %d\n",__func__,irq_gpio_value);
					break;
				}

				waittime_cnt = 50;
				bootmode_gpio_value =  gpio_get_value(data->pdata->boot_mode_gpio);
				pr_err("%s : bootmode_gpio_value = %d\n",__func__,bootmode_gpio_value);
				while((!bootmode_gpio_value)&&(waittime_cnt--))
				{
					msleep(50);
					bootmode_gpio_value =  gpio_get_value(data->pdata->boot_mode_gpio);
					pr_err("%s : bootmode_gpio_value = %d\n",__func__,bootmode_gpio_value);
				}
				
				if((waittime_cnt<=0)&&(!bootmode_gpio_value))
				{
					pr_err("after update fw , the bootmode signal change to H fail: %s\n",__func__);
					break;
				}
				pr_err("reset attiny1617 after  upgrade ATTINY1617 FW: %s\n",__func__);
				reset_attiny1617(data);
				msleep(50);
				continue;	
			}
			
			pr_err(" %s  IC enter normal working \n",__func__);

	
			
			cmdframe = send_chip_number_cmd;
			cmd=0x01;
			ret=attiny1617_send_cmd(data->client,cmdframe,sizeof(send_chip_number_cmd));
			if(ret<0)
			{
				pr_err(" send send_chip_number_cmd 1 err:s\n");
				break;
			}
			msleep(500);
			ret=attiny1617_read_frame(data->client,cmd,chippartnumber);
			pr_err(" chippartnumber: \n");
			for(i=0;i<12;i++)
			{
				printk("%c ",chippartnumber[i]);
			}
			printk("\n");
			
			
			pr_err(" chippartnumber: %s\n",chippartnumber);
			break;



		}
		else 
		{
				//just for test 
			if(check_workcnt==0)
			{
				msleep(1000);
				#if 1
				cmdframe = send_chip_number_cmd;
				cmd=0x01;
				ret=attiny1617_send_cmd(data->client,cmdframe,sizeof(send_chip_number_cmd));
				if(ret<0)
				{
					pr_err(" send send_chip_number_cmd 1 err:s\n");
					break;
				}
				msleep(100);
				ret=attiny1617_read_frame(data->client,cmd,chippartnumber);
				pr_err(" chippartnumber: \n");
				for(i=0;i<12;i++)
				{
					printk("%d ",chippartnumber[i]);
				}
				printk("\n");
				
				
				#endif
				pr_err(" chippartnumber: %s\n",chippartnumber);
				break;

			}
			pr_err(" 2 wait upgrade ATTINY1617 FW: %s\n",__func__);
			bootmode_gpio_value =  gpio_get_value(data->pdata->boot_mode_gpio);
			pr_err("%s : bootmode_gpio_value = %d\n",__func__,bootmode_gpio_value);

			//while(gpio_get_value(data->pdata->irq_gpio));
			irq_gpio_value = gpio_get_value(data->pdata->irq_gpio);
			pr_err("%s check int clk irq_gpio_value = %d\n",__func__,irq_gpio_value);
			waittime_cnt = 20;
			while((irq_gpio_value)&&(waittime_cnt--))
			{
				msleep(50);
				irq_gpio_value=  gpio_get_value(data->pdata->irq_gpio);
				pr_err("%s : irq_gpio_value = %d\n",__func__,irq_gpio_value);
			}
			pr_err("%s check int clk irq_gpio_value = %d,waittime_cnt=%d\n",__func__,irq_gpio_value,waittime_cnt);
			if((waittime_cnt<0)&&(irq_gpio_value))
			{
				pr_err("%s :wait irq chang to low level fail. irq_gpio_value = %d\n",__func__,irq_gpio_value);
				break;
			}
				
			err = attiny1617_update_fw(data);
			if(err < 0)
			{
				pr_err("  upgrade ATTINY1617 FW failed: %s\n",__func__);
				continue;
			}
			//while(!(gpio_get_value(data->pdata->boot_mode_gpio)));
			msleep(50);
			waittime_cnt = 50;
			irq_gpio_value = gpio_get_value(data->pdata->irq_gpio);
			pr_err("%s check int clk irq_gpio_value = %d\n",__func__,irq_gpio_value);
			while((!irq_gpio_value)&&(waittime_cnt--))
			{
				msleep(50);
				irq_gpio_value=  gpio_get_value(data->pdata->irq_gpio);
				pr_err("%s : irq_gpio_value = %d\n",__func__,irq_gpio_value);
			}		
			if((waittime_cnt<=0)&&(!irq_gpio_value))
			{
				pr_err("%s : after fw update  ,irq_gpio_value change to high fail\n",__func__);
				break;
			}

			
			waittime_cnt = 50;
			
			bootmode_gpio_value =  gpio_get_value(data->pdata->boot_mode_gpio);
			pr_err("%s : bootmode_gpio_value = %d\n",__func__,bootmode_gpio_value);
			while((!bootmode_gpio_value)&&(waittime_cnt--))
			{
				msleep(50);
				bootmode_gpio_value =  gpio_get_value(data->pdata->boot_mode_gpio);
				pr_err("%s : bootmode_gpio_value = %d\n",__func__,bootmode_gpio_value);
			}
			if((waittime_cnt<=0)&&(!bootmode_gpio_value))
			{
				pr_err("after update fw , the bootmode signal change to H fail: %s\n",__func__);
				break;
			}

			pr_err("reset attiny1617 after  upgrade ATTINY1617 FW: %s\n",__func__);
			reset_attiny1617(data);
			msleep(50);
			continue;
		}
			
	}
	#endif 

	cmdframe = send_chip_number_cmd;
	cmd=0x01;
	ret = attiny1617_send_cmd(data->client,cmdframe,sizeof(send_chip_number_cmd));
	msleep(100);
	ret=attiny1617_read_frame(data->client,cmd,chippartnumber);
	pr_err(" chippartnumber: \n");
	for(i=0;i<11;i++)
	{
		if(i != 1)
			printk("%c ",chippartnumber[i]);
		else
			printk("%d ",chippartnumber[i]);
	}

	printk("\n");
	cmdframe = send_fw_version_cmd;
	cmd=0x02;
	ret = attiny1617_send_cmd(data->client,cmdframe,sizeof(send_fw_version_cmd));
	msleep(100);
	ret=attiny1617_read_frame(data->client,cmd,readbuf);
	pr_err(" fw version: \n");
	for(i=0;i<11;i++)
	{
		if(i != 1)
		{
			printk("%c ",readbuf[i]);
		}
		else
		{
			printk("%d ",readbuf[i]);
		}	
	}

	printk("end \n");

	cmdframe= send_slide_position_cmd;
	
	cmd = CMD_SLIDE_POSITION; 
	ret=attiny1617_send_cmd(data->client,cmdframe,sizeof(send_slide_position_cmd));
	if(ret<0)
	{
		pr_err("%s: send slide position cmd err\n", __func__);
	}
	msleep(100);
	ret=attiny1617_read_frame(data->client,cmd,readbuf);
	// mutex_unlock(&attiny_mutex);
	msleep(100);
	pr_err("%s: touch position status = %d, positon=%d\n", __func__,readbuf[2],readbuf[3]);
	//msleep(20000);//just test gpio
	#ifdef ATTINY_WAKE_LOCK
	pr_err("wakelock and start attiny_timer: %s\n",__func__);
	wake_lock(&attiny_wake_lock);
	attiny_timer.expires = jiffies + msecs_to_jiffies(200);
	add_timer(&attiny_timer);
	#endif
	
attiny_continue:	
	err = request_threaded_irq(client->irq, NULL,
				attiny1617_tv_interrupt,
	/*
	* the interrupt trigger mode will be set in Device Tree with property
	* "interrupts", so here we just need to set the flag IRQF_ONESHOT
	*/
				pdata->irqflags | IRQF_ONESHOT | IRQF_TRIGGER_FALLING/*|IRQF_TRIGGER_RISING*/,
				client->dev.driver->name, data);
	if (err) {
		dev_err(&client->dev, "request irq failed\n");
		goto free_gpio;
	}
	enable_irq_wake(client->irq);
	//tv_key_state = 0;
	

	return 0;

free_gpio:
	if (gpio_is_valid(pdata->reset_gpio))
		gpio_free(pdata->reset_gpio);
	if (gpio_is_valid(pdata->irq_gpio))
		gpio_free(pdata->irq_gpio);
#if 0
err_gpio_req:
	if (data->ts_pinctrl) {
		if (IS_ERR_OR_NULL(data->pinctrl_state_release)) {
			devm_pinctrl_put(data->tv_pinctrl);
			data->tv_pinctrl = NULL;
		} else {
			err = pinctrl_select_state(data->tv_pinctrl,
					data->pinctrl_state_release);
			if (err)
				pr_err("failed to select relase pinctrl state\n");
		}
	}
#endif
	if (pdata->power_on)
		pdata->power_on(false);
	else
		attiny1617_power_on(data, false);
pwr_deinit:
	if (pdata->power_init)
		pdata->power_init(false);
	else
		attiny1617_power_init(data, false);
unreg_inputdev:
	input_unregister_device(input_dev);
	return err;
}

static int attiny1617_tv_remove(struct i2c_client *client)
{
	struct attiny1617_tv_data *data = i2c_get_clientdata(client);
	int retval;


	free_irq(client->irq, data);

	if (gpio_is_valid(data->pdata->reset_gpio))
		gpio_free(data->pdata->reset_gpio);

	if (gpio_is_valid(data->pdata->irq_gpio))
		gpio_free(data->pdata->irq_gpio);

	if (data->tv_pinctrl) {
		if (IS_ERR_OR_NULL(data->pinctrl_state_release)) {
			devm_pinctrl_put(data->tv_pinctrl);
			data->tv_pinctrl = NULL;
		} else {
			retval = pinctrl_select_state(data->tv_pinctrl,
					data->pinctrl_state_release);
			if (retval < 0)
				pr_err("failed to select release pinctrl state\n");
		}
	}


	if (data->pdata->power_on)
		data->pdata->power_on(false);
	else
		attiny1617_power_on(data, false);

	if (data->pdata->power_init)
		data->pdata->power_init(false);
	else
		attiny1617_power_init(data, false);

	input_unregister_device(data->input_dev);
	return 0;
}

static const struct i2c_device_id attiny1617_tv_id[] = {
	{"attiny1617", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, attiny1617_tv_id);


static struct of_device_id attiny1617_match_table[] = {
	{ .compatible = "attiny,attiny1617",},
	{ },
};


static struct i2c_driver attiny1617_tv_driver = {
	.probe = attiny1617_tv_probe,
	.remove = attiny1617_tv_remove,
	.driver = {
		   .name = "attiny1617",
		   .owner = THIS_MODULE,
		.of_match_table = attiny1617_match_table,
		   },
	.id_table = attiny1617_tv_id,
};

static int __init attiny1617_tv_init(void)
{
	return i2c_add_driver(&attiny1617_tv_driver);
}
module_init(attiny1617_tv_init);

static void __exit attiny1617_tv_exit(void)
{
	i2c_del_driver(&attiny1617_tv_driver);
}
module_exit(attiny1617_tv_exit);

MODULE_DESCRIPTION("ATTINY1617 TouchVolume driver");
MODULE_LICENSE("GPL v2");
