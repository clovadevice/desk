/*
 * Driver for the TAS5782M Audio Amplifier
 *
 * Author: Andy Liu <andy-liu@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/regmap.h>
#include <linux/delay.h>
#if defined(CONFIG_TARGET_PRODUCT_IF_S200N)
#include <linux/clk.h>
#endif

#include <sound/soc.h>
#include <sound/pcm.h>
#include <sound/initval.h>

#include "tas5782m.h"

#if defined(CONFIG_TARGET_PRODUCT_IF_S700N) || defined(CONFIG_TARGET_PRODUCT_IF_MKT200)
#define USE_SWITCHABLE_EQ_TABLE
#define EQ_TABLE_NORMAL		0
#define EQ_TABLE_VOIP		1
#endif

#if defined(CONFIG_TARGET_PRODUCT_IF_S700N) || defined(CONFIG_TARGET_PRODUCT_IF_MKT200)
//#include "tas5782m-reg-20180611.h"
//#include "tas5782m-reg-20180807.h"
//#include "tas5782m-reg-20180821.h"
#include "tas5782m-reg-20180906.h"
#else //CONFIG_TARGET_PRODUCT_IF_S200N
#include "tas5782m-reg-20171129.h"
//#include "tas5782m-reg-20180402.h"
#endif



#define TAS5872M_DRV_NAME    "tas5782m"

#define TAS5782M_VOLUME_OFFSET 110
#define TAS5782M_VOLUME_MIN -110
#define TAS5782M_VOLUME_MAX 24
#define TAS5782M_VOLUME_DEFAULT (-6 + TAS5782M_VOLUME_OFFSET)

#define TAS5872M_RATES	     (SNDRV_PCM_RATE_48000)
#define TAS5782M_FORMATS     (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
			                  SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

#define TAS5782M_REG_00      (0x00)
#define TAS5782M_REG_03      (0x03)
#define TAS5782M_REG_7F      (0x7F)
#define TAS5782M_REG_14      (0x14)
#define TAS5782M_REG_15      (0x15)
#define TAS5782M_REG_16      (0x16)
#define TAS5782M_REG_17      (0x17)

#define TAS5782M_REG_44      (0x44)
#define TAS5782M_REG_45      (0x45)
#define TAS5782M_REG_46      (0x46)
#define TAS5782M_REG_47      (0x47)
#define TAS5782M_REG_48      (0x48)
#define TAS5782M_REG_49      (0x49)
#define TAS5782M_REG_4A      (0x4A)
#define TAS5782M_REG_4B      (0x4B)

#define TAS5782M_PAGE_00     (0x00)
#define TAS5782M_PAGE_8C     (0x8C)     
#define TAS5782M_PAGE_1E     (0x1E)
#define TAS5782M_PAGE_23     (0x23)

#define CFG_META_SWITCH 	(255)
#define CFG_META_DELAY  	(254)  
#define CFG_META_BURST  	(253)

#define BUF_MAX 300

static char tmp_buf[BUF_MAX] = {0,};

#if defined(CONFIG_TARGET_PRODUCT_IF_S200N) || defined(CONFIG_TARGET_PRODUCT_IF_S700N) || defined(CONFIG_TARGET_PRODUCT_IF_MKT200)
cfg_reg registers_5782_power_up[] = {
	{ .command=0x02, .param=0x00 },
};

cfg_reg registers_5782_power_down[] = {
	{ .command=0x02, .param=0x01 },
};

cfg_reg registers_5782_mute[] = {
    { .command=0x00, .param=0x00 },
	{ .command=0x7f, .param=0x00 },
	{ .command=0x03, .param=0x11 },
};

#if defined(CONFIG_TARGET_PRODUCT_IF_S700N) || defined(CONFIG_TARGET_PRODUCT_IF_MKT200)
cfg_reg registers_5782_pre_init[] = {
	{ .command=0x00, .param=0x00 },
	{ .command=0x7f, .param=0x00 },
	{ .command=0x02, .param=0x11 },
	{ .command=0x01, .param=0x11 },
	{ .command=0x00, .param=0x00 },
	{ .command=0x00, .param=0x00 },
	{ .command=0x00, .param=0x00 },
	{ .command=0x00, .param=0x00 },
	{ .command=0x03, .param=0x11 },
	{ .command=0x2a, .param=0x00 },
	{ .command=0x25, .param=0x18 },
	{ .command=0x0d, .param=0x10 },
	{ .command=0x02, .param=0x00 },
	{ .command=0x00, .param=0x00 },
	{ .command=0x7f, .param=0x00 },
};

cfg_reg registers_5782_soft_reset[] = {
	{ .command=0x02, .param=0x80 },
};
#endif
void tas5782m_init_reg(void);
void tas5782_powerdown_request(bool enable);
#endif

static int tas5782m_mute(struct snd_soc_dai *dai, int mute);

const uint32_t tas5782m_volume[] = {
	0x07ECA9CD,    //24dB
	0x07100C4D,    //23dB
	0x064B6CAE,    //22dB
	0x059C2F02,    //21dB
	0x05000000,    //20dB
	0x0474CD1B,    //19dB
	0x03F8BD7A,    //18dB
	0x038A2BAD,    //17dB
	0x0327A01A,    //16dB
	0x02CFCC01,    //15dB
	0x02818508,    //14dB
	0x023BC148,    //13dB
	0x01FD93C2,    //12dB
	0x01C62940,    //11dB
	0x0194C584,    //10dB
	0x0168C0C6,    //9dB
	0x0141857F,    //8dB
	0x011E8E6A,    //7dB
	0x00FF64C1,    //6dB
	0x00E39EA9,    //5dB
	0x00CADDC8,    //4dB
	0x00B4CE08,    //3dB
	0x00A12478,    //2dB
	0x008F9E4D,    //1dB
	0x00800000,    //0dB
	0x00721483,    //-1dB	/*index 25*/
	0x0065AC8C,    //-2dB
	0x005A9DF8,    //-3dB
	0x0050C336,    //-4dB
	0x0047FACD,    //-5dB
	0x004026E7,    //-6dB	/*index 30*/
	0x00392CEE,    //-7dB
	0x0032F52D,    //-8dB
	0x002D6A86,    //-9dB
	0x00287A27,    //-10dB
	0x00241347,    //-11dB
	0x002026F3,    //-12dB
	0x001CA7D7,    //-13dB
	0x00198A13,    //-14dB
	0x0016C311,    //-15dB
	0x00144961,    //-16dB
	0x0012149A,    //-17dB
	0x00101D3F,    //-18dB
	0x000E5CA1,    //-19dB
	0x000CCCCD,    //-20dB
	0x000B6873,    //-21dB
	0x000A2ADB,    //-22dB
	0x00090FCC,    //-23dB
	0x00081385,    //-24dB
	0x000732AE,    //-25dB
	0x00066A4A,    //-26dB
	0x0005B7B1,    //-27dB
	0x00051884,    //-28dB
	0x00048AA7,    //-29dB
	0x00040C37,    //-30dB
	0x00039B87,    //-31dB
	0x00033718,    //-32dB
	0x0002DD96,    //-33dB
	0x00028DCF,    //-34dB
	0x000246B5,    //-35dB
	0x00020756,    //-36dB
	0x0001CEDC,    //-37dB
	0x00019C86,    //-38dB
	0x00016FAA,    //-39dB
	0x000147AE,    //-40dB
	0x0001240C,    //-41dB
	0x00010449,    //-42dB
	0x0000E7FB,    //-43dB
	0x0000CEC1,    //-44dB
	0x0000B845,    //-45dB
	0x0000A43B,    //-46dB
	0x0000925F,    //-47dB
	0x00008274,    //-48dB
	0x00007444,    //-49dB
	0x0000679F,    //-50dB
	0x00005C5A,    //-51dB
	0x0000524F,    //-52dB
	0x0000495C,    //-53dB
	0x00004161,    //-54dB
	0x00003A45,    //-55dB
	0x000033EF,    //-56dB
	0x00002E49,    //-57dB
	0x00002941,    //-58dB
	0x000024C4,    //-59dB
	0x000020C5,    //-60dB
	0x00001D34,    //-61dB
	0x00001A07,    //-62dB
	0x00001733,    //-63dB
	0x000014AD,    //-64dB
	0x0000126D,    //-65dB
	0x0000106C,    //-66dB
	0x00000EA3,    //-67dB
	0x00000D0C,    //-68dB
	0x00000BA0,    //-69dB
	0x00000A5D,    //-70dB
	0x0000093C,    //-71dB
	0x0000083B,    //-72dB
	0x00000756,    //-73dB
	0x0000068A,    //-74dB
	0x000005D4,    //-75dB
	0x00000532,    //-76dB
	0x000004A1,    //-77dB
	0x00000420,    //-78dB
	0x000003AD,    //-79dB
	0x00000347,    //-80dB
	0x000002EC,    //-81dB
	0x0000029A,    //-82dB
	0x00000252,    //-83dB
	0x00000211,    //-84dB
	0x000001D8,    //-85dB
	0x000001A4,    //-86dB
	0x00000177,    //-87dB
	0x0000014E,    //-88dB
	0x0000012A,    //-89dB
	0x00000109,    //-90dB
	0x000000EC,    //-91dB
	0x000000D3,    //-92dB
	0x000000BC,    //-93dB
	0x000000A7,    //-94dB
	0x00000095,    //-95dB
	0x00000085,    //-96dB
	0x00000076,    //-97dB
	0x0000006A,    //-98dB
	0x0000005E,    //-99dB
	0x00000054,    //-100dB
	0x0000004B,    //-101dB
	0x00000043,    //-102dB
	0x0000003B,    //-103dB
	0x00000035,    //-104dB
	0x0000002F,    //-105dB
	0x0000002A,    //-106dB
	0x00000025,    //-107dB
	0x00000021,    //-108dB
	0x0000001E,    //-109dB
	0x0000001B,    //-110dB
};

