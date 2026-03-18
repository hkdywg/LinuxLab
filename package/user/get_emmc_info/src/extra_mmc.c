#include "main.h"
#include "mmc.h"
#include "extra_mmc.h"

static int read_extcsd(int fd, __u8 *ext_csd)
{
    int ret = 0;
    struct mmc_ioc_cmd idata;
    memset(&idata, 0, sizeof(idata));
    memset(ext_csd, 0, sizeof(__u8) * 512);
    idata.write_flag = 0;
    idata.opcode = MMC_SEND_EXT_CSD;
    idata.arg = 0;
    idata.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
    idata.blksz = 512;
    idata.blocks = 1;
    mmc_ioc_cmd_set_data(idata, ext_csd);

    ret = ioctl(fd, MMC_IOC_CMD, &idata);
    if (ret)
        perror("ioctl");

    return ret;
}

static unsigned int get_sector_count(__u8 *ext_csd)
{
    return (ext_csd[EXT_CSD_SEC_COUNT_3] << 24) |
    (ext_csd[EXT_CSD_SEC_COUNT_2] << 16) |
    (ext_csd[EXT_CSD_SEC_COUNT_1] << 8)  |
    ext_csd[EXT_CSD_SEC_COUNT_0];
}

static int is_blockaddresed(__u8 *ext_csd)
{
    unsigned int sectors = get_sector_count(ext_csd);

    /* over 2GiB devices are block-addressed */
    return (sectors > (2u * 1024 * 1024 * 1024) / 512);
}


static unsigned int get_hc_wp_grp_size(__u8 *ext_csd)
{
    return ext_csd[221];
}

static unsigned int get_hc_erase_grp_size(__u8 *ext_csd)
{
    return ext_csd[224];
}

static void print_writeprotect_boot_status(__u8 *ext_csd)
{
    __u8 reg;
    __u8 ext_csd_rev = ext_csd[EXT_CSD_REV];

    /* A43: reserved [174:0] */
    if (ext_csd_rev >= 5) {
        printf("Boot write protection status registers"
            " [BOOT_WP_STATUS]: 0x%02x\n", ext_csd[174]);

        reg = ext_csd[EXT_CSD_BOOT_WP];
        printf("Boot Area Write protection [BOOT_WP]: 0x%02x\n", reg);
        printf(" Power ro locking: ");
        if (reg & EXT_CSD_BOOT_WP_B_PWR_WP_DIS)
            printf("not possible\n");
        else
            printf("possible\n");

        printf(" Permanent ro locking: ");
        if (reg & EXT_CSD_BOOT_WP_B_PERM_WP_DIS)
            printf("not possible\n");
        else
            printf("possible\n");

        reg = ext_csd[EXT_CSD_BOOT_WP_STATUS];
        printf(" partition 0 ro lock status: ");
        if (reg & EXT_CSD_BOOT_WP_S_AREA_0_PERM)
            printf("locked permanently\n");
        else if (reg & EXT_CSD_BOOT_WP_S_AREA_0_PWR)
            printf("locked until next power on\n");
        else
            printf("not locked\n");
        printf(" partition 1 ro lock status: ");
        if (reg & EXT_CSD_BOOT_WP_S_AREA_1_PERM)
            printf("locked permanently\n");
        else if (reg & EXT_CSD_BOOT_WP_S_AREA_1_PWR)
            printf("locked until next power on\n");
        else
            printf("not locked\n");
    }
}

