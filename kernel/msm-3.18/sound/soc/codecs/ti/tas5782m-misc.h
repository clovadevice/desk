
#ifndef _TAS5782M_MISC_H_
#define _TAS5782M_MISC_H_

#define TIAUDIO_CMD_REG_WITE            1
#define TIAUDIO_CMD_REG_READ            2
#define TIAUDIO_CMD_DEBUG_ON            3
#define TIAUDIO_CMD_PROGRAM         	4
#define TIAUDIO_CMD_CONFIGURATION       5
#define TIAUDIO_CMD_FW_TIMESTAMP        6
#define TIAUDIO_CMD_CALIBRATION         7
#define TIAUDIO_CMD_SAMPLERATE          8
#define TIAUDIO_CMD_BITRATE         	9
#define TIAUDIO_CMD_DACVOLUME           10
#define TIAUDIO_CMD_SPEAKER         	11
#define TIAUDIO_CMD_FW_RELOAD           12
#define TIAUDIO_CMD_MUTE            	13


#if defined(CONFIG_SND_SOC_TAS5782)/*by dusin.jang at 20180510*/
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


const uint32_t tas5782m_misc_volume[] = {
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
#endif/*CONFIG_SND_SOC_TAS5782*/

#endif