/*
struct tas5782m_priv {
	struct regmap *regmap;
	
	int ldo_enable_gpio;
	int reset_gpio;
	int amp_vdd_gpio;
	int mute_gpio;
	
	int volume_db;
};
*/

static bool tas5782m_volatile(struct device *pDev, unsigned int nRegister)
{
    return true;
}

static bool tas5782m_writeable(struct device *pDev, unsigned int nRegister)
{
    return true;
}

const struct regmap_config tas5782m_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.writeable_reg = tas5782m_writeable,
	.volatile_reg = tas5782m_volatile,
	.cache_type = REGCACHE_NONE,
	.max_register = 128,
};

static struct tas5782m_priv *g_tas5782m;

struct tas5782m_priv* getTAS5782M(void)
{
	return g_tas5782m;
}
EXPORT_SYMBOL(getTAS5782M);

static int tas5782m_ldo_enable(struct tas5782m_priv *tas5782m, bool enable)
{
	int ret = 0;
		
	pr_info("[%s] - %s\n", __func__, enable?"ON":"OFF");
	
	if (!gpio_is_valid(tas5782m->ldo_enable_gpio)) {
		pr_err("[%s] GPIO is invalid\n", __func__);
		return -EINVAL;
	}
	
	if(enable)
		gpio_direction_output(tas5782m->ldo_enable_gpio, 1);
	else
		gpio_direction_output(tas5782m->ldo_enable_gpio, 0);
	
	mdelay(10);
	
	return ret;
}


