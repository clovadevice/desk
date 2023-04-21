
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/firmware.h>
#include <linux/regmap.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>


#include "tas5782m.h"
#include "tas5782m-misc.h"

#define MODULE_NAME "tas2559"

extern struct tas5782m_priv* getTAS5782M(void);


#if defined(CONFIG_SND_SOC_TAS5782)/*by dusin.jang at 20180510*/
 int get_volume_misc_index(int vol)
{
	pr_info("[%s]\n", __func__);
   //output : 0  -- 134
   return ((TAS5782M_VOLUME_MAX + TAS5782M_VOLUME_OFFSET) - vol);
}



int tas5782m_reg_write(unsigned int reg, unsigned int value)
{
	struct tas5782m_priv *tas5782m = getTAS5782M();
	int ret = 0;

	dev_info(tas5782m->dev, "%s, BOOK=0x%x, PAGE=0x%x, reg:0x%x, value:0x%x\n", __func__, TAS5782M_BOOK_ID(reg), TAS5782M_PAGE_ID(reg), TAS5782M_PAGE_REG(reg), value);

	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_PAGE_REG(reg), value);


	return ret;


}

int tas5782m_reg_read(unsigned int reg, unsigned int *pValue)
{
	struct tas5782m_priv *tas5782m = getTAS5782M();
	int ret = 0;
	unsigned char value = 0;

	ret = tas5782m_i2c_read_device(tas5782m, tas5782m->devA_addr, TAS5782M_PAGE_ID(reg), &value);
	if(ret >=0)
		*pValue = value;

	dev_info(tas5782m->dev, "%s, reg=0x%x, value:0x%x\n", __func__, TAS5782M_PAGE_ID(reg), value);

	return ret;

}



static void tas5782m_misc_set_volume(int db)
{
	struct tas5782m_priv *tas5782m = getTAS5782M();

    unsigned int index;
	uint32_t volume_hex;
	uint8_t byte4;
	uint8_t byte3;
	uint8_t byte2;
	uint8_t byte1;


	if(db > (TAS5782M_VOLUME_MAX + TAS5782M_VOLUME_OFFSET)) 
		db = (TAS5782M_VOLUME_MAX + TAS5782M_VOLUME_OFFSET);
	else if(db < TAS5782M_VOLUME_MIN + TAS5782M_VOLUME_OFFSET) 
		db = (TAS5782M_VOLUME_MIN + TAS5782M_VOLUME_OFFSET);

	tas5782m->volume_db = db;

	index = get_volume_misc_index(db);
	
	//pr_info("[%s] %d(Index), %d(dB), %d(Vol)\n", __func__, index, (TAS5782M_VOLUME_MAX - index),
	//		(TAS5782M_VOLUME_OFFSET + TAS5782M_VOLUME_MAX - index));
	dev_info(tas5782m->dev, "[%s] %d(Index), %d(dB), %d(Vol)\n", __func__, index, (TAS5782M_VOLUME_MAX - index),
			(TAS5782M_VOLUME_OFFSET + TAS5782M_VOLUME_MAX - index));

	volume_hex = tas5782m_misc_volume[index];

	byte4 = ((volume_hex >> 24) & 0xFF);
	byte3 = ((volume_hex >> 16) & 0xFF);
	byte2 = ((volume_hex >> 8)	& 0xFF);
	byte1 = ((volume_hex >> 0)	& 0xFF);

	//w 90 00 00
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_00, TAS5782M_PAGE_00);
	//w 90 7f 8c	
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_7F, TAS5782M_PAGE_8C);
	//w 90 00 1e
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_00, TAS5782M_PAGE_1E);
	//w 90 44 xx xx xx xx 
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_44, byte4);
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_45, byte3);
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_46, byte2);
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_47, byte1);
	
	//w 90 00 00
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_00, TAS5782M_PAGE_00);
	//w 90 7f 8c
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_7F, TAS5782M_PAGE_8C);	
	//w 90 00 1e
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_00, TAS5782M_PAGE_1E);	
	//w 90 48 xx xx xx xx 
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_48, byte4);
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_49, byte3);
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_4A, byte2);
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_4B, byte1);
	
	//w 90 00 00
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_00, TAS5782M_PAGE_00);	
	//w 90 7f 8c
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_7F, TAS5782M_PAGE_8C);	
	//w 90 00 23
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_00, TAS5782M_PAGE_23);	
	//w 90 14 00 00 00 01 
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_14, 0x00);
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_15, 0x00);
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_16, 0x00);
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_17, 0x01);
}


static int tas5782m_misc_sw_mute(int mute)
{
	struct tas5782m_priv *tas5782m = getTAS5782M();
	u8 reg3_value = 0;
		
	//pr_info("[%s] mute: %d\n", __func__, mute);
	dev_info(tas5782m->dev, "[%s] mute: %d\n", __func__, mute);	
	
	if (mute) reg3_value = 0x11;

	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_00, 0x00);
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_7F, 0x00);
	tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_REG_03, reg3_value);
	
	if (mute) {
		tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, 0x02, 0x01);
	} else {
		tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, 0x02, 0x00);
	}

	tas5782m->sw_mute = mute;
	
	return 0;
}
#endif /*CONFIG_SND_SOC_TAS5782*/