int get_emmc_extcsd(char **argv){
    
    __u8 ext_csd[512], ext_csd_rev, reg;
    __u32 regl;
    int fd, ret;
    char *device;
    const char *str;
    
    device = argv[1];
    
    fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(1);
    }
    
    ret = read_extcsd(fd, ext_csd);
    if (ret) {
        fprintf(stderr, "Could not read EXT_CSD from %s\n", device);
        exit(1);
    }
    
    ext_csd_rev = ext_csd[EXT_CSD_REV];

    switch (ext_csd_rev) {
    case 8:
        str = "5.1";
        break;
    case 7:
        str = "5.0";
        break;
    case 6:
        str = "4.5";
        break;
    case 5:
        str = "4.41";
        break;
    case 3:
        str = "4.3";
        break;
    case 2:
        str = "4.2";
        break;
    case 1:
        str = "4.1";
        break;
    case 0:
        str = "4.0";
        break;
    default:
        goto out_free;
    }
    printf("=============================================\n");
    printf("  Extended CSD rev 1.%d (MMC %s)\n", ext_csd_rev, str);
    printf("=============================================\n\n");

    if (ext_csd_rev < 3)
        goto out_free; /* No ext_csd */

    /* Parse the Extended CSD registers.
     * Reserved bit should be read as "0" in case of spec older
     * than A441.
     */
    reg = ext_csd[EXT_CSD_S_CMD_SET];
    printf("Card Supported Command sets [S_CMD_SET: 0x%02x]\n", reg);
    if (!reg)
        printf(" - Standard MMC command sets\n");

    reg = ext_csd[EXT_CSD_HPI_FEATURE];
    printf("HPI Features [HPI_FEATURE: 0x%02x]: ", reg);
    if (reg & EXT_CSD_HPI_SUPP) {
        if (reg & EXT_CSD_HPI_IMPL)
            printf("implementation based on CMD12\n");
        else
            printf("implementation based on CMD13\n");
    }

    printf("Background operations support [BKOPS_SUPPORT: 0x%02x]\n",
        ext_csd[502]);

    if (ext_csd_rev >= 6) {
        printf("Max Packet Read Cmd [MAX_PACKED_READS: 0x%02x]\n",
            ext_csd[501]);
        printf("Max Packet Write Cmd [MAX_PACKED_WRITES: 0x%02x]\n",
            ext_csd[500]);
        printf("Data TAG support [DATA_TAG_SUPPORT: 0x%02x]\n",
            ext_csd[499]);

        printf("Data TAG Unit Size [TAG_UNIT_SIZE: 0x%02x]\n",
            ext_csd[498]);
        printf("Tag Resources Size [TAG_RES_SIZE: 0x%02x]\n",
            ext_csd[497]);
        printf("Context Management Capabilities"
            " [CONTEXT_CAPABILITIES: 0x%02x]\n", ext_csd[496]);
        printf("Large Unit Size [LARGE_UNIT_SIZE_M1: 0x%02x]\n",
            ext_csd[495]);
        printf("Extended partition attribute support"
            " [EXT_SUPPORT: 0x%02x]\n", ext_csd[494]);
        printf("Generic CMD6 Timer [GENERIC_CMD6_TIME: 0x%02x]\n",
            ext_csd[248]);
        printf("Power off notification [POWER_OFF_LONG_TIME: 0x%02x]\n",
            ext_csd[247]);
        printf("Cache Size [CACHE_SIZE] is %d KiB\n",
            (ext_csd[249] << 0 | (ext_csd[250] << 8) |
            (ext_csd[251] << 16) | (ext_csd[252] << 24)) / 8);
    }

    /* A441: Reserved [501:247]
        A43: reserved [246:229] */
    if (ext_csd_rev >= 5) {
        printf("Background operations status"
            " [BKOPS_STATUS: 0x%02x]\n", ext_csd[246]);

        /* CORRECTLY_PRG_SECTORS_NUM [245:242] TODO */

        printf("1st Initialisation Time after programmed sector"
            " [INI_TIMEOUT_AP: 0x%02x]\n", ext_csd[241]);

        /* A441: reserved [240] */
        printf("Power class for 52MHz, DDR at 3.6V"
            " [PWR_CL_DDR_52_360: 0x%02x]\n", ext_csd[239]);
        printf("Power class for 52MHz, DDR at 1.95V"
            " [PWR_CL_DDR_52_195: 0x%02x]\n", ext_csd[238]);

        /* A441: reserved [237-236] */

        if (ext_csd_rev >= 6) {
            printf("Power class for 200MHz at 3.6V"
                " [PWR_CL_200_360: 0x%02x]\n", ext_csd[237]);
            printf("Power class for 200MHz, at 1.95V"
                " [PWR_CL_200_195: 0x%02x]\n", ext_csd[236]);
        }
        printf("Minimum Performance for 8bit at 52MHz in DDR mode:\n");
        printf(" [MIN_PERF_DDR_W_8_52: 0x%02x]\n", ext_csd[235]);
        printf(" [MIN_PERF_DDR_R_8_52: 0x%02x]\n", ext_csd[234]);
        /* A441: reserved [233] */
        printf("TRIM Multiplier [TRIM_MULT: 0x%02x]\n", ext_csd[232]);
        printf("Secure Feature support [SEC_FEATURE_SUPPORT: 0x%02x]\n",
            ext_csd[231]);
    }
    if (ext_csd_rev == 5) { /* Obsolete in 4.5 */
        printf("Secure Erase Multiplier [SEC_ERASE_MULT: 0x%02x]\n",
            ext_csd[230]);
        printf("Secure TRIM Multiplier [SEC_TRIM_MULT: 0x%02x]\n",
            ext_csd[229]);
    }
    reg = ext_csd[EXT_CSD_BOOT_INFO];
    printf("Boot Information [BOOT_INFO: 0x%02x]\n", reg);
    if (reg & EXT_CSD_BOOT_INFO_ALT)
        printf(" Device supports alternative boot method\n");
    if (reg & EXT_CSD_BOOT_INFO_DDR_DDR)
        printf(" Device supports dual data rate during boot\n");
    if (reg & EXT_CSD_BOOT_INFO_HS_MODE)
        printf(" Device supports high speed timing during boot\n");

    /* A441/A43: reserved [227] */
    printf("Boot partition size [BOOT_SIZE_MULTI: 0x%02x]\n", ext_csd[226]);
    printf("Access size [ACC_SIZE: 0x%02x]\n", ext_csd[225]);

    reg = get_hc_erase_grp_size(ext_csd);
    printf("High-capacity erase unit size [HC_ERASE_GRP_SIZE: 0x%02x]\n",
        reg);
    printf(" i.e. %u KiB\n", 512 * reg);

    printf("High-capacity erase timeout [ERASE_TIMEOUT_MULT: 0x%02x]\n",
        ext_csd[223]);
    printf("Reliable write sector count [REL_WR_SEC_C: 0x%02x]\n",
        ext_csd[222]);

    reg = get_hc_wp_grp_size(ext_csd);
    printf("High-capacity W protect group size [HC_WP_GRP_SIZE: 0x%02x]\n",
        reg);
    printf(" i.e. %lu KiB\n", 512l * get_hc_erase_grp_size(ext_csd) * reg);

    printf("Sleep current (VCC) [S_C_VCC: 0x%02x]\n", ext_csd[220]);
    printf("Sleep current (VCCQ) [S_C_VCCQ: 0x%02x]\n", ext_csd[219]);
    /* A441/A43: reserved [218] */
    printf("Sleep/awake timeout [S_A_TIMEOUT: 0x%02x]\n", ext_csd[217]);
    /* A441/A43: reserved [216] */

    unsigned int sectors =  get_sector_count(ext_csd);
    printf("Sector Count [SEC_COUNT: 0x%08x]\n", sectors);
    if (is_blockaddresed(ext_csd))
        printf(" Device is block-addressed\n");
    else
        printf(" Device is NOT block-addressed\n");

    /* A441/A43: reserved [211] */
    printf("Minimum Write Performance for 8bit:\n");
    printf(" [MIN_PERF_W_8_52: 0x%02x]\n", ext_csd[210]);
    printf(" [MIN_PERF_R_8_52: 0x%02x]\n", ext_csd[209]);
    printf(" [MIN_PERF_W_8_26_4_52: 0x%02x]\n", ext_csd[208]);
    printf(" [MIN_PERF_R_8_26_4_52: 0x%02x]\n", ext_csd[207]);
    printf("Minimum Write Performance for 4bit:\n");
    printf(" [MIN_PERF_W_4_26: 0x%02x]\n", ext_csd[206]);
    printf(" [MIN_PERF_R_4_26: 0x%02x]\n", ext_csd[205]);
    /* A441/A43: reserved [204] */
    printf("Power classes registers:\n");
    printf(" [PWR_CL_26_360: 0x%02x]\n", ext_csd[203]);
    printf(" [PWR_CL_52_360: 0x%02x]\n", ext_csd[202]);
    printf(" [PWR_CL_26_195: 0x%02x]\n", ext_csd[201]);
    printf(" [PWR_CL_52_195: 0x%02x]\n", ext_csd[200]);

    /* A43: reserved [199:198] */
    if (ext_csd_rev >= 5) {
        printf("Partition switching timing "
            "[PARTITION_SWITCH_TIME: 0x%02x]\n", ext_csd[199]);
        printf("Out-of-interrupt busy timing"
            " [OUT_OF_INTERRUPT_TIME: 0x%02x]\n", ext_csd[198]);
    }

    /* A441/A43: reserved   [197] [195] [193] [190] [188]
     * [186] [184] [182] [180] [176] */

    if (ext_csd_rev >= 6)
        printf("I/O Driver Strength [DRIVER_STRENGTH: 0x%02x]\n",
            ext_csd[197]);

    /* DEVICE_TYPE in A45, CARD_TYPE in A441 */
    reg = ext_csd[196];
    printf("Card Type [CARD_TYPE: 0x%02x]\n", reg);
    if (reg & 0x80) printf(" HS400 Dual Data Rate eMMC @200MHz 1.2VI/O\n");
    if (reg & 0x40) printf(" HS400 Dual Data Rate eMMC @200MHz 1.8VI/O\n");
    if (reg & 0x20) printf(" HS200 Single Data Rate eMMC @200MHz 1.2VI/O\n");
    if (reg & 0x10) printf(" HS200 Single Data Rate eMMC @200MHz 1.8VI/O\n");
    if (reg & 0x08) printf(" HS Dual Data Rate eMMC @52MHz 1.2VI/O\n");
    if (reg & 0x04) printf(" HS Dual Data Rate eMMC @52MHz 1.8V or 3VI/O\n");
    if (reg & 0x02) printf(" HS eMMC @52MHz - at rated device voltage(s)\n");
    if (reg & 0x01) printf(" HS eMMC @26MHz - at rated device voltage(s)\n");

    printf("CSD structure version [CSD_STRUCTURE: 0x%02x]\n", ext_csd[194]);
    /* ext_csd_rev = ext_csd[EXT_CSD_REV] (already done!!!) */
    printf("Command set [CMD_SET: 0x%02x]\n", ext_csd[191]);
    printf("Command set revision [CMD_SET_REV: 0x%02x]\n", ext_csd[189]);
    printf("Power class [POWER_CLASS: 0x%02x]\n", ext_csd[187]);
    printf("High-speed interface timing [HS_TIMING: 0x%02x]\n",
        ext_csd[185]);
    if (ext_csd_rev >= 8)
        printf("Enhanced Strobe mode [STROBE_SUPPORT: 0x%02x]\n",
            ext_csd[184]);
    /* bus_width: ext_csd[183] not readable */
    printf("Erased memory content [ERASED_MEM_CONT: 0x%02x]\n",
        ext_csd[181]);
    reg = ext_csd[EXT_CSD_BOOT_CFG];
    printf("Boot configuration bytes [PARTITION_CONFIG: 0x%02x]\n", reg);
    switch ((reg & EXT_CSD_BOOT_CFG_EN)>>3) {
    case 0x0:
        printf(" Not boot enable\n");
        break;
    case 0x1:
        printf(" Boot Partition 1 enabled\n");
        break;
    case 0x2:
        printf(" Boot Partition 2 enabled\n");
        break;
    case 0x7:
        printf(" User Area Enabled for boot\n");
        break;
    }
    switch (reg & EXT_CSD_BOOT_CFG_ACC) {
    case 0x0:
        printf(" No access to boot partition\n");
        break;
    case 0x1:
        printf(" R/W Boot Partition 1\n");
        break;
    case 0x2:
        printf(" R/W Boot Partition 2\n");
        break;
    case 0x3:
        printf(" R/W Replay Protected Memory Block (RPMB)\n");
        break;
    default:
        printf(" Access to General Purpose partition %d\n",
            (reg & EXT_CSD_BOOT_CFG_ACC) - 3);
        break;
    }

    printf("Boot config protection [BOOT_CONFIG_PROT: 0x%02x]\n",
        ext_csd[178]);
    printf("Boot bus Conditions [BOOT_BUS_CONDITIONS: 0x%02x]\n",
        ext_csd[177]);
    printf("High-density erase group definition"
        " [ERASE_GROUP_DEF: 0x%02x]\n", ext_csd[EXT_CSD_ERASE_GROUP_DEF]);

    print_writeprotect_boot_status(ext_csd);

    if (ext_csd_rev >= 5) {
        /* A441]: reserved [172] */
        printf("User area write protection register"
            " [USER_WP]: 0x%02x\n", ext_csd[171]);
        /* A441]: reserved [170] */
        printf("FW configuration [FW_CONFIG]: 0x%02x\n", ext_csd[169]);
        printf("RPMB Size [RPMB_SIZE_MULT]: 0x%02x\n", ext_csd[168]);

        reg = ext_csd[EXT_CSD_WR_REL_SET];
        const char * const fast = "existing data is at risk if a power "
                "failure occurs during a write operation";
        const char * const reliable = "the device protects existing "
                "data if a power failure occurs during a write "
                "operation";
        printf("Write reliability setting register"
            " [WR_REL_SET]: 0x%02x\n", reg);

        printf(" user area: %s\n", (reg & (1<<0)) ? reliable : fast);
        int i;
        for (i = 1; i <= 4; i++) {
            printf(" partition %d: %s\n", i,
                (reg & (1<<i)) ? reliable : fast);
        }

        reg = ext_csd[EXT_CSD_WR_REL_PARAM];
        printf("Write reliability parameter register"
            " [WR_REL_PARAM]: 0x%02x\n", reg);
        if (reg & 0x01)
            printf(" Device supports writing EXT_CSD_WR_REL_SET\n");
        if (reg & 0x04)
            printf(" Device supports the enhanced def. of reliable "
                "write\n");

        /* sanitize_start ext_csd[165]]: not readable
         * bkops_start ext_csd[164]]: only writable */
        printf("Enable background operations handshake"
            " [BKOPS_EN]: 0x%02x\n", ext_csd[163]);
        printf("H/W reset function"
            " [RST_N_FUNCTION]: 0x%02x\n", ext_csd[162]);
        printf("HPI management [HPI_MGMT]: 0x%02x\n", ext_csd[161]);
        reg = ext_csd[EXT_CSD_PARTITIONING_SUPPORT];
        printf("Partitioning Support [PARTITIONING_SUPPORT]: 0x%02x\n",
            reg);
        if (reg & EXT_CSD_PARTITIONING_EN)
            printf(" Device support partitioning feature\n");
        else
            printf(" Device NOT support partitioning feature\n");
        if (reg & EXT_CSD_ENH_ATTRIBUTE_EN)
            printf(" Device can have enhanced tech.\n");
        else
            printf(" Device cannot have enhanced tech.\n");

        regl = (ext_csd[EXT_CSD_MAX_ENH_SIZE_MULT_2] << 16) |
            (ext_csd[EXT_CSD_MAX_ENH_SIZE_MULT_1] << 8) |
            ext_csd[EXT_CSD_MAX_ENH_SIZE_MULT_0];

        printf("Max Enhanced Area Size [MAX_ENH_SIZE_MULT]: 0x%06x\n",
               regl);
        unsigned int wp_sz = get_hc_wp_grp_size(ext_csd);
        unsigned int erase_sz = get_hc_erase_grp_size(ext_csd);
        printf(" i.e. %lu KiB\n", 512l * regl * wp_sz * erase_sz);

        printf("Partitions attribute [PARTITIONS_ATTRIBUTE]: 0x%02x\n",
            ext_csd[EXT_CSD_PARTITIONS_ATTRIBUTE]);
        reg = ext_csd[EXT_CSD_PARTITION_SETTING_COMPLETED];
        printf("Partitioning Setting"
            " [PARTITION_SETTING_COMPLETED]: 0x%02x\n",
            reg);
        if (reg)
            printf(" Device partition setting complete\n");
        else
            printf(" Device partition setting NOT complete\n");

        printf("General Purpose Partition Size\n"
            " [GP_SIZE_MULT_4]: 0x%06x\n", (ext_csd[154] << 16) |
            (ext_csd[153] << 8) | ext_csd[152]);
        printf(" [GP_SIZE_MULT_3]: 0x%06x\n", (ext_csd[151] << 16) |
               (ext_csd[150] << 8) | ext_csd[149]);
        printf(" [GP_SIZE_MULT_2]: 0x%06x\n", (ext_csd[148] << 16) |
               (ext_csd[147] << 8) | ext_csd[146]);
        printf(" [GP_SIZE_MULT_1]: 0x%06x\n", (ext_csd[145] << 16) |
               (ext_csd[144] << 8) | ext_csd[143]);

        regl =  (ext_csd[EXT_CSD_ENH_SIZE_MULT_2] << 16) |
            (ext_csd[EXT_CSD_ENH_SIZE_MULT_1] << 8) |
            ext_csd[EXT_CSD_ENH_SIZE_MULT_0];
        printf("Enhanced User Data Area Size"
            " [ENH_SIZE_MULT]: 0x%06x\n", regl);
        printf(" i.e. %lu KiB\n", 512l * regl *
               get_hc_erase_grp_size(ext_csd) *
               get_hc_wp_grp_size(ext_csd));

        regl =  (ext_csd[EXT_CSD_ENH_START_ADDR_3] << 24) |
            (ext_csd[EXT_CSD_ENH_START_ADDR_2] << 16) |
            (ext_csd[EXT_CSD_ENH_START_ADDR_1] << 8) |
            ext_csd[EXT_CSD_ENH_START_ADDR_0];
        printf("Enhanced User Data Start Address"
            " [ENH_START_ADDR]: 0x%08x\n", regl);
        printf(" i.e. %llu bytes offset\n", (is_blockaddresed(ext_csd) ?
                512ll : 1ll) * regl);

        /* A441]: reserved [135] */
        printf("Bad Block Management mode"
            " [SEC_BAD_BLK_MGMNT]: 0x%02x\n", ext_csd[134]);
        /* A441: reserved [133:0] */
    }
    
    /* B45 */
    if (ext_csd_rev >= 6) {
        int j;
        /* tcase_support ext_csd[132] not readable */
        printf("Periodic Wake-up [PERIODIC_WAKEUP]: 0x%02x\n",
            ext_csd[131]);
        printf("Program CID/CSD in DDR mode support"
            " [PROGRAM_CID_CSD_DDR_SUPPORT]: 0x%02x\n",
               ext_csd[130]);

        for (j = 127; j >= 64; j--)
            printf("Vendor Specific Fields"
                " [VENDOR_SPECIFIC_FIELD[%d]]: 0x%02x\n",
                j, ext_csd[j]);

        printf("Native sector size [NATIVE_SECTOR_SIZE]: 0x%02x\n",
            ext_csd[63]);
        printf("Sector size emulation [USE_NATIVE_SECTOR]: 0x%02x\n",
            ext_csd[62]);
        printf("Sector size [DATA_SECTOR_SIZE]: 0x%02x\n", ext_csd[61]);
        printf("1st initialization after disabling sector"
            " size emulation [INI_TIMEOUT_EMU]: 0x%02x\n",
            ext_csd[60]);
        printf("Class 6 commands control [CLASS_6_CTRL]: 0x%02x\n",
            ext_csd[59]);
        printf("Number of addressed group to be Released"
            "[DYNCAP_NEEDED]: 0x%02x\n", ext_csd[58]);
        printf("Exception events control"
            " [EXCEPTION_EVENTS_CTRL]: 0x%04x\n",
            (ext_csd[57] << 8) | ext_csd[56]);
        printf("Exception events status"
            "[EXCEPTION_EVENTS_STATUS]: 0x%04x\n",
            (ext_csd[55] << 8) | ext_csd[54]);
        printf("Extended Partitions Attribute"
            " [EXT_PARTITIONS_ATTRIBUTE]: 0x%04x\n",
            (ext_csd[53] << 8) | ext_csd[52]);

        for (j = 51; j >= 37; j--)
            printf("Context configuration"
                " [CONTEXT_CONF[%d]]: 0x%02x\n", j, ext_csd[j]);

        printf("Packed command status"
            " [PACKED_COMMAND_STATUS]: 0x%02x\n", ext_csd[36]);
        printf("Packed command failure index"
            " [PACKED_FAILURE_INDEX]: 0x%02x\n", ext_csd[35]);
        printf("Power Off Notification"
            " [POWER_OFF_NOTIFICATION]: 0x%02x\n", ext_csd[34]);
        printf("Control to turn the Cache ON/OFF"
            " [CACHE_CTRL]: 0x%02x\n", ext_csd[33]);
        /* flush_cache ext_csd[32] not readable */
        printf("Control to turn the Cache Barrier ON/OFF"
            " [BARRIER_CTRL]: 0x%02x\n", ext_csd[31]);
        /*Reserved [30:0] */
    }

    if (ext_csd_rev >= 7) {
        printf("eMMC Firmware Version: %.8s\n", (char*)&ext_csd[EXT_CSD_FIRMWARE_VERSION]);
        printf("eMMC Life Time Estimation A [EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A]: 0x%02x\n",
            ext_csd[EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A]);
        printf("eMMC Life Time Estimation B [EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B]: 0x%02x\n",
            ext_csd[EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B]);
        printf("eMMC Pre EOL information [EXT_CSD_PRE_EOL_INFO]: 0x%02x\n",
            ext_csd[EXT_CSD_PRE_EOL_INFO]);
        reg = ext_csd[EXT_CSD_SECURE_REMOVAL_TYPE];
        printf("Secure Removal Type [SECURE_REMOVAL_TYPE]: 0x%02x\n", reg);
        printf(" information is configured to be removed ");
        /* Bit [5:4]: Configure Secure Removal Type */
        switch ((reg & EXT_CSD_CONFIG_SECRM_TYPE) >> 4) {
            case 0x0:
                printf("by an erase of the physical memory\n");
                break;
            case 0x1:
                printf("by an overwriting the addressed locations"
                       " with a character followed by an erase\n");
                break;
            case 0x2:
                printf("by an overwriting the addressed locations"
                       " with a character, its complement, then a random character\n");
                break;
            case 0x3:
                printf("using a vendor defined\n");
                break;
        }
        /* Bit [3:0]: Supported Secure Removal Type */
        printf(" Supported Secure Removal Type:\n");
        if (reg & 0x01)
            printf("  information removed by an erase of the physical memory\n");
        if (reg & 0x02)
            printf("  information removed by an overwriting the addressed locations"
                   " with a character followed by an erase\n");
        if (reg & 0x04)
            printf("  information removed by an overwriting the addressed locations"
                   " with a character, its complement, then a random character\n");
        if (reg & 0x08)
            printf("  information removed using a vendor defined\n");
    }

    if (ext_csd_rev >= 8) {
        printf("Command Queue Support [CMDQ_SUPPORT]: 0x%02x\n",
               ext_csd[EXT_CSD_CMDQ_SUPPORT]);
        printf("Command Queue Depth [CMDQ_DEPTH]: %u\n",
               (ext_csd[EXT_CSD_CMDQ_DEPTH] & 0x1f) + 1);
        printf("Command Enabled [CMDQ_MODE_EN]: 0x%02x\n",
               ext_csd[EXT_CSD_CMDQ_MODE_EN]);
        printf("Note: CMDQ_MODE_EN may not indicate the runtime CMDQ ON or OFF.\n"
               "Please check sysfs node '/sys/devices/.../mmc_host/mmcX/mmcX:XXXX/cmdq_en'\n");
    }