static int tas5782m_reset(struct tas5782m_priv *tas5782m, bool enable)
{
	int ret = 0;
	
	pr_info("[%s] - %s\n", __func__, enable?"ON":"OFF");
	
	if (!gpio_is_valid(tas5782m->reset_gpio)) {
		pr_err("[%s] GPIO is invalid\n", __func__);
		return -EINVAL;
	}
	
	if(enable)
		gpio_direction_output(tas5782m->reset_gpio, 1);
	else
		gpio_direction_output(tas5782m->reset_gpio, 0);
	
	mdelay(10);
	
	return ret;
}

#if defined(CONFIG_TARGET_PRODUCT_IF_S700N) || defined(CONFIG_TARGET_PRODUCT_IF_MKT200)
static int tas5782m_amp_vdd_enable(struct tas5782m_priv *tas5782m, bool enable)
{
	int ret = 0;
	
	pr_info("[%s] - %s\n", __func__, enable?"ON":"OFF");
	
	if (!gpio_is_valid(tas5782m->amp_vdd_gpio)) {
		pr_err("[%s] GPIO is invalid\n", __func__);
		return -EINVAL;
	}
	
	if(enable)
		gpio_direction_output(tas5782m->amp_vdd_gpio, 1);
	else
		gpio_direction_output(tas5782m->amp_vdd_gpio, 0);
		
	return ret;
}

static int tas5782m_amp_mute(struct tas5782m_priv *tas5782m, bool enable)
{
	int ret = 0;
	
	pr_info("[%s] - %s\n", __func__, enable?"ON":"OFF");
	
	if (!gpio_is_valid(tas5782m->mute_gpio)) {
		pr_err("[%s] GPIO is invalid\n", __func__);
		return -EINVAL;
	}
	
	if(enable)
		gpio_direction_output(tas5782m->mute_gpio, 1);
	else
		gpio_direction_output(tas5782m->mute_gpio, 0);
	
		
	return ret;
}
#endif


static int tas5782m_vol_info(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_info *uinfo)
{
	pr_info("[%s]\n", __func__);
	uinfo->type   = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->access = SNDRV_CTL_ELEM_ACCESS_TLV_READ | SNDRV_CTL_ELEM_ACCESS_READWRITE;
	uinfo->count  = 1;
	
	/*
	uinfo->value.integer.min  = -110;
	uinfo->value.integer.max  = 24;
	*/
	uinfo->value.integer.min  = TAS5782M_VOLUME_MIN + TAS5782M_VOLUME_OFFSET;
	uinfo->value.integer.max  = TAS5782M_VOLUME_MAX + TAS5782M_VOLUME_OFFSET;
	uinfo->value.integer.step = 1;

	return 0;
}

static inline int get_db(int index)
{
	pr_info("[%s]\n", __func__);
   //input  : 0  -- 134  
   //output : 24 -- -110 --> before
   //output : 134 -- 0 --> after

   //return (24 - index);
   return ((TAS5782M_VOLUME_MAX + TAS5782M_VOLUME_OFFSET) - index);
}

static int tas5782m_vol_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tas5782m_priv *tas5782m = snd_soc_codec_get_drvdata(codec);
	
	pr_info("[%s] Vol: %d dB\n", __func__, (tas5782m->volume_db - TAS5782M_VOLUME_OFFSET));
	
	ucontrol->value.integer.value[0] = tas5782m->volume_db;

	return 0;
}

static inline unsigned int get_volume_index(int vol)
{
	pr_info("[%s]\n", __func__);
   //input  : 24 -- -110
   //output : 0  -- 134
   //return (24 - vol);
   return ((TAS5782M_VOLUME_MAX + TAS5782M_VOLUME_OFFSET) - vol);
}

static void tas5782m_set_volume(struct snd_soc_codec *codec, int db)
{
    unsigned int index;
	uint32_t volume_hex;
	uint8_t byte4;
	uint8_t byte3;
	uint8_t byte2;
	uint8_t byte1;
	
	//pr_info("[%s]\n", __func__);
 
	index = get_volume_index(db);
#if defined(CONFIG_TARGET_PRODUCT_IF_S200N) || defined(CONFIG_TARGET_PRODUCT_IF_S700N) || defined(CONFIG_TARGET_PRODUCT_IF_MKT200)
	pr_info("[%s] %d(Index), %d(dB), %d(Vol)\n", __func__, index, (TAS5782M_VOLUME_MAX - index),
			(TAS5782M_VOLUME_OFFSET + TAS5782M_VOLUME_MAX - index));
#endif
	volume_hex = tas5782m_volume[index];

	byte4 = ((volume_hex >> 24) & 0xFF);
	byte3 = ((volume_hex >> 16) & 0xFF);
	byte2 = ((volume_hex >> 8)	& 0xFF);
	byte1 = ((volume_hex >> 0)	& 0xFF);
#if 1	
	//w 90 00 00
	snd_soc_write(codec, TAS5782M_REG_00, TAS5782M_PAGE_00);
	//w 90 7f 8c
	snd_soc_write(codec, TAS5782M_REG_7F, TAS5782M_PAGE_8C);
	//w 90 00 1e
	snd_soc_write(codec, TAS5782M_REG_00, TAS5782M_PAGE_1E);
	//w 90 44 xx xx xx xx 
	snd_soc_write(codec, TAS5782M_REG_44, byte4);
	snd_soc_write(codec, TAS5782M_REG_45, byte3);
	snd_soc_write(codec, TAS5782M_REG_46, byte2);
	snd_soc_write(codec, TAS5782M_REG_47, byte1);
	
	//w 90 00 00
	snd_soc_write(codec, TAS5782M_REG_00, TAS5782M_PAGE_00);
	//w 90 7f 8c
	snd_soc_write(codec, TAS5782M_REG_7F, TAS5782M_PAGE_8C);	
	//w 90 00 1e
	snd_soc_write(codec, TAS5782M_REG_00, TAS5782M_PAGE_1E);	
	//w 90 48 xx xx xx xx 
	snd_soc_write(codec, TAS5782M_REG_48, byte4);
	snd_soc_write(codec, TAS5782M_REG_49, byte3);
	snd_soc_write(codec, TAS5782M_REG_4A, byte2);
	snd_soc_write(codec, TAS5782M_REG_4B, byte1);
	
	//w 90 00 00
	snd_soc_write(codec, TAS5782M_REG_00, TAS5782M_PAGE_00);	
	//w 90 7f 8c
	snd_soc_write(codec, TAS5782M_REG_7F, TAS5782M_PAGE_8C);	
	//w 90 00 23
	snd_soc_write(codec, TAS5782M_REG_00, TAS5782M_PAGE_23);	
	//w 90 14 00 00 00 01 
	snd_soc_write(codec, TAS5782M_REG_14, 0x00);
	snd_soc_write(codec, TAS5782M_REG_15, 0x00);
	snd_soc_write(codec, TAS5782M_REG_16, 0x00);
	snd_soc_write(codec, TAS5782M_REG_17, 0x01);
#endif
}

