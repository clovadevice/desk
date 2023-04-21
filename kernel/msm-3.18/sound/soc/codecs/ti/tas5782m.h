
#ifndef _TAS5782M_H_
#define _TAS5782M_H_

#include <linux/regmap.h>

/* Page Control Register */
#define TAS5782M_PAGECTL_REG         		0

/* Book Control Register (available in page0 of each book) */
#define TAS5782M_BOOKCTL_PAGE           	0
#define TAS5782M_BOOKCTL_REG         		127

#define TAS5782M_REG(book, page, reg)		((((unsigned int)book * 256 * 128) + ((unsigned int)page * 128)) + reg)

#define TAS5782M_BOOK_ID(reg)            	((unsigned char)(reg / (256 * 128)))
#define TAS5782M_PAGE_ID(reg)            	((unsigned char)((reg % (256 * 128)) / 128))
#define TAS5782M_BOOK_REG(reg)           	((unsigned char)(reg % (256 * 128)))
#define TAS5782M_PAGE_REG(reg)           	((unsigned char)((reg % (256 * 128)) % 128))

enum channel {
    DevA = 0x01,
    DevB = 0x02,
    DevBoth = (DevA),
};

typedef unsigned char cfg_u8;

typedef union {
    struct {
        cfg_u8 offset;
        cfg_u8 value;
    };
    struct {
        cfg_u8 command;
        cfg_u8 param;
    };
} cfg_reg;


struct tas5782m_priv {
    struct regmap *regmap;
  	struct i2c_client *client;
	unsigned char devA_addr;
	unsigned char devB_addr;
 
    int ldo_enable_gpio;
    int reset_gpio;
	struct device *dev;	
#if defined(CONFIG_TARGET_PRODUCT_IF_S700N) || defined(CONFIG_TARGET_PRODUCT_IF_MKT200)
    int amp_vdd_gpio;
    int mute_gpio;
	int spk_fault_gpio;
	int spk_fault_irq;
#endif
#if defined(CONFIG_TARGET_PRODUCT_IF_S200N)	
	struct clk *ext_clk; // MCLK
#endif
#if defined(CONFIG_SND_SOC_TAS5782)
	int sw_mute;
#endif	
   
    int volume_db;

	enum channel current_ch;
	int current_reg;

	unsigned char devA_current_book, devB_current_book;
	unsigned char devA_current_page, devB_current_page;
	
	int cmd;
	int (*read)(struct tas5782m_priv *tas5782m,
			enum channel ch, unsigned int reg, unsigned int *value);
	int (*write)(struct tas5782m_priv *tas5782m,
			enum channel ch, unsigned int reg, unsigned int value);
	int (*bulk_read)(struct tas5782m_priv *tas5782m,
			enum channel ch, unsigned int reg, unsigned char *pData, unsigned int len);
	int (*bulk_write)(struct tas5782m_priv *tas5782m,
			enum channel ch, unsigned int reg, unsigned char *pData, unsigned int len);
	int (*update_bits)(struct tas5782m_priv *tas5782m,
			enum channel chn, unsigned int reg, unsigned int mask, unsigned int value);
};

#if defined(CONFIG_SND_SOC_TAS5782)/*by dusin.jang at 20180510*/
int tas5782m_i2c_write_device(struct tas5782m_priv *tas5782m, 
										unsigned char addr, 
										unsigned char reg, 
										unsigned char value);

int tas5782m_i2c_read_device(struct tas5782m_priv *tas5782m, 
										unsigned char addr, 
										unsigned char reg, 
										unsigned char *pData);
#endif/*CONFIG_SND_SOC_TAS5782*/
#endif/*_TAS5782M_H_*/