out_free:
    return ret;
}

/*********************************************************************
 *function name :   do_status_switch
 *Descriptions  :   CMD13 status query &CMD7 status switching
 ********************************************************************/
int do_status_switch(char **argv)
{
    int fd = open("/dev/mmcblk0", O_RDWR);
    if (fd < 0) {
        perror("open failed");
        return -1;
    }

    // 1. Check the current status (CMD13)
    struct mmc_ioc_cmd status_cmd = {
        .opcode = MMC_SEND_STATUS,
        .arg = 0x00010000,  // Suppose RCA=0x0001
        .flags = MMC_RSP_R1,
    };
    ioctl(fd, MMC_IOC_CMD, &status_cmd);
    printf("Current State: %d\n", (status_cmd.response[0] >> 9) & 0x0F);

    // 2. Send the CMD7 selection card (switch to STANBY state)
    struct mmc_ioc_cmd select_cmd = {
        .opcode = MMC_SELECT_CARD,
        .arg = 0,
        .flags = MMC_RSP_NONE | MMC_CMD_AC,
    };
    if (ioctl(fd, MMC_IOC_CMD, &select_cmd) < 0) {
        perror("CMD7 failed");
        close(fd);
        return -1;
    }
    
     // 3. Check the status again
    ioctl(fd, MMC_IOC_CMD, &status_cmd);
    printf("Current State: %d\n", (status_cmd.response[0] >> 9) & 0x0F);
    
    // 4. Send the CMD7 selection card (switch to TRAN state)
    struct mmc_ioc_cmd sw_cmd = {
        .opcode = MMC_SELECT_CARD,
        .arg = 0x00010000,  // RCA=0x0001
        .flags = MMC_RSP_R1 | MMC_CMD_AC,
    };
    if (ioctl(fd, MMC_IOC_CMD, &sw_cmd) < 0) {
        perror("CMD7 failed");
        close(fd);
        return -1;
    }

    // 5. Check the status again
    ioctl(fd, MMC_IOC_CMD, &status_cmd);
    printf("Current State: %d\n", (status_cmd.response[0] >> 9) & 0x0F);

    close(fd);
    return 0;
}