static int tas5782m_vol_put(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tas5782m_priv *tas5782m = snd_soc_codec_get_drvdata(codec);
		
	int db;
	
#if defined(CONFIG_TARGET_PRODUCT_IF_S200N) || defined(CONFIG_TARGET_PRODUCT_IF_S700N) || defined(CONFIG_TARGET_PRODUCT_IF_MKT200)
	if(ucontrol->value.integer.value[0] > (TAS5782M_VOLUME_MAX + TAS5782M_VOLUME_OFFSET)) 
		db = (TAS5782M_VOLUME_MAX + TAS5782M_VOLUME_OFFSET);
	else if(ucontrol->value.integer.value[0] < TAS5782M_VOLUME_MIN + TAS5782M_VOLUME_OFFSET) 
		db = (TAS5782M_VOLUME_MIN + TAS5782M_VOLUME_OFFSET);
	else 
		db = ucontrol->value.integer.value[0];
#else
	db = ucontrol->value.integer.value[0];
#endif

	tas5782m->volume_db = db;
	tas5782m_set_volume(codec, db);
	pr_info("[%s] Vol: %d\n", __func__, db);
	return 0;
}

static const struct snd_kcontrol_new tas5782m_vol_control = 
{	
    .iface = SNDRV_CTL_ELEM_IFACE_MIXER, 
    .name  = "TAS5782M Software Volume", 
	.info  = tas5782m_vol_info, 
	.get   = tas5782m_vol_get,
	.put   = tas5782m_vol_put, 
};

#if defined(CONFIG_SND_SOC_TAS5782)
static int tas5782m_sw_mute(struct snd_soc_codec *codec, int mute)
{
	u8 reg3_value = 0;
		
	pr_info("[%s] mute: %d\n", __func__, mute);
	
	if (mute) reg3_value = 0x11;

	snd_soc_write(codec, TAS5782M_REG_00, 0x00);
	snd_soc_write(codec, TAS5782M_REG_7F, 0x00);
	snd_soc_write(codec, TAS5782M_REG_03, reg3_value);
	
	if (mute) {
		snd_soc_write(codec, 0x02, 0x01);
	} else {
		snd_soc_write(codec, 0x02, 0x00);
	}
	
	return 0;
}

static int tas5782m_mute_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tas5782m_priv *tas5782m = snd_soc_codec_get_drvdata(codec);

	pr_info("[%s] %d\n", __func__, tas5782m->sw_mute);

	return tas5782m->sw_mute;;
}


static int tas5782m_mute_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tas5782m_priv *tas5782m = snd_soc_codec_get_drvdata(codec);
	
	tas5782m->sw_mute = ucontrol->value.integer.value[0];
	pr_info("[%s] %d\n", __func__, tas5782m->sw_mute);

	tas5782m_sw_mute(codec, tas5782m->sw_mute);

	return 0;
}


static const char * const dev_mute_text[] = {
	"unmute",
	"mute"
};

static const struct soc_enum dev_mute_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(dev_mute_text), dev_mute_text),
};


static const struct snd_kcontrol_new tas5782_snd_controls[] = {
	SOC_ENUM_EXT("TAS5782M Mute", dev_mute_enum[0],
		tas5782m_mute_get, tas5782m_mute_set),
};
#endif /*CONFIG_SND_SOC_TAS5782*/

static int tas5782m_snd_probe(struct snd_soc_codec *codec)
{
    int ret;
	
	pr_info("[%s]\n", __func__);
#if defined(CONFIG_TARGET_PRODUCT_IF_S200N) || defined(CONFIG_TARGET_PRODUCT_IF_S700N) || defined(CONFIG_TARGET_PRODUCT_IF_MKT200)
	tas5782m_init_reg();
#endif
	ret = snd_soc_add_codec_controls(codec, &tas5782m_vol_control, 1);
		
    return ret;
}

static struct snd_soc_codec_driver soc_codec_tas5782m = {
	.probe = tas5782m_snd_probe,
#if defined(CONFIG_TARGET_PRODUCT_IF_S200N) || defined(CONFIG_TARGET_PRODUCT_IF_S700N) || defined(CONFIG_TARGET_PRODUCT_IF_MKT200)
	.controls = tas5782_snd_controls,
	.num_controls = ARRAY_SIZE(tas5782_snd_controls),
#endif
};