static ssize_t tas5782m_misc_read(struct file *file, char *buf, 
						size_t count, loff_t *ppos)
{
	struct tas5782m_priv *tas5782m = getTAS5782M();
	int ret = 0;
	unsigned int nValue = 0;
	unsigned char value = 0;
	unsigned char *p_kBuf = NULL;

	switch(tas5782m->cmd)
	{
	case TIAUDIO_CMD_REG_READ:
		//pr_info("[%s] TIAUDIO_CMD_REG_READ: chn[%d], current_reg = 0x%x, count=%d\n", __func__, tas5782m->current_ch, tas5782m->current_reg, (int)count);
		dev_info(tas5782m->dev, "%s, chn[%d], current_reg = 0x%x, count=%d\n", __func__, tas5782m->current_ch, tas5782m->current_reg, (int)count);
		
		if(count == 1) {
			//ret = tas5782m->read(tas5782m, tas5782m->current_ch, tas5782m->current_reg, &nValue);
			ret = tas5782m_reg_read(tas5782m->current_reg, &nValue);
			if(ret < 0) {
				pr_err("[%s] read fail\n", __func__);
				break;
			}
			
			value = (u8)nValue;
			//pr_info("[%s] TIAUDIO_CMD_REG_READ: nValue=0x%x, value=0x%x\n", __func__, nValue, value);
			dev_info(tas5782m->dev, "%s, nValue=0x%x, value=0x%x\n", __func__, nValue, value);
			
			ret = copy_to_user(buf, &value, 1);
			if(ret != 0)
				pr_err("[%s] copy to user fail\n", __func__);
		} else if(count > 1) {
			p_kBuf = kzalloc(count, GFP_KERNEL);
			if (p_kBuf != NULL) {
				//ret = tas5782m->bulk_read(tas5782m, tas5782m->current_ch, tas5782m->current_reg, p_kBuf, count);
				if(ret >= 0) {
					ret = copy_to_user(buf, p_kBuf, count);
					if(ret != 0)
						pr_err("[%s] copy to user fail\n", __func__);
				} 
				kfree(p_kBuf);
			}
			 else {
				pr_err("[%s] read no mem\n", __func__);
			}
		}
		break;
#if defined(CONFIG_SND_SOC_TAS5782)/*by dusin.jang at 20180510*/
	case TIAUDIO_CMD_PROGRAM:		
	case TIAUDIO_CMD_CONFIGURATION:		
	case TIAUDIO_CMD_FW_TIMESTAMP:		
	case TIAUDIO_CMD_CALIBRATION:
	case TIAUDIO_CMD_SAMPLERATE:
	case TIAUDIO_CMD_BITRATE:
		pr_info("[%s]Not Support\n", __func__);break;
	case TIAUDIO_CMD_MUTE:
		value=(u8)tas5782m->sw_mute;
		ret = copy_to_user(buf, &value, 1);
		if(ret != 0)
			pr_err("[%s] copy to user fail\n", __func__);
		break;
	case TIAUDIO_CMD_DACVOLUME:
		value=(u8)tas5782m->volume_db;
		
		ret = copy_to_user(buf, &value, 1);
		if(ret != 0)
			pr_err("[%s] copy to user fail\n", __func__);
		
		pr_info("[%s] Vol:%d\n", __func__, (int)value);
		break;
#endif/*CONFIG_SND_SOC_TAS5782*/
	default:
		break;
	}

	tas5782m->cmd = 0;

	pr_info("[%s]\n", __func__);
	return count;
}

