/*
* ti_ds90uh981.c 
*	driver for ds90uh981
*
* @copyright Copyright (c) 2022 Jiangsu New Vision Automotive Electronics Co.，Ltd. All rights reserved.
*
* Author: weigenyin <weigenyin@zjautomotive.com>
*
*/
#include "../display_serdes_core.h"
#include "ti_ds90uh981.h"

struct pll_reg_config {
    u8 ch0_ndiv[2];
    u8 ch0_num[3];
    u8 ch0_den[3];
    u8 ch0_pdiv;
    u8 mash_order;
};

static const struct regmap_range ds90uh981_bridge_volatile_ranges[] = {
    { .range_min = 0, .range_max = 0xFF },
};

static const struct regmap_access_table ds90uh981_bridge_volatile_table = {
    .yes_ranges = ds90uh981_bridge_volatile_ranges,
    .n_yes_ranges = ARRAY_SIZE(ds90uh981_bridge_volatile_ranges),
};

static struct regmap_config ds90uh981_regmap_config = {
    .name = "ds90uh981",
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = 0x8000,
    .volatile_table = &ds90uh981_bridge_volatile_table,
    .cache_type = REGCACHE_RBTREE,
};

struct serdes_function_data {
    u8 gpio_out_dis:1;
    u8 gpio_io_rx_en:1;
    u8 gpio_tx_en_a:1;
    u8 gpio_tx_en_b:1;
    u8 gpio_rx_en_a:1;
    u8 gpio_rx_en_b:1;
    u8 gpio_tx_id;
    u8 gpio_rx_id;
};

struct config_desc {
    u16 reg;
    u8 mask;
    u8 val;
};

struct serdes_group_data {
    const struct config_desc *configs;
    int num_configs;
};

static int ds90uh981_mfp0_pins[] = {0}; 
static int ds90uh981_mfp1_pins[] = {1}; 
static int ds90uh981_mfp2_pins[] = {2}; 
static int ds90uh981_mfp3_pins[] = {3}; 
static int ds90uh981_mfp4_pins[] = {4}; 
static int ds90uh981_mfp5_pins[] = {5}; 
static int ds90uh981_mfp6_pins[] = {6}; 
static int ds90uh981_mfp7_pins[] = {7}; 

static int ds90uh981_mfp8_pins[] = {8}; 
static int ds90uh981_mfp9_pins[] = {9}; 
static int ds90uh981_mfp10_pins[] = {10}; 
static int ds90uh981_mfp11_pins[] = {11}; 
static int ds90uh981_mfp12_pins[] = {12}; 
static int ds90uh981_mfp13_pins[] = {13}; 
static int ds90uh981_mfp14_pins[] = {14}; 
static int ds90uh981_mfp15_pins[] = {15}; 

static int ds90uh981_mfp16_pins[] = {16}; 
static int ds90uh981_mfp17_pins[] = {17}; 
static int ds90uh981_mfp18_pins[] = {18}; 
static int ds90uh981_mfp19_pins[] = {19}; 
static int ds90uh981_mfp20_pins[] = {20}; 
static int ds90uh981_mfp21_pins[] = {21}; 
static int ds90uh981_mfp22_pins[] = {22}; 
static int ds90uh981_mfp23_pins[] = {23}; 

static int ds90uh981_mfp24_pins[] = {24}; 
static int ds90uh981_mfp25_pins[] = {25}; 
static int ds90uh981_i2c_pins[] = {3, 7}; 
static int ds90uh981_uart_pins[] = {3, 7}; 