static void transmit_registers(struct tas5782m_priv *tas5782m, cfg_reg *r, int n)
{
	int ret = 0;
	int i = 0;

	while(i < n) {
		switch(r[i].command) {
		case CFG_META_SWITCH:
			break;
		case CFG_META_DELAY:
			break;
		case CFG_META_BURST:
			tas5782m->client->addr = tas5782m->devA_addr;
			memset(tmp_buf,0x00,BUF_MAX);
			memcpy(tmp_buf,&(r[i+1].param),r[i].param-1);
			ret = regmap_bulk_write(tas5782m->regmap, r[i+1].command, tmp_buf, r[i].param-1);
			i +=  (r[i].param + 1)/2;
			break;
		default:
			tas5782m->client->addr = tas5782m->devA_addr;
			ret = regmap_write(tas5782m->regmap, r[i].command, r[i].param);
			if(ret < 0)
				pr_err("[%s] I2C Write Error\n", __func__);
			break;
		}
		i++;
	}
}


#if defined(CONFIG_TARGET_PRODUCT_IF_S200N) || defined(CONFIG_TARGET_PRODUCT_IF_S700N) || defined(CONFIG_TARGET_PRODUCT_IF_MKT200)
static bool first_init = false;

void tas5782m_init_reg(void)
{
	pr_info("[%s]\n", __func__);

	if(!first_init){
#if defined(CONFIG_TARGET_PRODUCT_IF_S700N) || defined(CONFIG_TARGET_PRODUCT_IF_MKT200)
		transmit_registers(g_tas5782m, registers_5782_pre_init, (sizeof(registers_5782_pre_init) / 2));
		msleep(20);
		transmit_registers(g_tas5782m, registers_5782_soft_reset, (sizeof(registers_5782_soft_reset) / 2));
		msleep(20);
#endif		
		transmit_registers(g_tas5782m, registers_init, (sizeof(registers_init) / 2));
		transmit_registers(g_tas5782m, registers_5782_mute, (sizeof(registers_5782_mute) / 2));
		first_init = true;
		pr_info("[%s] Init End\n", __func__);
	}
	else
		pr_info("[%s]No more set\n", __func__);

}
//EXPORT_SYMBOL(tas5782m_init_reg);

/*
static irqreturn_t tas5782m_spk_fault_irq_handler(int irq, void *dev_id) 
{
	struct tas5782m_priv *tas5782m = NULL;

	tas5782m = (struct tas5782m_priv*)dev_id;
	pr_info("[%s] value: %d\n", __func__, gpio_get_value(tas5782m->spk_fault_gpio));

	enable_irq(tas5782m->spk_fault_irq);	

	return IRQ_HANDLED;
}
*/
#endif

static int tas5782m_mute(struct snd_soc_dai *dai, int mute)
{
	u8 reg3_value = 0;
	struct snd_soc_codec *codec = dai->codec;
	
	pr_debug("[%s] Mute: %d\n", __func__, mute);

	if (mute)
		reg3_value = 0x11;
		
	snd_soc_write(codec, TAS5782M_REG_00, 0x00);
	snd_soc_write(codec, TAS5782M_REG_7F, 0x00);
	snd_soc_write(codec, TAS5782M_REG_03, reg3_value);
	
	return 0;
}

static const struct snd_soc_dai_ops tas5782m_dai_ops = {
	.digital_mute = tas5782m_mute,
};

static struct snd_soc_dai_driver tas5782m_dai = {
	.name		= "tas5782m-amplifier",
	.playback 	= {
		.stream_name	= "Playback",
		.channels_min	= 2,
		.channels_max	= 2,
		.rates		= TAS5872M_RATES,
		.formats	= TAS5782M_FORMATS,
	},
	.ops = &tas5782m_dai_ops,
};