static ssize_t tas5782m_misc_write(struct file *file, const char __user *buf, 
						size_t count, loff_t *ppos)
{
	struct tas5782m_priv *tas5782m = getTAS5782M();
	unsigned char *p_kBuf = NULL;
	int ret = 0;
	unsigned int reg = 0, len = 0;
	enum channel ch;
#if defined(CONFIG_SND_SOC_TAS5782)/*by dusin.jang at 20180510*/
	int value;			
#endif

	//pr_info("[%s]\n", __func__);
	dev_info(tas5782m->dev, "%s\n", __func__);


	p_kBuf = kzalloc(count, GFP_KERNEL);
	if (p_kBuf == NULL) {
		pr_err("[%s] write no mem\n", __func__);
		goto err;
	}

	ret = copy_from_user(p_kBuf, buf, count);
	if(ret != 0) {
		pr_err("[%s] copy_from_user failed...\n", __func__);
		goto err;
	}

	tas5782m->cmd = p_kBuf[0];

	switch(tas5782m->cmd)
	{
	case TIAUDIO_CMD_REG_WITE:
		if (count > 6)  {
			ch = p_kBuf[1];
			reg = ((unsigned int)p_kBuf[2] << 24) +  
					((unsigned int)p_kBuf[3] << 16) +
					((unsigned int)p_kBuf[4] << 8) +
					((unsigned int)p_kBuf[5]);
			
			len = count - 6;
			
			if(len == 1) {
				//ret = tas5782m->write(tas5782m, ch, reg, p_kBuf[6]);
				//ret = tas5782m_dev_write(tas5782m, ch, reg, p_kBuf[6]);
				ret = tas5782m_reg_write(reg, p_kBuf[6]);

				//pr_info("[%s] TIAUDIO_CMD_REG_WITE,chn[%d], Reg=0x%x, Val=0x%x\n", __func__, ch, reg, p_kBuf[6]);
				dev_info(tas5782m->dev, "%s, ch:%d, Reg=0x%x, Val=0x%x\n", __func__, ch, reg, p_kBuf[6]);
				
			} 
			else {
				ret = tas5782m->bulk_write(tas5782m, ch, reg, &p_kBuf[6], len);	
			}
		} else {
			pr_err("[%s] write len fail, count=%d\n", __func__, (int)count);
		}
		tas5782m->cmd = 0;
		break;
	case TIAUDIO_CMD_REG_READ:
		if (count == 6) {
			tas5782m->current_ch = p_kBuf[1];
			tas5782m->current_reg = ((unsigned int)p_kBuf[2] << 24) +
									((unsigned int)p_kBuf[3] << 16) +
									((unsigned int)p_kBuf[4] << 8) +
									((unsigned int)p_kBuf[5]);

			//pr_info("[%s] TIAUDIO_CMD_REG_READ chl[%d], whole=0x%x\n", __func__, tas5782m->current_ch, tas5782m->current_reg);
			dev_info(tas5782m->dev, "%s, chl[%d], whole=0x%x\n", __func__, tas5782m->current_ch, tas5782m->current_reg);
		} else {
			pr_err("[%s] read len fail..\n", __func__);
		}
		break;
#if defined(CONFIG_SND_SOC_TAS5782)/*by dusin.jang at 20180510*/
	case TIAUDIO_CMD_DACVOLUME:
			value = (int)p_kBuf[2];			
			pr_info("[%s] vol:%d\n", __func__, value);
			
			tas5782m_misc_set_volume(value);
		break;
	case TIAUDIO_CMD_MUTE:
			value = (int)p_kBuf[2]; 					
			tas5782m_misc_sw_mute(value);
		break;
	case TIAUDIO_CMD_DEBUG_ON:
	case TIAUDIO_CMD_PROGRAM:
	case TIAUDIO_CMD_CONFIGURATION:
	case TIAUDIO_CMD_FW_TIMESTAMP:
	case TIAUDIO_CMD_SAMPLERATE:
	case TIAUDIO_CMD_BITRATE:
	case TIAUDIO_CMD_SPEAKER:
	case TIAUDIO_CMD_FW_RELOAD:		
		pr_info("[%s]Not Support\n", __func__);break;
#endif/*CONFIG_SND_SOC_TAS5782*/

	default:
		tas5782m->cmd = 0;
		break;
	}	

err:
	
	if(p_kBuf != NULL)
		kfree(p_kBuf);	

	return count;
}

static int tas5782m_misc_open(struct inode *inode, struct file *file)
{
	struct tas5782m_priv *tas5782m = getTAS5782M();

	if (!try_module_get(THIS_MODULE))
        return -ENODEV;

	if(tas5782m == NULL) {
		pr_err("[%s] tas5782m_priv is null\n", __func__);
		return -ENODEV;
	}

	file->private_data = (void *)tas5782m;

	pr_info("[%s]\n", __func__);
	return 0;
}

static int tas5782m_misc_close(struct inode *inodep, struct file *file)
{
	//struct tas5782m_priv *tas5782m = getTAS5782M();

	pr_info("[%s]\n", __func__);

	file->private_data = (void *)NULL;
	module_put(THIS_MODULE);

	return 0;
}

static const struct file_operations tas5782m_misc_fops = {
    .owner = THIS_MODULE,
    .read = tas5782m_misc_read,
    .write = tas5782m_misc_write,
    .open = tas5782m_misc_open,
    .release = tas5782m_misc_close,
};

struct miscdevice tas5782m_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = MODULE_NAME,
    .fops = &tas5782m_misc_fops,
};

static int __init tas5782m_misc_init(void)
{
	int ret = 0;

	ret = misc_register(&tas5782m_misc_device);
	if(ret) {
		pr_err("[%s] misc register fail...\n", __func__);
		return ret;
	}

	return 0;
}

static void __exit tas5782m_misc_exit(void)
{
	misc_deregister(&tas5782m_misc_device);
}

module_init(tas5782m_misc_init)
module_exit(tas5782m_misc_exit)

MODULE_AUTHOR("Texas Instruments Inc.");
MODULE_DESCRIPTION("TAS2559 Misc Smart Amplifier driver");
MODULE_LICENSE("GPL v2");