#define GROUP_DESC(nm) \
{ \
    .name = #nm, \
    .pins = nm ## _pins, \
    .num_pins = ARRAY_SIZE(nm ## _pins), \
}

#define GROUP_DESC_CONFIG(nm) \
{ \
    .name = #nm, \
    .pins = nm ## _pins, \
    .num_pins = ARRAY_SIZE(nm ## _pins), \
    .data = (void *)(const struct serdes_group_data []) { \
        { .configs = nm ## _configs, \
          .num_configs = ARRAY_SIZE(nm ## _configs), } \
    }, \
}

static const struct config_desc ds90uh981_mfp13_configs[] = {
    { 0x0005, PU_LF0, 0 },
};

static const struct config_desc ds90uh981_mfp14_configs[] = {
    { 0x0005, PU_LF1, 0 },
};

static const struct config_desc ds90uh981_mfp15_configs[] = {
    { 0x0005, PU_LF2, 0 },
};

static const struct config_desc ds90uh981_mfp16_configs[] = {
    { 0x0005, PU_LF3, 0 },
};

static const char *serdes_gpio_groups[] = {
    "ds90uh981_mfp0", "ds90uh981_mfp1", "ds90uh981_mfp2", "ds90uh981_mfp3",
    "ds90uh981_mfp4", "ds90uh981_mfp5", "ds90uh981_mfp6", "ds90uh981_mfp7",

    "ds90uh981_mfp8", "ds90uh981_mfp9", "ds90uh981_mfp10", "ds90uh981_mfp11",
    "ds90uh981_mfp12", "ds90uh981_mfp13", "ds90uh981_mfp14", "ds90uh981_mfp15",

    "ds90uh981_mfp16", "ds90uh981_mfp17", "ds90uh981_mfp18", "ds90uh981_mfp19",
    "ds90uh981_mfp20", "ds90uh981_mfp21", "ds90uh981_mfp22", "ds90uh981_mfp23",

    "ds90uh981_mfp24", "ds90uh981_mfp25",
};

static const char *ds90uh981_i2c_groups[] = { "ds90uh981_i2c" };
static const char *ds90uh981_uart_groups[] = { "ds90uh981_uart" };

#define FUNCTION_DESC(nm) \
{ \
    .name = #nm, \
    .group_names = nm##_groups, \
    .num_group_names = ARRAY_SIZE(nm##_groups), \
}

#define FUNCTION_DESC_GPIO_OUTPUT_A(id) \
{ \
    .name = "SER_TXID"#id"_TO_DES_LINKA", \
    .group_names = serdes_gpio_groups, \
    .num_group_names = ARRAY_SIZE(serdes_gpio_groups), \
    .data = (void *)(const struct serdes_function_data []) { \
        { .gpio_out_dis = 1, .gpio_tx_en_a = 1, \
          .gpio_io_rx_en = 1, .gpio_rx_id = id, } \
    }, \
}

#define FUNCTION_DESC_GPIO_OUTPUT_B(id) \
{ \
    .name = "SER_TXID"#id"_TO_DES_LINKB", \
    .group_names = serdes_gpio_groups, \
    .num_group_names = ARRAY_SIZE(serdes_gpio_groups), \
    .data = (void *)(const struct serdes_function_data []) { \
        { .gpio_out_dis = 1, .gpio_tx_en_a = 1, \
          .gpio_io_rx_en = 1, .gpio_rx_id = id, } \
    }, \
}

#define FUNCTION_DESC_GPIO_INPUT_A(id) \
{ \
    .name = "SER_TXID"#id"_TO_DES_LINKA", \
    .group_names = serdes_gpio_groups, \
    .num_group_names = ARRAY_SIZE(serdes_gpio_groups), \
    .data = (void *)(const struct serdes_function_data []) { \
        { .gpio_io_rx_en = 1, .gpio_rx_id = id, } \
    }, \
}

#define FUNCTION_DESC_GPIO_INPUT_B(id) \
{ \
    .name = "SER_TXID"#id"_TO_DES_LINKA", \
    .group_names = serdes_gpio_groups, \
    .num_group_names = ARRAY_SIZE(serdes_gpio_groups), \
    .data = (void *)(const struct serdes_function_data []) { \
        { .gpio_io_rx_en = 1, .gpio_rx_id = id, } \
    }, \
}


#define FUNCTION_DESC_GPIO() \
{ \
    .name = "DS90uh981_GPIO", \
    .group_names = serdes_gpio_groups, \
    .num_group_names = ARRAY_SIZE(serdes_gpio_groups), \
    .data = (void *)(const struct serdes_function_data []) { \
        {  } \
    }, \
}

static struct pinctrl_pin_desc ds90uh981_pins_desc[] = {
    PINCTRL_PIN(DS90UH981_MFP0, "ds90uh981_mfp0"),
    PINCTRL_PIN(DS90UH981_MFP1, "ds90uh981_mfp1"),
    PINCTRL_PIN(DS90UH981_MFP2, "ds90uh981_mfp2"),
    PINCTRL_PIN(DS90UH981_MFP3, "ds90uh981_mfp3"),
    PINCTRL_PIN(DS90UH981_MFP4, "ds90uh981_mfp4"),
    PINCTRL_PIN(DS90UH981_MFP5, "ds90uh981_mfp5"),
    PINCTRL_PIN(DS90UH981_MFP6, "ds90uh981_mfp6"),
    PINCTRL_PIN(DS90UH981_MFP7, "ds90uh981_mfp7"),

    PINCTRL_PIN(DS90UH981_MFP8,  "ds90uh981_mfp8"),
    PINCTRL_PIN(DS90UH981_MFP9,  "ds90uh981_mfp9"),
    PINCTRL_PIN(DS90UH981_MFP10, "ds90uh981_mfp10"),
    PINCTRL_PIN(DS90UH981_MFP11, "ds90uh981_mfp11"),
    PINCTRL_PIN(DS90UH981_MFP12, "ds90uh981_mfp12"),
    PINCTRL_PIN(DS90UH981_MFP13, "ds90uh981_mfp13"),
    PINCTRL_PIN(DS90UH981_MFP14, "ds90uh981_mfp14"),
    PINCTRL_PIN(DS90UH981_MFP15, "ds90uh981_mfp15"),

    PINCTRL_PIN(DS90UH981_MFP16, "ds90uh981_mfp16"),
    PINCTRL_PIN(DS90UH981_MFP17, "ds90uh981_mfp17"),
    PINCTRL_PIN(DS90UH981_MFP18, "ds90uh981_mfp18"),
    PINCTRL_PIN(DS90UH981_MFP19, "ds90uh981_mfp19"),
    PINCTRL_PIN(DS90UH981_MFP20, "ds90uh981_mfp20"),
    PINCTRL_PIN(DS90UH981_MFP21, "ds90uh981_mfp21"),
    PINCTRL_PIN(DS90UH981_MFP22, "ds90uh981_mfp22"),
    PINCTRL_PIN(DS90UH981_MFP23, "ds90uh981_mfp23"),

    PINCTRL_PIN(DS90UH981_MFP24, "ds90uh981_mfp24"),
    PINCTRL_PIN(DS90UH981_MFP25, "ds90uh981_mfp25"),
};

static struct group_desc ds90uh981_groups_desc[] = {
    GROUP_DESC(ds90uh981_mfp0),
    GROUP_DESC(ds90uh981_mfp1),
    GROUP_DESC(ds90uh981_mfp2),
    GROUP_DESC(ds90uh981_mfp3),
    GROUP_DESC(ds90uh981_mfp4),
    GROUP_DESC(ds90uh981_mfp5),
    GROUP_DESC(ds90uh981_mfp6),
    GROUP_DESC(ds90uh981_mfp7),

    GROUP_DESC(ds90uh981_mfp8),
    GROUP_DESC(ds90uh981_mfp9),
    GROUP_DESC(ds90uh981_mfp10),
    GROUP_DESC(ds90uh981_mfp11),
    GROUP_DESC(ds90uh981_mfp12),
    GROUP_DESC(ds90uh981_mfp13),
    GROUP_DESC(ds90uh981_mfp14),
    GROUP_DESC(ds90uh981_mfp15),

    GROUP_DESC(ds90uh981_mfp16),
    GROUP_DESC(ds90uh981_mfp17),
    GROUP_DESC(ds90uh981_mfp18),
    GROUP_DESC(ds90uh981_mfp19),
    GROUP_DESC(ds90uh981_mfp20),
    GROUP_DESC(ds90uh981_mfp21),
    GROUP_DESC(ds90uh981_mfp22),
    GROUP_DESC(ds90uh981_mfp23),

    GROUP_DESC(ds90uh981_mfp24),
    GROUP_DESC(ds90uh981_mfp25),

    GROUP_DESC(ds90uh981_i2c),
    GROUP_DESC(ds90uh981_uart),
};

static struct function_desc ds90uh981_functions_desc[] = {
    FUNCTION_DESC_GPIO_INPUT_A(0),
    FUNCTION_DESC_GPIO_INPUT_A(1),
    FUNCTION_DESC_GPIO_INPUT_A(2),
    FUNCTION_DESC_GPIO_INPUT_A(3),
    FUNCTION_DESC_GPIO_INPUT_A(4),
    FUNCTION_DESC_GPIO_INPUT_A(6),
    FUNCTION_DESC_GPIO_INPUT_A(7),

    FUNCTION_DESC_GPIO_INPUT_A(8),
    FUNCTION_DESC_GPIO_INPUT_A(9),
    FUNCTION_DESC_GPIO_INPUT_A(10),
    FUNCTION_DESC_GPIO_INPUT_A(11),
    FUNCTION_DESC_GPIO_INPUT_A(12),
    FUNCTION_DESC_GPIO_INPUT_A(13),
    FUNCTION_DESC_GPIO_INPUT_A(14),
    FUNCTION_DESC_GPIO_INPUT_A(15),

    FUNCTION_DESC_GPIO_INPUT_A(16),
    FUNCTION_DESC_GPIO_INPUT_A(17),
    FUNCTION_DESC_GPIO_INPUT_A(18),
    FUNCTION_DESC_GPIO_INPUT_A(19),
    FUNCTION_DESC_GPIO_INPUT_A(20),
    FUNCTION_DESC_GPIO_INPUT_A(21),
    FUNCTION_DESC_GPIO_INPUT_A(22),
    FUNCTION_DESC_GPIO_INPUT_A(23),

    FUNCTION_DESC_GPIO_INPUT_A(24),
    FUNCTION_DESC_GPIO_INPUT_A(25),

    FUNCTION_DESC_GPIO_OUTPUT_A(0),
    FUNCTION_DESC_GPIO_OUTPUT_A(1),
    FUNCTION_DESC_GPIO_OUTPUT_A(2),
    FUNCTION_DESC_GPIO_OUTPUT_A(3),
    FUNCTION_DESC_GPIO_OUTPUT_A(4),
    FUNCTION_DESC_GPIO_OUTPUT_A(6),
    FUNCTION_DESC_GPIO_OUTPUT_A(7),

    FUNCTION_DESC_GPIO_OUTPUT_A(8),
    FUNCTION_DESC_GPIO_OUTPUT_A(9),
    FUNCTION_DESC_GPIO_OUTPUT_A(10),
    FUNCTION_DESC_GPIO_OUTPUT_A(11),
    FUNCTION_DESC_GPIO_OUTPUT_A(12),
    FUNCTION_DESC_GPIO_OUTPUT_A(13),
    FUNCTION_DESC_GPIO_OUTPUT_A(14),
    FUNCTION_DESC_GPIO_OUTPUT_A(15),

    FUNCTION_DESC_GPIO_OUTPUT_A(16),
    FUNCTION_DESC_GPIO_OUTPUT_A(17),
    FUNCTION_DESC_GPIO_OUTPUT_A(18),
    FUNCTION_DESC_GPIO_OUTPUT_A(19),
    FUNCTION_DESC_GPIO_OUTPUT_A(20),
    FUNCTION_DESC_GPIO_OUTPUT_A(21),
    FUNCTION_DESC_GPIO_OUTPUT_A(22),
    FUNCTION_DESC_GPIO_OUTPUT_A(23),

    FUNCTION_DESC_GPIO_OUTPUT_A(24),
    FUNCTION_DESC_GPIO_OUTPUT_A(25),

    FUNCTION_DESC_GPIO_INPUT_B(0),
    FUNCTION_DESC_GPIO_INPUT_B(1),
    FUNCTION_DESC_GPIO_INPUT_B(2),
    FUNCTION_DESC_GPIO_INPUT_B(3),
    FUNCTION_DESC_GPIO_INPUT_B(4),
    FUNCTION_DESC_GPIO_INPUT_B(6),
    FUNCTION_DESC_GPIO_INPUT_B(7),

    FUNCTION_DESC_GPIO_INPUT_B(8),
    FUNCTION_DESC_GPIO_INPUT_B(9),
    FUNCTION_DESC_GPIO_INPUT_B(10),
    FUNCTION_DESC_GPIO_INPUT_B(11),
    FUNCTION_DESC_GPIO_INPUT_B(12),
    FUNCTION_DESC_GPIO_INPUT_B(13),
    FUNCTION_DESC_GPIO_INPUT_B(14),
    FUNCTION_DESC_GPIO_INPUT_B(15),

    FUNCTION_DESC_GPIO_INPUT_B(16),
    FUNCTION_DESC_GPIO_INPUT_B(17),
    FUNCTION_DESC_GPIO_INPUT_B(18),
    FUNCTION_DESC_GPIO_INPUT_B(19),
    FUNCTION_DESC_GPIO_INPUT_B(20),
    FUNCTION_DESC_GPIO_INPUT_B(21),
    FUNCTION_DESC_GPIO_INPUT_B(22),
    FUNCTION_DESC_GPIO_INPUT_B(23),

    FUNCTION_DESC_GPIO_INPUT_B(24),
    FUNCTION_DESC_GPIO_INPUT_B(25),

    FUNCTION_DESC_GPIO_OUTPUT_B(0),
    FUNCTION_DESC_GPIO_OUTPUT_B(1),
    FUNCTION_DESC_GPIO_OUTPUT_B(2),
    FUNCTION_DESC_GPIO_OUTPUT_B(3),
    FUNCTION_DESC_GPIO_OUTPUT_B(4),
    FUNCTION_DESC_GPIO_OUTPUT_B(6),
    FUNCTION_DESC_GPIO_OUTPUT_B(7),

    FUNCTION_DESC_GPIO_OUTPUT_B(8),
    FUNCTION_DESC_GPIO_OUTPUT_B(9),
    FUNCTION_DESC_GPIO_OUTPUT_B(10),
    FUNCTION_DESC_GPIO_OUTPUT_B(11),
    FUNCTION_DESC_GPIO_OUTPUT_B(12),
    FUNCTION_DESC_GPIO_OUTPUT_B(13),
    FUNCTION_DESC_GPIO_OUTPUT_B(14),
    FUNCTION_DESC_GPIO_OUTPUT_B(15),

    FUNCTION_DESC_GPIO_OUTPUT_B(16),
    FUNCTION_DESC_GPIO_OUTPUT_B(17),
    FUNCTION_DESC_GPIO_OUTPUT_B(18),
    FUNCTION_DESC_GPIO_OUTPUT_B(19),
    FUNCTION_DESC_GPIO_OUTPUT_B(20),
    FUNCTION_DESC_GPIO_OUTPUT_B(21),
    FUNCTION_DESC_GPIO_OUTPUT_B(22),
    FUNCTION_DESC_GPIO_OUTPUT_B(23),

    FUNCTION_DESC_GPIO_OUTPUT_B(24),
    FUNCTION_DESC_GPIO_OUTPUT_B(25),

    FUNCTION_DESC_GPIO(),

    FUNCTION_DESC(ds90uh981_i2c),
    FUNCTION_DESC(ds90uh981_uart),
};

static struct serdes_chip_pinctrl_info ds90uh981_pinctrl_info = {
    .pins = ds90uh981_pins_desc,
    .num_pins = ARRAY_SIZE(ds90uh981_pins_desc),
    .groups = ds90uh981_groups_desc,
    .num_groups = ARRAY_SIZE(ds90uh981_groups_desc),
    .functions = ds90uh981_functions_desc,
    .num_functions = ARRAY_SIZE(ds90uh981_functions_desc),
};

static void rational_best_approximation(u64 given_numerator, u64 given_denominator,
				u64 max_numerator, u64 max_denominator,
				u64 *best_numerator, u64 *best_denominator)
{
    u64 n, d, n0, d0, n1, d1;

    n = given_numerator;
    d = given_denominator;
    n0 = 0;
    d0 = 1;
    n1 = 1;
    d1 = 0;

    for (;;) {
        u64 a, t;

        if (d == 0)
            break;

        a = n / d;

        t = n0 + a * n1;
        n0 = n1;
        n1 = t;

        t = d0 + a * d1;
        d0 = d1;
        d1 = t;

        if (d1 > max_denominator) {
            /* If we exceeded the max denominator, back off */
            n1 = n0;
            d1 = d0;
            break;
        }

        if ((n % d) == 0)
            break;

        t = n % d;
        n = d;
        d = t;
    }
    *best_numerator = n1;
    *best_denominator = d1;
}


/**
 * calculate_pll - Calculate PLL register settings
 * @target_pclk_hz: Target Pixel Clock in Hz
 * @ref_freq_hz: Reference Frequency in Hz
 * @port_num: Port number (0 or 1)
 * @fpd_mode: FPD Link Mode (0=FPD3, 1=FPD4)
 * @config: Pointer to output configuration structure
 *
 * Returns 0 on success, negative error code on failure.
 */
static int calculate_pll(u64 target_pclk_hz, u64 ref_freq_hz,
			 int port_num, int fpd_mode,
			 struct pll_reg_config *config)
{
	int p_divs[] = {1, 2, 4, 8, 16, 32};
	int p_codes[] = {0x0, 0x0, 0xE0, 0xF0, 0xF8, 0xFC}; // Mapped codes
	int num_p_divs = sizeof(p_divs) / sizeof(p_divs[0]);
	
	/* Best solution tracking */
	int best_p_idx = -1;
	u64 best_n = 0;
	u64 best_num = 0;
	u64 best_den = 1;
	u64 best_score = (u64)-1; /* Lower is better */
	bool solution_found = false;

	int i;
	
	u64 output_freq_hz;
	u64 pdf_hz;

	/* 
	 * FPD3: output_freq = PCLK * 35 / 2
	 * PDF = Ref * 2
	 */
	if (fpd_mode == 0) { // FPD3
		/* Use * 35 / 2. To avoid precision loss, * 35 then / 2 */
		output_freq_hz = (target_pclk_hz * 35) / 2;
	} else {
		// FPD4 placeholder (assumes target_pclk is rate)
		output_freq_hz = target_pclk_hz; 
	}

	pdf_hz = ref_freq_hz * 2;

	/* 
	 * Search strategy:
	 * Iterate P dividers.
	 * Calculate required VCO freq = OutputFreq * P.
	 * Check if VCO is within range.
	 * Calculate divider N and fraction Num/Den using PDF.
	 */

	for (i = 0; i < num_p_divs; i++) {
		int p = p_divs[i];
		u64 vco_freq, n, rem, num, den, score;

		vco_freq = output_freq_hz * p;

		/* Check VCO range */
		if (vco_freq < SPEC_MIN_VCO_FREQ_HZ || 
		    vco_freq > SPEC_MAX_VCO_FREQ_HZ)
			continue;

		/* Calculate Divider */
		/* ratio = VCO / PDF */
		n = vco_freq / pdf_hz;
		rem = vco_freq % pdf_hz;
		
		num = 0;
		den = 1;

		if (rem == 0) {
			/* Integer solution */
			num = 0;
			den = 1;
			score = 0; /* Best possible score */
		} else {
			/* Fractional solution */
			/* We have fraction rem/pdf_hz */
			rational_best_approximation(rem, pdf_hz,
						    SPEC_MAX_DENOMINATOR,
						    SPEC_MAX_DENOMINATOR,
						    &num, &den);
			
			/* Score: prefer smaller denominators */
			score = den + num;
		}

		/* 
		 * Update best solution.
		 */
		if (!solution_found || score < best_score) {
			best_score = score;
			best_p_idx = i;
			best_n = n;
			best_num = num;
			best_den = den;
			solution_found = true;
		}
	}

	if (!solution_found) {
		pr_err("No valid PLL solution found for Target=%llu Hz, Ref=%llu Hz\n",
		       target_pclk_hz, ref_freq_hz);
		return -1;
	}

	/* Scale fraction to maximize dynamic range if fractional */
	if (best_num != 0) {
		u64 scale = SPEC_MAX_DENOMINATOR / best_den;
		best_num *= scale;
		best_den *= scale;
	}

	pr_info("Selected Solution: VCO=%llu Hz, P=%d, N=%llu, Num=%llu, Den=%llu\n",
		output_freq_hz * p_divs[best_p_idx], 
		p_divs[best_p_idx], best_n, best_num, best_den);

	/* Map to registers */
	config->ch0_ndiv[0] = (u8)(best_n & 0xFF);
	config->ch0_ndiv[1] = (u8)((best_n >> 8) & 0x0F); 

	config->ch0_num[0] = (u8)(best_num & 0xFF);
	config->ch0_num[1] = (u8)((best_num >> 8) & 0xFF);
	config->ch0_num[2] = (u8)((best_num >> 16) & 0xFF);

	config->ch0_den[0] = (u8)(best_den & 0xFF);
	config->ch0_den[1] = (u8)((best_den >> 8) & 0xFF);
	config->ch0_den[2] = (u8)((best_den >> 16) & 0xFF);

	config->ch0_pdiv = (u8)p_codes[best_p_idx];

	if (best_num == 0)
		config->mash_order = 0x00; 
	else
		config->mash_order = 0x08; 

	return 0;
}

static int ds90uh981_bridge_init(struct serdes *serdes)
{
	int ret;
	int fpd_mode, pclk_freq, select_port;	
    u64 ref_freq = 27000000;
    struct device *dev = serdes->dev;
    struct pll_reg_config pll_cfg;

    ret = of_property_read_u32(dev->of_node, "fpd-mode", &fpd_mode);
    if(!ret)
        fpd_mode = 0;

    ret = of_property_read_u32(dev->of_node, "select-port", &select_port);
    if(!ret)
        fpd_mode = 0;

    ret = of_property_read_u32(dev->of_node, "pclk-freq", &pclk_freq);
    if(!ret) {
        dev_err(dev, "Not set pclk frequence, please check device tree file!\n");
        return -ENODEV;
    }

    ret = calculate_pll(pclk_freq, ref_freq, select_port, fpd_mode, &pll_cfg);
    if(!ret) {
        // N-divider config
		serdes_reg_write(serdes, 0x40, 0x0a);
		serdes_reg_write(serdes, 0x41, 0x05);
		serdes_reg_write(serdes, 0x42, pll_cfg.ch0_ndiv[0]);
		serdes_reg_write(serdes, 0x42, pll_cfg.ch0_ndiv[1]);
        // Denominator config
		serdes_reg_write(serdes, 0x40, 0x0a);
		serdes_reg_write(serdes, 0x41, 0x18);
		serdes_reg_write(serdes, 0x42, pll_cfg.ch0_den[0]);
		serdes_reg_write(serdes, 0x42, pll_cfg.ch0_den[1]);
		serdes_reg_write(serdes, 0x42, pll_cfg.ch0_den[2]);
        // Numeraor config
		serdes_reg_write(serdes, 0x40, 0x0a);
		serdes_reg_write(serdes, 0x41, 0x1e);
		serdes_reg_write(serdes, 0x42, pll_cfg.ch0_num[0]);
		serdes_reg_write(serdes, 0x42, pll_cfg.ch0_num[1]);
		serdes_reg_write(serdes, 0x42, pll_cfg.ch0_num[2]);
        // Post-divider config
		serdes_reg_write(serdes, 0x40, 0x08);
		serdes_reg_write(serdes, 0x41, 0x13);
		serdes_reg_write(serdes, 0x42, pll_cfg.ch0_pdiv);
    }

    extcon_set_state(serdes->extcon, EXTCON_JACK_VIDEO_OUT, true);

    return 0;
}

static bool ds90uh981_bridge_link_locked(struct serdes *serdes)
{
    u32 val = 0, i;

    if(serdes->lock_gpio) {
        for(i = 0; i < 3; i++) {
            val = gpiod_get_value_cansleep(serdes->lock_gpio);
            if(val)
                break;
            msleep(20);
        }

        SERDES_DBG_CHIP("%s:%s-%s, gpio %s\n", __func__, dev_name(serdes->dev),
                serdes->chip_data->name, (val) ? "locked" : "unlocked");

        if(val)
            return true;
    }

    if(serdes_reg_read(serdes, 0x002a, &val)) {
        SERDES_DBG_CHIP("serdes %s: unlocked val = 0x%x\n", __func__, val);
        return false;
    }

    if(!FIELD_GET(LINK_LOCKED, val)) {
        SERDES_DBG_CHIP("serdes %s: unlocked val = 0x%x\n", __func__, val);
        return false;
    }

    SERDES_DBG_CHIP("%s: serdes reg locked val = 0x%x\n", __func__, val);

    return true;
}

static int ds90uh981_bridge_attach(struct serdes *serdes)
{
    if(ds90uh981_bridge_link_locked(serdes))
        serdes->serdes_bridge->status = connector_status_connected;
    else
        serdes->serdes_bridge->status = connector_status_disconnected;

    return 0;
}

static enum drm_connector_status 
ds90uh981_bridge_detect(struct serdes *serdes)
{
    struct serdes_bridge *serdes_bridge = serdes->serdes_bridge;
    enum drm_connector_status status = connector_status_connected;

    if(!drm_kms_helper_is_poll_worker())
        return serdes_bridge->status;
    
    if(!ds90uh981_bridge_link_locked(serdes)) {
        status = connector_status_disconnected;
        goto out;
    }

    if(extcon_get_state(serdes->extcon, EXTCON_JACK_VIDEO_OUT)) {
        u32 dprx_trn_status2;

        if(atomic_cmpxchg(&serdes_bridge->triggered, 1, 0)) {
            status = connector_status_disconnected;
            SERDES_DBG_CHIP("1 status = %d state = %d\n", status, serdes->extcon->state);
            goto out;
        }

        if(serdes_reg_read(serdes, 0x641a, &dprx_trn_status2)) {
            status = connector_status_disconnected;
            SERDES_DBG_CHIP("2 status = %d state = %d\n", status, serdes->extcon->state);
            goto out;
        }

        if((dprx_trn_status2 & DPRX_TRAIN_STATE) != DPRX_TRAIN_STATE) {
            dev_err(serdes->dev, "Taining State: 0x%lx\n",
                    FIELD_GET(DPRX_TRAIN_STATE, dprx_trn_status2));
            SERDES_DBG_CHIP("3 status = %d state = %d\n", status, serdes->extcon->state);
        }
    } else {
        atomic_set(&serdes_bridge->triggered, 0);
        SERDES_DBG_CHIP("4 status = %d state = %d\n", status, serdes->extcon->state);
    }

    if(serdes_bridge->next_bridge && (serdes_bridge->next_bridge->ops & DRM_BRIDGE_OP_DETECT))
        return drm_bridge_detect(serdes_bridge->next_bridge);

out:
    serdes_bridge->status = status;

    SERDES_DBG_CHIP("%s: %s %s status = %d state = %d\n", __func__, dev_name(serdes->dev),
                serdes->chip_data->name, status, serdes->extcon->state);

    return status;
}

static int ds90uh981_bridge_enable(struct serdes *serdes)
{
    int ret = 0;

    SERDES_DBG_CHIP("%s: serdes chip %s ret = %d status = %d\n",
            __func__, serdes->chip_data->name, ret, serdes->extcon->state);

    return ret;
}

static int ds90uh981_bridge_disable(struct serdes *serdes)
{
    return 0;
}

static void ds90uh981_bridge_mode_set(struct serdes *serdes,
                                      const struct drm_display_mode *mode,
                                      const struct drm_display_mode *adjusted_mode)
{
    uint16_t h_total, h_active, h_front_porch, h_back_porch, h_sync_len;
    uint16_t v_total, v_active, v_front_porch, v_back_porch, v_sync_len;

    /* program ds90uh981 VP pages */
    if(!adjusted_mode) {
        h_total = adjusted_mode->htotal;
        v_total = adjusted_mode->vtotal;
        h_active = adjusted_mode->hdisplay;
        v_active = adjusted_mode->vdisplay;
        h_front_porch = adjusted_mode->hsync_start - adjusted_mode->hdisplay; 
        v_front_porch = adjusted_mode->vsync_start - adjusted_mode->vdisplay; 
        h_sync_len = adjusted_mode->hsync_end - adjusted_mode->hsync_start; 
        v_sync_len = adjusted_mode->vsync_end - adjusted_mode->vsync_start; 
        h_back_porch = adjusted_mode->htotal - adjusted_mode->hsync_end; 
        v_back_porch = adjusted_mode->vtotal - adjusted_mode->vsync_end; 

		serdes_reg_write(serdes, 0x40, 0x32);   // select page 12
		serdes_reg_write(serdes, 0x41, 0x02);
		serdes_reg_write(serdes, 0x42, h_active & 0xFF);
		serdes_reg_write(serdes, 0x42, (h_active & 0xFF00) >> 8);
		serdes_reg_write(serdes, 0x41, 0x10);
		serdes_reg_write(serdes, 0x42, h_active & 0xFF);
		serdes_reg_write(serdes, 0x42, (h_active & 0xFF00) >> 8);
		serdes_reg_write(serdes, 0x42, h_back_porch & 0xFF);
		serdes_reg_write(serdes, 0x42, (h_back_porch & 0xFF00) >> 8);
		serdes_reg_write(serdes, 0x42, h_sync_len & 0xFF);
		serdes_reg_write(serdes, 0x42, (h_sync_len & 0xFF00) >> 8);
		serdes_reg_write(serdes, 0x42, h_total & 0xFF);
		serdes_reg_write(serdes, 0x42, (h_total & 0xFF00) >> 8);

		serdes_reg_write(serdes, 0x42, v_total & 0xFF);
		serdes_reg_write(serdes, 0x42, (v_total & 0xFF00) >> 8);
		serdes_reg_write(serdes, 0x42, v_active & 0xFF);
		serdes_reg_write(serdes, 0x42, (v_active & 0xFF00) >> 8);
		serdes_reg_write(serdes, 0x42, v_back_porch & 0xFF);
		serdes_reg_write(serdes, 0x42, (v_back_porch & 0xFF00) >> 8);
		serdes_reg_write(serdes, 0x42, v_sync_len & 0xFF);
		serdes_reg_write(serdes, 0x42, (v_sync_len & 0xFF00) >> 8);
		serdes_reg_write(serdes, 0x42, v_front_porch & 0xFF);
		serdes_reg_write(serdes, 0x42, (v_front_porch & 0xFF00) >> 8);
        
        SERDES_DBG_CHIP("%s: clock=%d hdisplay=%d vdisplay=%d\n", __func__,
                        mode->clock, mode->hdisplay, mode->vdisplay);
    }
}

static struct serdes_chip_bridge_ops ds90uh981_bridge_ops = {
    .init = ds90uh981_bridge_init,
    .attach = ds90uh981_bridge_attach,
    .detect = ds90uh981_bridge_detect,
    .mode_set = ds90uh981_bridge_mode_set,
    .enable = ds90uh981_bridge_enable,
    .disable = ds90uh981_bridge_disable,
};

static int ds90uh981_pinctrl_set_mux(struct serdes *serdes,
                        unsigned int function, unsigned int group)
{
    struct serdes_pinctrl *pinctrl = serdes->pinctrl;
    struct function_desc *func;
    struct group_desc *grp;
    int i;

    func = pinmux_generic_get_function(pinctrl->pctl, function);
    if(!func)
        return -EINVAL;

    grp = pinctrl_generic_get_group(pinctrl->pctl, group);
    if(!grp)
        return -EINVAL;

    SERDES_DBG_CHIP("%s: serdes chip %s func = %s data = %p group = %s data = %p, num_pin = %d",
            __func__, serdes->chip_data->name,
            func->name, func->data, grp->name, grp->data, grp->num_pins);

    if(func->data) {
        struct serdes_function_data *data = func->data;

        for(i = 0; i < grp->num_pins; i++) {
            serdes_set_bits(serdes,
                            GPIO_A_REG(grp->pins[i] - pinctrl->pin_base),
                            GPIO_OUT_DIS,
                            FIELD_PREP(GPIO_OUT_DIS, data->gpio_out_dis));
            serdes_set_bits(serdes,
                            GPIO_B_REG(grp->pins[i] - pinctrl->pin_base),
                            OUT_TYPE,
                            FIELD_PREP(OUT_TYPE, 1));
            if(data->gpio_tx_en_a || data->gpio_tx_en_b)
                serdes_set_bits(serdes,
                                GPIO_B_REG(grp->pins[i] - pinctrl->pin_base),
                                GPIO_TX_ID,
                                FIELD_PREP(GPIO_TX_ID, data->gpio_tx_id));
            if(data->gpio_rx_en_a || data->gpio_rx_en_b)
                serdes_set_bits(serdes,
                                GPIO_C_REG(grp->pins[i] - pinctrl->pin_base),
                                GPIO_TX_ID,
                                FIELD_PREP(GPIO_RX_ID, data->gpio_rx_id));
            serdes_set_bits(serdes,
                            GPIO_D_REG(grp->pins[i] - pinctrl->pin_base),
                            GPIO_TX_EN_A | GPIO_RX_EN_A,
                            FIELD_PREP(GPIO_TX_EN_A, data->gpio_tx_en_a) |
                            FIELD_PREP(GPIO_RX_EN_A, data->gpio_rx_en_a) |
                            FIELD_PREP(GPIO_IO_RX_EN, data->gpio_io_rx_en));
        }
    }

    return 0;
}

static int ds90uh981_pinctrl_config_set(struct serdes *serdes,
                        unsigned int pin, unsigned long *configs,
                        unsigned int num_configs)
{
    enum pin_config_param param;
    u32 arg;
    u8 res_cfg;
    int i;

    for(i = 0; i < num_configs; i++) {
        param = pinconf_to_config_param(configs[i]);
        arg = pinconf_to_config_argument(configs[i]);

        SERDES_DBG_CHIP("%s: serdes chip %s pin = %d param = %d\n", __func__,
                   serdes->chip_data->name, pin, param);

        switch(param) {
        case PIN_CONFIG_DRIVE_OPEN_DRAIN:
            serdes_set_bits(serdes, GPIO_B_REG(pin),
                            OUT_TYPE, FIELD_PREP(OUT_TYPE, 0));
            break;
        case PIN_CONFIG_DRIVE_PUSH_PULL:
            serdes_set_bits(serdes, GPIO_B_REG(pin),
                            OUT_TYPE, FIELD_PREP(OUT_TYPE, 1));
            break;
        case PIN_CONFIG_BIAS_DISABLE:
            serdes_set_bits(serdes, GPIO_C_REG(pin),
                            PULL_UPDN_SEL, FIELD_PREP(PULL_UPDN_SEL, 0));
            break;
        case PIN_CONFIG_BIAS_PULL_UP:
            switch(arg) {
            case 40000:
                res_cfg = 0;
                break;
            case 1000000:
                res_cfg = 1;
                break;
            default:
                return -EINVAL;
            }

            serdes_set_bits(serdes, GPIO_A_REG(pin),
                            RES_CFG, FIELD_PREP(RES_CFG, res_cfg));
            serdes_set_bits(serdes, GPIO_C_REG(pin),
                            PULL_UPDN_SEL, FIELD_PREP(PULL_UPDN_SEL, 1));
            break;
        case PIN_CONFIG_BIAS_PULL_DOWN:
            switch(arg) {
            case 40000:
                res_cfg = 0;
                break;
            case 1000000:
                res_cfg = 1;
                break;
            default:
                return -EINVAL;
            }

            serdes_set_bits(serdes, GPIO_A_REG(pin),
                            RES_CFG, FIELD_PREP(RES_CFG, res_cfg));
            serdes_set_bits(serdes, GPIO_C_REG(pin),
                            PULL_UPDN_SEL, FIELD_PREP(PULL_UPDN_SEL, 2));
            break;
        case PIN_CONFIG_OUTPUT:
            serdes_set_bits(serdes, GPIO_A_REG(pin),
                            GPIO_OUT_DIS | GPIO_OUT, 
                            FIELD_PREP(GPIO_OUT_DIS, 0) |
                            FIELD_PREP(GPIO_OUT, arg));
            break;
        default:
            return -EOPNOTSUPP;
        }
    }

    return 0;
}

static int ds90uh981_pinctrl_config_get(struct serdes *serdes,
                        unsigned int pin, unsigned long *config)
{
    enum pin_config_param param = pinconf_to_config_param(*config);
    unsigned int gpio_a_reg, gpio_b_reg;
    u16 arg;

    serdes_reg_read(serdes, GPIO_A_REG(pin), &gpio_a_reg);
    serdes_reg_read(serdes, GPIO_B_REG(pin), &gpio_b_reg);

    SERDES_DBG_CHIP("%s: serdes chip %s pin = %d param = %d\n", __func__,
                   serdes->chip_data->name, pin, param);

    switch(param) {
    case PIN_CONFIG_DRIVE_OPEN_DRAIN:
        if(FIELD_GET(OUT_TYPE, gpio_b_reg))
            return -EINVAL;
        break;
    case PIN_CONFIG_DRIVE_PUSH_PULL:
        if(!FIELD_GET(OUT_TYPE, gpio_b_reg))
            return -EINVAL;
        break;
    case PIN_CONFIG_BIAS_DISABLE:
        if(FIELD_GET(PULL_UPDN_SEL, gpio_b_reg) != 0)
            return -EINVAL;
        break;
    case PIN_CONFIG_BIAS_PULL_UP:
        if(FIELD_GET(PULL_UPDN_SEL, gpio_b_reg) != 1)
            return -EINVAL;
        switch(FIELD_GET(RES_CFG, gpio_a_reg)) {
        case 0:
            arg = 40000;
            break;
        case 1:
            arg = 10000;
            break;
        }
        break;
    case PIN_CONFIG_BIAS_PULL_DOWN:
        if(FIELD_GET(PULL_UPDN_SEL, gpio_b_reg) != 2)
            return -EINVAL;
        switch(FIELD_GET(RES_CFG, gpio_a_reg)) {
        case 0:
            arg = 40000;
            break;
        case 1:
            arg = 10000;
            break;
        }
        break;
    case PIN_CONFIG_OUTPUT:
        if(FIELD_GET(GPIO_OUT_DIS, gpio_a_reg))
            return -EINVAL;
        arg = FIELD_GET(GPIO_OUT, gpio_a_reg);
        break;
    default:
        return -EOPNOTSUPP;
    }

    *config = pinconf_to_config_packed(param, arg);

    return 0;
}

static struct serdes_chip_pinctrl_ops ds90uh981_pinctrl_ops = {
    .pin_config_get = ds90uh981_pinctrl_config_get,
    .pin_config_set = ds90uh981_pinctrl_config_set,
    .set_mux        = ds90uh981_pinctrl_set_mux,
};

static int ds90uh981_gpio_direction_input(struct serdes *serdes, int gpio)
{
    return 0;
}

static int ds90uh981_gpio_direction_ouput(struct serdes *serdes, int gpio, int value)
{
    return 0;
}

static int ds90uh981_gpio_get_level(struct serdes *serdes, int gpio)
{
    return 0;
}

static int ds90uh981_gpio_set_level(struct serdes *serdes, int gpio, int value)
{
    return 0;
}

static int ds90uh981_gpio_set_config(struct serdes *serdes, int gpio, unsigned long config)
{
    return 0;
}

static int ds90uh981_gpio_to_irq(struct serdes *serdes, int gpio)
{
    return 0;
}

static struct serdes_chip_gpio_ops ds90uh981_gpio_ops = {
    .direction_input        = ds90uh981_gpio_direction_input,
    .direction_output       = ds90uh981_gpio_direction_ouput,
    .get_level              = ds90uh981_gpio_get_level,
    .set_level              = ds90uh981_gpio_set_level,
    .set_config             = ds90uh981_gpio_set_config,
    .to_irq                 = ds90uh981_gpio_to_irq,
};

static const struct check_reg_data ds90uh981_important_reg[10] = {
    {
        "ds90uh981 LINK LOCK",
        { 0x0013, (1 << 3) },
    },
    {
        "ds90uh981 LINKA LOCK",
        { 0x002A, (1 << 3) },
    },
    {
        "ds90uh981 X PLCK DET",
        { 0x0102, (1 << 7) },
    },
    {
        "ds90uh981 Y PLCK DET",
        { 0x0112, (1 << 7) },
    },
};

static int ds90uh981_check_reg(struct serdes *serdes)
{
    int i, ret;
    unsigned int val;

    for(i = 0; i < ARRAY_SIZE(ds90uh981_important_reg); i++) {
        if(!ds90uh981_important_reg[i].seq.reg)
            break;

        ret = serdes_reg_read(serdes, ds90uh981_important_reg[i].seq.reg, &val);
        if(!ret && !(val & ds90uh981_important_reg[i].seq.def)
           && (!atomic_read(&serdes->flag_early_suspend)))
            dev_info(serdes->dev, "warning %s %s reg[0x%x] = 0x%x\n", __func__,
                     ds90uh981_important_reg[i].name,
                     ds90uh981_important_reg[i].seq.reg, val);
            
    }

    return 0;
}

static int ds90uh981_identify(struct serdes *serdes)
{
    unsigned int val;
    int ret;
    struct i2c_client *client = to_i2c_client(serdes->dev);

    ret = serdes_reg_read(serdes, DEVICE_ADDR_REG, &val);
    if (ret)
        return ret;

    dev_info(serdes->dev, "I2C Addr: 0x%02x, Device ID read: 0x%02x\n", 
             client->addr, val);

    if (val != client->addr << 1) {
        dev_err(serdes->dev, "Device ID mismatch: 0x%02x (expected 0x98)\n", val);
        return -ENODEV;
    }

    return 0;
}

static struct serdes_check_reg_ops ds90uh981_check_reg_ops = {
    .check_reg = ds90uh981_check_reg,
    .identify  = ds90uh981_identify,
};

static int  ds90uh981_pm_suspend(struct serdes *serdes)
{
    return 0;
}

static int  ds90uh981_pm_resume(struct serdes *serdes)
{
    return 0;
}

static struct serdes_chip_pm_ops ds90uh981_pm_ops = {
    .suspend = ds90uh981_pm_suspend,
    .resume  = ds90uh981_pm_resume,
};

static int ds90uh981_irq_lock_handle(struct serdes *serdes)
{
    return IRQ_HANDLED;
}

static int ds90uh981_irq_err_handle(struct serdes *serdes)
{
    return IRQ_HANDLED;
}

static struct serdes_chip_irq_ops ds90uh981_irq_ops = {
    .lock_handle = ds90uh981_irq_lock_handle,
    .err_handle  = ds90uh981_irq_err_handle,
};


static const struct mfd_cell serdes_ds90uh981_devs[] = {
    {
        .name = "serdes-pinctrl",
        .of_compatible = "ti,ds90uh981-pinctrl",
    },
    {
        .name = "serdes-bridge",
        .of_compatible = "ti,ds90uh981-bridge",
    },
};

struct serdes_chip_data serdes_ds90uh981_data  = {
    .name               = "ds90uh981",
    .serdes_type        = TYPE_SER,
    .serdes_id          = TI_ID_DS90UH981,
    .connector_type     = DRM_MODE_CONNECTOR_LVDS,
    .regmap_config      = &ds90uh981_regmap_config,
    .pinctrl_info       = &ds90uh981_pinctrl_info,
    .bridge_ops         = &ds90uh981_bridge_ops,
    .pinctrl_ops        = &ds90uh981_pinctrl_ops,
    .gpio_ops           = &ds90uh981_gpio_ops,
    .check_ops          = &ds90uh981_check_reg_ops,
    .pm_ops             = &ds90uh981_pm_ops,
    .irq_ops            = &ds90uh981_irq_ops,
    .mfd_cells          = serdes_ds90uh981_devs,
    .num_cells          = ARRAY_SIZE(serdes_ds90uh981_devs),
};

static const struct of_device_id ds90uh981_of_match[] = {
    { .compatible = "ti,ds90uh981", .data = &serdes_ds90uh981_data },
    { }
};
MODULE_DEVICE_TABLE(of, ds90uh981_of_match);

static struct i2c_driver ds90uh981_driver = {
    .driver = {
        .name = "ds90uh981",
        .of_match_table = ds90uh981_of_match,
        .pm = &serdes_pm_ops,
    },
    .probe = serdes_i2c_probe,
    .remove = serdes_i2c_remove,
    .shutdown = serdes_i2c_shutdown,
};
module_i2c_driver(ds90uh981_driver);

MODULE_LICENSE("GPL");