static int tas5782m_parse_dt(struct device *dev, struct tas5782m_priv *tas5782m)
{
	struct device_node *np = dev->of_node;
	int ret;
	unsigned int value;
		
	pr_info("[%s]\n", __func__);
	
	tas5782m->ldo_enable_gpio = of_get_named_gpio(np, "infr,ldo-enable-gpio", 0);
	if (!gpio_is_valid(tas5782m->ldo_enable_gpio)) {
		pr_err("[%s] Looking up %s property is node %s falied %d\n", 
			__func__, "infr,ldo-enable-gpio", np->full_name, tas5782m->ldo_enable_gpio);
		tas5782m->ldo_enable_gpio = 0;
	} else {
		pr_info("[%s] tas5782m ldo-enable-gpio %d\n", __func__, tas5782m->ldo_enable_gpio);
		
		ret = gpio_request(tas5782m->ldo_enable_gpio, "TAS5782M-LDO-EN");
		if(ret < 0) {
			pr_err("[%s] GPIO %d request error\n", __func__, tas5782m->ldo_enable_gpio);
			tas5782m->ldo_enable_gpio = 0;
		} else {
			gpio_direction_output(tas5782m->ldo_enable_gpio, 0);
		}
	}
	
	tas5782m->reset_gpio = of_get_named_gpio(np, "ti,tas5782-reset-gpio", 0);
	if (!gpio_is_valid(tas5782m->reset_gpio)) {
		pr_err("[%s] Looking up %s property is node %s falied %d\n", 
			__func__, "ti,tas5782-reset-gpio", np->full_name, tas5782m->reset_gpio);
		tas5782m->reset_gpio = 0;
	} else {
		pr_info("[%s] tas5782m tas5782-reset-gpio %d\n", __func__, tas5782m->reset_gpio);
		
		ret = gpio_request(tas5782m->reset_gpio, "TAS5782M-RESET-EN");
		if(ret < 0) {
			pr_err("[%s] GPIO %d request error\n", __func__, tas5782m->reset_gpio);
			tas5782m->reset_gpio = 0;
		} else {
			gpio_direction_output(tas5782m->reset_gpio, 0);
		}
	}

#if defined(CONFIG_TARGET_PRODUCT_IF_S700N) || defined(CONFIG_TARGET_PRODUCT_IF_MKT200)
	tas5782m->amp_vdd_gpio = of_get_named_gpio(np, "infr,amp-vdd-gpio", 0);
	if (!gpio_is_valid(tas5782m->amp_vdd_gpio)) {
		pr_err("[%s] Looking up %s property is node %s falied %d\n", 
			__func__, "infr,amp-vdd-gpio", np->full_name, tas5782m->amp_vdd_gpio);
		tas5782m->amp_vdd_gpio = 0;
	} else {
		pr_info("[%s] tas5782m amp-vdd-gpio %d\n", __func__, tas5782m->amp_vdd_gpio);
		
		ret = gpio_request(tas5782m->amp_vdd_gpio, "TAS5782-AMP-VDD-EN");
		if(ret < 0) {
			pr_err("[%s] GPIO %d request error\n", __func__, tas5782m->amp_vdd_gpio);
			tas5782m->amp_vdd_gpio = 0;
		} else {
			gpio_direction_output(tas5782m->amp_vdd_gpio, 0);
		}
	}

	tas5782m->mute_gpio = of_get_named_gpio(np, "infr,amp-mute", 0);
	if (!gpio_is_valid(tas5782m->mute_gpio)) {
		pr_err("[%s] Looking up %s property is node %s falied %d\n", 
			__func__, "infr,amp-mute", np->full_name, tas5782m->mute_gpio);
		tas5782m->mute_gpio = 0;
	} else {
		pr_info("[%s] tas5782m amp-mute %d\n", __func__, tas5782m->mute_gpio);
		
		ret = gpio_request(tas5782m->mute_gpio, "TAS5782-AMP-MUTE");
		if(ret < 0) {
			pr_err("[%s] GPIO %d request error\n", __func__, tas5782m->mute_gpio);
			tas5782m->mute_gpio = 0;
		} else {
			gpio_direction_output(tas5782m->mute_gpio, 0);
		}
	}

	/*
	tas5782m->spk_fault_gpio = of_get_named_gpio(np, "ti,tas2559-irq-gpio", 0);
	if (!gpio_is_valid(tas5782m->spk_fault_gpio)) {
		pr_err("[%s] Looking up %s property is node %s falied %d\n",
            __func__, "ti,tas2559-irq-gpio", np->full_name, tas5782m->spk_fault_gpio);
        tas5782m->spk_fault_gpio = 0;
	} else {
		pr_info("[%s] tas5782m spk_fault_gpio %d\n", __func__, tas5782m->spk_fault_gpio);

        ret = gpio_request(tas5782m->spk_fault_gpio, "TAS5782-SPK_FALUT");
        if(ret < 0) {
            pr_err("[%s] GPIO %d request error\n", __func__, tas5782m->spk_fault_gpio);
            tas5782m->spk_fault_gpio = 0;
        } else {
            gpio_direction_input(tas5782m->spk_fault_gpio);
			tas5782m->spk_fault_irq = gpio_to_irq(tas5782m->spk_fault_gpio);
        	pr_info("[%s] irq = %d\n", __func__, tas5782m->spk_fault_irq);
			ret = request_threaded_irq(tas5782m->spk_fault_irq, tas5782m_spk_fault_irq_handler, NULL, 
								IRQF_TRIGGER_FALLING, "tas5782m spk fault", tas5782m);
			if(ret < 0) 
				pr_err("[%s] request irq fail... %d\n", __func__, ret);
			else
				disable_irq_nosync(tas5782m->spk_fault_irq);
		}
	}
	*/
#endif

	ret = of_property_read_u32(np, "ti,tas5782-addr", &value);
	if(ret) {
		pr_err("[%s] Looking up %s property in node %s failed\n", __func__, "ti,tas5782-addr", np->full_name);
		ret = -EINVAL;
	} else {
		tas5782m->devA_addr = value;
		pr_info("[%s] ti,tas5782 addr=0x%x\n", __func__, tas5782m->devA_addr);
	}	
	
	return 0;
}


/*static*//* deleted by dusin.jang, CONFIG_SND_SOC_TAS5782*/
int tas5782m_i2c_write_device(struct tas5782m_priv *tas5782m, 
										unsigned char addr, 
										unsigned char reg, 
										unsigned char value)
{
	int ret = 0;
	//dev_info(tas5782m->dev, "[%s] 0x%x(addr), 0x%x(reg), 0x%x(value)\n", __func__, addr, reg, value);

	tas5782m->client->addr = addr;
	ret = regmap_write(tas5782m->regmap, reg, value);

	if(addr == 0)
		return 0;

	if(ret < 0)
		pr_err("[%s] Addr: 0x%x Error\n", __func__, addr);

	return ret;
}

/*static*//* deleted by dusin.jang, CONFIG_SND_SOC_TAS5782*/
int tas5782m_i2c_read_device(struct tas5782m_priv *tas5782m, 
										unsigned char addr, 
										unsigned char reg, 
										unsigned char *pData)
{
	int ret = 0;
	unsigned int val = 0;

	tas5782m->client->addr = addr;
	ret = regmap_read(tas5782m->regmap, reg, &val);	

	if(ret < 0) {
		pr_err("[%s] Addr: 0x%x Error\n", __func__, addr);
	} else {
		*pData = (unsigned char)val;
	}

	return ret;	
}

static int tas5782m_i2c_update_bits(struct tas5782m_priv *tas5782m,
										unsigned char addr,
										unsigned char reg,
										unsigned char mask,
										unsigned char value)
{
	int ret = 0;

	tas5782m->client->addr = addr;
	ret = regmap_update_bits(tas5782m->regmap, reg, mask, value);

	if(reg < 0)
		pr_err("[%s] [0x%x] Error %d\n", __func__, addr, ret);

	return ret;
}