/*********************************************************************
 *function name :   do_general_cmd_read
 *Descriptions  :   Read the extended information
 ********************************************************************/
int do_general_cmd_read(char **argv,int type)
{
    int dev_fd;
    char *device;
    __u8 buf[512];
    __u16 bad_count_jbl;
    __u32 arg = 0x01;
    __u32 min_erase_count,max_erase_count,total_erase_count,avg_erase_count;
    __u32 bad_count_bw;
    int ret = -EINVAL,i;
    int host_total_w,host_total_r;
    struct mmc_ioc_cmd idata;

    device = argv[1];
    dev_fd = open(device, O_RDWR);
    if (dev_fd < 0) {
        perror("device open failed");
        exit(1);
    }
    
    switch(type){
        case type_bw :
            printf("Device Type : bw eMMC\n");
            arg = MMC_BW_CODE;
            break;  
        case type_jbl :
            printf("Device Type : jbl eMMC\n"); 
            arg = MMC_JBL_CODE;
            break;  
        case type_ky :
            printf("Device Type : ky eMMC\n");
            arg = MMC_KY_CODE;
            return 1;
            break;  
        default:
            exit(1);
        break;
    }
    
    /* arg is specified */
    if (errno != 0 || !(arg & 0x1)) {
        fprintf(stderr, "Wrong ARG, it should be Hex number and bit0 must be 1\n");
        goto out;
    }
    
    memset(&idata, 0, sizeof(idata));
    idata.write_flag = 0;
    idata.opcode = MMC_GEN_CMD;//CMD56
    idata.arg = arg;
    idata.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
    idata.blksz = 512;
    idata.blocks = 1;
    mmc_ioc_cmd_set_data(idata, buf);

    ret = ioctl(dev_fd, MMC_IOC_CMD, &idata);
    if (ret) {
        perror("ioctl");
        goto out;
    }

#if 0
    Debug_pr("Data:\n");
    for (i = 0; i < 512; i++) {
        Debug_pr("%2x ", buf[i]);
        if ((i + 1) % 16 == 0)
            Debug_pr("\n");
    }
#endif

    if(type==type_bw){
    
        min_erase_count = 0;
        max_erase_count = 0;
        avg_erase_count = 0;
        
        for (i = 0; i < 4; i++) {
            min_erase_count = min_erase_count | buf[MMC_BW_SLC_MIN_ERASE_COUNT+i]<<(i*8);           //Minimum number of erase and write cycles
            max_erase_count = max_erase_count | buf[MMC_BW_SLC_MAX_ERASE_COUNT+i]<<(i*8);           //Maximum number of erase and write cycles
            avg_erase_count = avg_erase_count | buf[MMC_BW_SLC_AVG_ERASE_COUNT+i]<<(i*8);           //Average number of erase and write cycles
        }
        printf("SLC Min_Erase_Count   : %d\n",min_erase_count);
        printf("SLC Max_Erase_Count   : %d\n",max_erase_count);
        printf("SLC Avg_Erase_Count   : %d\n",avg_erase_count);
    
        //////////////////////////////////////////////////////////////////////////////////
        
        min_erase_count = 0;
        max_erase_count = 0;
        avg_erase_count = 0;
        
        for (i = 0; i < 4; i++) {
            min_erase_count = min_erase_count | buf[MMC_BW_TLC_MIN_ERASE_COUNT+i]<<(i*8);           //Minimum number of erase and write cycles
            max_erase_count = max_erase_count | buf[MMC_BW_TLC_MAX_ERASE_COUNT+i]<<(i*8);           //Maximum number of erase and write cycles
            avg_erase_count = avg_erase_count | buf[MMC_BW_TLC_AVG_ERASE_COUNT+i]<<(i*8);           //Average number of erase and write cycles
        }
        printf("MLC_TLC Min_Erase_Count   : %d\n",min_erase_count);
        printf("MLC_TLC Max_Erase_Count   : %d\n",max_erase_count);
        printf("MLC_TLC Avg_Erase_Count   : %d\n",avg_erase_count);
    
        bad_count_bw = 0;
        for (i = 0; i < 4; i++) {
            bad_count_bw = bad_count_bw | buf[MMC_BW_ORIGINAL_BAD_COUNT+i]<<(i*8);      //Number of bad blocks
        }
        
        printf("eMMC Run Bad Pieces : %d\n",bad_count_bw);
        
        host_total_w = 0;
        for (i = 0; i < 7; i++) {
            host_total_w = host_total_w | buf[MMC_BW_HOST_TOTAL_WRITE+i]<<(i*8);        //Total number of writes 
        }
        host_total_w = (double)host_total_w / (2 * 1024);
        
        printf("Host Total_Write: %d MB\n",host_total_w);
        
        printf(">>NEXT\n");
    }else if(type==type_jbl){
        min_erase_count = 0;
        max_erase_count = 0;
        total_erase_count = 0;
        avg_erase_count = 0;
    
        for (i = 0; i < 4; i++) {
            min_erase_count = min_erase_count | buf[MMC_JBL_MIN_ERASE_COUNT+i]<<(i*8);          //Minimum number of erase and write cycles
            max_erase_count = max_erase_count | buf[MMC_JBL_MAX_ERASE_COUNT+i]<<(i*8);          //Maximum number of erase and write cycles
            total_erase_count = total_erase_count | buf[MMC_JBL_TOTAL_ERASE_COUNT+i]<<(i*8);    //Total number of erase and write cycles
            avg_erase_count = avg_erase_count | buf[MMC_JBL_AVG_ERASE_COUNT+i]<<(i*8);          //Average number of erase and write cycles
        }
        printf("MLC_TLC Min_Erase_Count   : %d\n",min_erase_count);
        printf("MLC_TLC Max_Erase_Count   : %d\n",max_erase_count);
        printf("MLC_TLC Total_Erase_Count : %d\n",total_erase_count);
        printf("MLC_TLC Avg_Erase_Count   : %d\n",avg_erase_count);
    
        bad_count_jbl = 0;
        for (i = 0; i < 2; i++) {
            bad_count_jbl = bad_count_jbl | buf[MMC_JBL_ORIGINAL_BAD_COUNT+i]<<(i*8);           //Number of bad blocks
        }
        
        printf("eMMC Run Bad Pieces : %d\n",bad_count_jbl);
        
        host_total_w = 0;
        host_total_r = 0;
        for (i = 0; i < 8; i++) {
            host_total_w = host_total_w | buf[MMC_JBL_HOST_TOTAL_WRITE+i]<<(i*8);       //Total number of writes
            host_total_r = host_total_r | buf[MMC_JBL_HOST_TOTAL_READ+i]<<(i*8);        //Total number of reads
        }
        
        printf("Host Total_Write: %d ,(128M unit)\n",host_total_w);
        printf("Host Total_Read: %d ,(128M unit)\n",host_total_r);
        
        printf(">>NEXT\n");
    }else if(type==type_ky){
        printf(">>NEXT\n");
    }
out:
    close(dev_fd);
    return ret;
}

