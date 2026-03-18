#ifndef EXTRA_MMC_H_
#define EXTRA_MMC_H_

#define MMC_BW_CODE                     0xf0000061
#define MMC_JBL_CODE                    0x110005f1
#define MMC_KY_CODE                     0x00

#define MMC_BW_ORIGINAL_BAD_COUNT       40
#define MMC_BW_SLC_MIN_ERASE_COUNT      16
#define MMC_BW_SLC_MAX_ERASE_COUNT      20
#define MMC_BW_SLC_TOTAL_ERASE_COUNT    0   
#define MMC_BW_SLC_AVG_ERASE_COUNT      24
#define MMC_BW_TLC_MIN_ERASE_COUNT      28
#define MMC_BW_TLC_MAX_ERASE_COUNT      32
#define MMC_BW_TLC_TOTAL_ERASE_COUNT    0   
#define MMC_BW_TLC_AVG_ERASE_COUNT      36
#define MMC_BW_HOST_TOTAL_WRITE         8
#define MMC_BW_HOST_TOTAL_READ          0

#define MMC_JBL_ORIGINAL_BAD_COUNT      25
#define MMC_JBL_MIN_ERASE_COUNT         32
#define MMC_JBL_MAX_ERASE_COUNT         36
#define MMC_JBL_TOTAL_ERASE_COUNT       40
#define MMC_JBL_AVG_ERASE_COUNT         44
#define MMC_JBL_HOST_TOTAL_WRITE        112
#define MMC_JBL_HOST_TOTAL_READ         120

enum eMMC_Life
{
    Class_0=0, 
    Class_1, 
    Class_2, 
    Class_3, 
    Class_4, 
    Class_5, 
    Class_6, 
    Class_7, 
    Class_8, 
    Class_9, 
    Class_10,
    Class_Max
};

int get_emmc_extcsd(char **argv);               //CMD8 eMMC extcsd information
int do_status_switch(char **argv);              //CMD13 Status query &CMD7 status switching
int do_general_cmd_read(char **argv,int type);  //CMD56 Extended information query
int get_emmc_extinfo(char **argv,int type);     //eMMC information organization

#endif /*EXTRA_MMC_H_*/