static int tas5782m_i2c_bulkread_device(struct tas5782m_priv *tas5782m,
											unsigned char addr,
											unsigned char reg,
											unsigned char *pValue,
											unsigned int len)
{
	int ret = 0;

	tas5782m->client->addr = addr;
	ret = regmap_bulk_read(tas5782m->regmap, reg, pValue, len);
	
	if(ret < 0)
		pr_err("[%s] [0x%x] Error %d\n", __func__, addr, ret);

	return ret;
}

static int tas5782m_i2c_bulkwrite_device(struct tas5782m_priv *tas5782m,
											unsigned char addr,
											unsigned char reg,
											unsigned char *pBuf,
											unsigned int len)
{
	int ret = 0;

	tas5782m->client->addr = addr;
	ret = regmap_bulk_write(tas5782m->regmap, reg, pBuf, len);

	if(ret < 0)
		pr_err("[%s] [0x%x] Error %d\n", __func__, addr, ret);

	return ret;
}

static int tas5782m_change_book_page(struct tas5782m_priv *tas5782m, enum channel ch, unsigned char book, unsigned char page)
{
	int ret = 0;

	if(ch & DevA) {
		if(tas5782m->devA_current_book == book) {
			if(tas5782m->devA_current_page != page) {
				ret = tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_BOOKCTL_PAGE, page);
				if(ret >= 0)
					tas5782m->devA_current_page = page; 
			}
		} else {
			ret = tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_BOOKCTL_PAGE, 0);
			if(ret >= 0) {
				tas5782m->devA_current_page = 0;
				ret = tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_BOOKCTL_REG, book);
				tas5782m->devA_current_book = book;
				
				if(page != 0) {
					ret = tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_BOOKCTL_PAGE, page);
					tas5782m->devA_current_page = page;
				}
			}
		}	
	} else if(ch & DevB) {
		
	}

	return ret;
}

static int tas5782m_dev_read(struct tas5782m_priv *tas5782m, enum channel ch, unsigned int reg, unsigned int *pValue) 
{
	int ret = 0;
	unsigned char value = 0;

	ret = tas5782m_change_book_page(tas5782m, ch, TAS5782M_BOOK_ID(reg), TAS5782M_PAGE_ID(reg));

	if(ret >= 0) {
		if(ch == DevA)
			ret = tas5782m_i2c_read_device(tas5782m, tas5782m->devA_addr, TAS5782M_PAGE_ID(reg), &value);

		else if(ch == DevB)
			ret = tas5782m_i2c_read_device(tas5782m, tas5782m->devB_addr, TAS5782M_PAGE_ID(reg), &value);

		if(ret >=0)
			*pValue = value;
	}

	return ret;
}

static int tas5782m_dev_write(struct tas5782m_priv *tas5782m, enum channel ch, unsigned int reg, unsigned int value)
{
	int ret = 0;

	ret = tas5782m_change_book_page(tas5782m, ch, TAS5782M_BOOK_ID(reg), TAS5782M_PAGE_ID(reg));
	if(ret >= 0) {
		if(ch & DevA)
			ret = tas5782m_i2c_write_device(tas5782m, tas5782m->devA_addr, TAS5782M_PAGE_ID(reg), value);
	
		if(ch & DevB)
			ret = tas5782m_i2c_write_device(tas5782m, tas5782m->devB_addr, TAS5782M_PAGE_ID(reg), value);
	}

    return ret;
}

static int tas5782m_dev_bulk_read(struct tas5782m_priv *tas5782m, enum channel ch, unsigned int reg, unsigned char *pData, unsigned int len)
{
	int ret = 0;
	unsigned char page_reg = 0;

	ret = tas5782m_change_book_page(tas5782m, ch, TAS5782M_BOOK_ID(reg), TAS5782M_PAGE_ID(reg));

	if(ret >= 0) {
		page_reg = TAS5782M_PAGE_REG(reg);	

		if(ch == DevA)
			ret = tas5782m_i2c_bulkread_device(tas5782m, tas5782m->devA_addr, page_reg, pData, len);

		else if(ch == DevB)
			ret = tas5782m_i2c_bulkread_device(tas5782m, tas5782m->devB_addr, page_reg, pData, len);


		if(ret < 0) {
			pr_err("[%s] ch Error: %d\n", __func__, ch);
			ret = -EINVAL;
		}
	}

    return ret;
}

static int tas5782m_dev_bulk_write(struct tas5782m_priv *tas5782m, enum channel ch, unsigned int reg, unsigned char *pData, unsigned int len)
{
	int ret = 0;
	unsigned char page_reg = 0;

	ret = tas5782m_change_book_page(tas5782m, ch, TAS5782M_BOOK_ID(reg), TAS5782M_PAGE_ID(reg));

	if(ret >= 0) {
		page_reg = TAS5782M_PAGE_REG(reg);

        if(ch & DevA)
            ret = tas5782m_i2c_bulkwrite_device(tas5782m, tas5782m->devA_addr, page_reg, pData, len);

        if(ch & DevB)
            ret = tas5782m_i2c_bulkwrite_device(tas5782m, tas5782m->devB_addr, page_reg, pData, len);


        if(ret < 0) {
            pr_err("[%s] ch Error: %d\n", __func__, ch);
            ret = -EINVAL;
        }
    }

    return ret;
}

static int tas5782m_dev_update_bits(struct tas5782m_priv *tas5782m,
										enum channel ch, 
										unsigned int reg, 
										unsigned int mask, 
										unsigned int value)
{
	int ret = 0;

	ret = tas5782m_change_book_page(tas5782m, ch, TAS5782M_BOOK_ID(reg), TAS5782M_PAGE_ID(reg));

	if(ret >= 0) {
		if(ch & DevA)
			ret = tas5782m_i2c_update_bits(tas5782m, tas5782m->devA_addr, TAS5782M_PAGE_REG(reg), mask, value);
	}	

	return ret;	
}