/*********************************************************************
 *function name :   get_emmc_extinfo
 ********************************************************************/
int get_emmc_extinfo(char **argv,int type){

    __u8 ext_csd[512],life_est_A,life_est_B;
    int fd, ret;
    char *device;
    
    device = argv[1];
    fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(1);
    }
    
    ret = read_extcsd(fd, ext_csd);
    if (ret) {
        fprintf(stderr, "Could not read EXT_CSD from %s\n", device);
        exit(1);
    }
    
#if 0 //debug
    int i;
    for (i = 270; i < 301; i++) {
        printf("%2x ", ext_csd[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
#endif
    
    life_est_A = ext_csd[EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A];               //Obtain lifespan A
    life_est_B = ext_csd[EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B];               //Obtain lifespan B
    
    printf("eMMC Life Time Estimation A : 0x%02x\n",life_est_A);    
    
    #if 1
    switch(life_est_A){
        case Class_0 :printf("Not defined\n"); break;
        case Class_1 :printf("00%% - 10%% device life time used\n");break;          //00%-10%Utilization rate
        case Class_2 :printf("10%% - 20%% device life time used\n");break;          //10%-20%Utilization rate
        case Class_3 :printf("20%% - 30%% device life time used\n");break;          //20%-30%Utilization rate
        case Class_4 :printf("30%% - 40%% device life time used\n");break;          //30%-40%Utilization rate
        case Class_5 :printf("40%% - 50%% device life time used\n");break;          //40%-50%Utilization rate
        case Class_6 :printf("50%% - 60%% device life time used\n");break;          //50%-60%Utilization rate
        case Class_7 :printf("60%% - 70%% device life time used\n");break;          //60%-70%Utilization rate
        case Class_8 :printf("70%% - 80%% device life time used\n");break;          //70%-80%Utilization rate
        case Class_9 :printf("80%% - 90%% device life time used\n");break;          //80%-90%Utilization rate
        case Class_10 :printf("90%% - 100%% device life time used\n");break;       //90%-100%Utilization rate, the lifespan is approaching. Pay attention
        case Class_Max :printf("Exceeded its maximum estimated device life time\n");break;
        default:break;
    }
    #endif
    
    printf("eMMC Life Time Estimation B : 0x%02x\n",life_est_B);
    
    #if 1
    switch(life_est_B){
        case Class_0 :printf("Not defined\n");break;
        case Class_1 :printf("00%% - 10%% device life time used\n");break;          //00%-10%Utilization rate
        case Class_2 :printf("10%% - 20%% device life time used\n");break;          //10%-20%Utilization rate
        case Class_3 :printf("20%% - 30%% device life time used\n");break;          //20%-30%Utilization rate
        case Class_4 :printf("30%% - 40%% device life time used\n");break;          //30%-40%Utilization rate
        case Class_5 :printf("40%% - 50%% device life time used\n");break;          //40%-50%Utilization rate
        case Class_6 :printf("50%% - 60%% device life time used\n");break;          //50%-60%Utilization rate
        case Class_7 :printf("60%% - 70%% device life time used\n");break;          //60%-70%Utilization rate
        case Class_8 :printf("70%% - 80%% device life time used\n");break;          //70%-80%Utilization rate
        case Class_9 :printf("80%% - 90%% device life time used\n");break;          //80%-90%Utilization rate
        case Class_10 :printf("90%% - 100%% device life time used\n");break;       //90%-100%Utilization rate, the lifespan is approaching. Pay attention
        case Class_Max :printf("Exceeded its maximum estimated device life time\n");break;
        default:break;
    }
    #endif
    
    printf(">>END\n");
    
    return ret;
}