#if defined(CONFIG_TARGET_PRODUCT_IF_S200N) || defined(CONFIG_TARGET_PRODUCT_IF_S700N) || defined(CONFIG_TARGET_PRODUCT_IF_MKT200)
void tas5782_powerdown_request(bool enable)
{
	if(enable)
	{
		pr_debug("[%s] power down\n", __func__);
		transmit_registers(g_tas5782m,registers_5782_power_down,(sizeof(registers_5782_power_down) / 2)); 
	}
	else
	{
		pr_debug("[%s] nomal mode\n", __func__);
		transmit_registers(g_tas5782m,registers_5782_power_up,(sizeof(registers_5782_power_up) / 2)); 
	}
}
EXPORT_SYMBOL(tas5782_powerdown_request);
#endif



#if defined(CONFIG_TARGET_PRODUCT_IF_S700N) || defined(CONFIG_TARGET_PRODUCT_IF_MKT200)

/*
#define EQ_TABLE_NORMAL		0
#define EQ_TABLE_VOIP		1
*/
#ifdef USE_SWITCHABLE_EQ_TABLE
void tas5782_choose_eq_table(int mode)
{
	
}
EXPORT_SYMBOL(tas5782_choose_eq_table);
#endif /* USE_SWITCHABLE_EQ_TABLE */
#endif 


static int tas5782m_probe(struct i2c_client *i2c, struct regmap *regmap)
{
	struct device *dev = &i2c->dev;
	struct tas5782m_priv *tas5782m;
	int ret;
	
#if defined(CONFIG_TARGET_PRODUCT_IF_S200N)
	struct clk *ext_clk;
#endif
	
	pr_info("[%s]\n", __func__);
	
	tas5782m = devm_kzalloc(dev, sizeof(struct tas5782m_priv), GFP_KERNEL);
	if (!tas5782m)
		return -ENOMEM;

	dev_set_drvdata(dev, tas5782m);
	tas5782m->regmap = regmap;
    tas5782m->volume_db = TAS5782M_VOLUME_DEFAULT;
	
	if(dev->of_node) {
		tas5782m_parse_dt(dev, tas5782m);
	}


#if defined(CONFIG_TARGET_PRODUCT_IF_S200N)
	tas5782m_ldo_enable(tas5782m, true);
	tas5782m_reset(tas5782m, true);

	pr_err("WON : %s ext_clk\n", __func__);
	ext_clk = clk_get(dev, "wcd_clk");
	if (IS_ERR(ext_clk)) 
	{
		dev_err(dev, "%s: clk get %s failed\n",__func__, "ext_clk");
		goto err;
	}
	tas5782m->ext_clk = ext_clk;
#endif


	tas5782m->client = i2c; 
	tas5782m->read = tas5782m_dev_read;
	tas5782m->write = tas5782m_dev_write;
	tas5782m->bulk_read = tas5782m_dev_bulk_read;
	tas5782m->bulk_write = tas5782m_dev_bulk_write;
	tas5782m->update_bits = tas5782m_dev_update_bits;

	g_tas5782m = tas5782m;
	
#if defined(CONFIG_TARGET_PRODUCT_IF_S700N) || defined(CONFIG_TARGET_PRODUCT_IF_MKT200)
	tas5782m_ldo_enable(tas5782m, true);
	msleep(10);
	tas5782m_reset(tas5782m, true);
	tas5782m_amp_mute(tas5782m, true);
	tas5782m_amp_vdd_enable(tas5782m, true);
#endif
	
	ret = snd_soc_register_codec(dev, 
	                             &soc_codec_tas5782m,
			                     &tas5782m_dai, 
								 1);
	if (ret != 0) 
	{
		dev_err(dev, "Failed to register CODEC: %d\n", ret);
		goto err;
	}

	return 0;
	
err:
	return ret;

}



static int tas5782m_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{	
	struct regmap *regmap;
	struct regmap_config config = tas5782m_regmap;
	
	pr_info("[%s]\n", __func__);
	
	regmap = devm_regmap_init_i2c(i2c, &config);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);
		
	return tas5782m_probe(i2c, regmap);
}

static int tas5782m_remove(struct device *dev)
{	
	pr_info("[%s]\n", __func__);
	
	snd_soc_unregister_codec(dev);

#if defined(CONFIG_TARGET_PRODUCT_IF_S700N) || defined(CONFIG_TARGET_PRODUCT_IF_MKT200)
	first_init = false;
#endif

	return 0;
}

static int tas5782m_i2c_remove(struct i2c_client *i2c)
{	
	pr_info("[%s]\n", __func__);
	
	tas5782m_remove(&i2c->dev);
	
	return 0;
}

static const struct i2c_device_id tas5782m_i2c_id[] = {
	{ "tas5782m", },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tas5782m_i2c_id);

#ifdef CONFIG_OF
static const struct of_device_id tas5782m_of_match[] = {
	{ .compatible = "ti,tas5782m", },
	{ }
};
MODULE_DEVICE_TABLE(of, tas5782m_of_match);
#endif

static struct i2c_driver tas5782m_i2c_driver = {
	.probe 		= tas5782m_i2c_probe,
	.remove 	= tas5782m_i2c_remove,
	.id_table	= tas5782m_i2c_id,
	.driver		= {
		.name	= TAS5872M_DRV_NAME,
		.of_match_table = tas5782m_of_match,
	},
};

module_i2c_driver(tas5782m_i2c_driver);

MODULE_AUTHOR("Andy Liu <andy-liu@ti.com>");
MODULE_DESCRIPTION("TAS5782M Audio Amplifier Driver");
MODULE_LICENSE("GPL");
