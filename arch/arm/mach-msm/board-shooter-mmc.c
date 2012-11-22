/* linux/arch/arm/mach-msm/board-shooter-mmc.c
 *
 * Copyright (C) 2008 HTC Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/err.h>
#include <linux/debugfs.h>
#include <linux/gpio.h>

#include <asm/gpio.h>
#include <asm/io.h>

#include <mach/vreg.h>
#include <mach/htc_pwrsink.h>

#include <asm/mach/mmc.h>

#include "devices.h"
#include "board-shooter.h"
#include "proc_comm.h"
#include "board-common-wimax.h"

#include <mach/msm_iomap.h>
#include <linux/mfd/pmic8058.h>
#include <linux/irq.h>
#include "board-shooter-mmc.h"

#include <mach/rpm.h>
#include <mach/rpm-regulator.h>

#include "mpm.h"
#include "rpm_resources.h"

int msm_proc_comm(unsigned cmd, unsigned *data1, unsigned *data2);

/* ---- PM QOS ---- */
static struct msm_rpmrs_level msm_rpmrs_levels[] = {
	{
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		true,
		1, 8000, 100000, 1,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		true,
		1500, 5000, 60100000, 3000,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		false,
		1800, 5000, 60350000, 3500,
	},
	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, ACTIVE, MAX, ACTIVE),
		false,
		3800, 4500, 65350000, 5500,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(ON, HSFS_OPEN, MAX, ACTIVE),
		false,
		2800, 2500, 66850000, 4800,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, MAX, ACTIVE),
		false,
		4800, 2000, 71850000, 6800,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, ACTIVE, RET_HIGH),
		false,
		6800, 500, 75850000, 8800,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, RET_HIGH, RET_LOW),
		false,
		7800, 0, 76350000, 9800,
	},
};

static uint32_t msm_rpm_get_swfi_latency(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(msm_rpmrs_levels); i++) {
		if (msm_rpmrs_levels[i].sleep_mode ==
			MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)
			return msm_rpmrs_levels[i].latency_us;
	}

	return 0;
}


/* ---- SDCARD ---- */
/* ---- WIFI ---- */

static uint32_t wifi_on_gpio_table[] = {
	GPIO_CFG(shooter_GPIO_WIFI_IRQ, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA), /* WLAN IRQ */
};

static uint32_t wifi_off_gpio_table[] = {
	GPIO_CFG(shooter_GPIO_WIFI_IRQ, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_4MA), /* WLAN IRQ */
};

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

/* BCM4329 returns wrong sdio_vsn(1) when we read cccr,
 * we use predefined value (sdio_vsn=2) here to initial sdio driver well
 */
static struct embedded_sdio_data shooter_wifi_emb_data = {
	.cccr	= {
		.sdio_vsn	= 2,
		.multi_block	= 1,
		.low_speed	= 0,
		.wide_bus	= 0,
		.high_power	= 1,
		.high_speed	= 1,
	}
};

static void (*wifi_status_cb)(int card_present, void *dev_id);
static void *wifi_status_cb_devid;

static int
shooter_wifi_status_register(void (*callback)(int card_present, void *dev_id),
				void *dev_id)
{
	if (wifi_status_cb)
		return -EAGAIN;

	wifi_status_cb = callback;
	wifi_status_cb_devid = dev_id;
	return 0;
}

static int shooter_wifi_cd;	/* WiFi virtual 'card detect' status */

static unsigned int shooter_wifi_status(struct device *dev)
{
	return shooter_wifi_cd;
}

static unsigned int shooter_wifislot_type = MMC_TYPE_SDIO_WIFI;
static struct mmc_platform_data shooter_wifi_data = {
	.ocr_mask               = MMC_VDD_28_29,
	.status                 = shooter_wifi_status,
	.register_status_notify = shooter_wifi_status_register,
	.embedded_sdio          = &shooter_wifi_emb_data,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.slot_type = &shooter_wifislot_type,
	.msmsdcc_fmin   = 400000,
	.msmsdcc_fmid   = 24000000,
	.msmsdcc_fmax   = 48000000,
	.nonremovable   = 0,
	.pclk_src_dfab	= 1,
};


int shooter_wifi_set_carddetect(int val)
{
	printk(KERN_INFO "%s: %d\n", __func__, val);
	shooter_wifi_cd = val;
	if (wifi_status_cb)
		wifi_status_cb(val, wifi_status_cb_devid);
	else
		printk(KERN_WARNING "%s: Nobody to notify\n", __func__);
	return 0;
}
EXPORT_SYMBOL(shooter_wifi_set_carddetect);

int shooter_wifi_power(int on)
{
	const unsigned SDC4_HDRV_PULL_CTL_ADDR = (unsigned) MSM_TLMM_BASE + 0x20A0;

	printk(KERN_INFO "%s: %d\n", __func__, on);

	if (on) {
		writel(0x1FDB, SDC4_HDRV_PULL_CTL_ADDR);
		config_gpio_table(wifi_on_gpio_table,
				  ARRAY_SIZE(wifi_on_gpio_table));
	} else {
		writel(0x0BDB, SDC4_HDRV_PULL_CTL_ADDR);
		config_gpio_table(wifi_off_gpio_table,
				  ARRAY_SIZE(wifi_off_gpio_table));
	}
	mdelay(1); /* Delay 1 ms, Recommand by Hardware */
	gpio_set_value(shooter_GPIO_WIFI_SHUTDOWN_N, on); /* WIFI_SHUTDOWN */

	mdelay(120);
	return 0;
}
EXPORT_SYMBOL(shooter_wifi_power);

int shooter_wifi_reset(int on)
{
	printk(KERN_INFO "%s: do nothing\n", __func__);
	return 0;
}

#ifdef CONFIG_WIMAX
/* ---- WIMAX ---- */
static uint32_t wimax_on_gpio_table[] = {
	GPIO_CFG(SHOOTER_GPIO_WIMAX_SDIO_D0,  2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), /* DAT0 */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_SDIO_D1,  2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), /* DAT1 */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_SDIO_D2,  2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), /* DAT2 */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_SDIO_D3,  2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), /* DAT3 */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_SDIO_CMD, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), /* CMD */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_SDIO_CLK_CPU, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), /* CLK */
	GPIO_CFG(SHOOTER_GPIO_V_WIMAX_1V2_RF_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),  /* 1v2RF */
	GPIO_CFG(SHOOTER_GPIO_V_WIMAX_DVDD_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),    /* DVDD */
	GPIO_CFG(SHOOTER_GPIO_V_WIMAX_PVDD_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),    /* PVDD */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_EXT_RST,   0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),    /* EXT_RST */
};

static uint32_t wimax_off_gpio_table[] = {
	GPIO_CFG(SHOOTER_GPIO_WIMAX_SDIO_D0,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* DAT0 */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_SDIO_D1,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* DAT1 */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_SDIO_D2,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* DAT2 */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_SDIO_D3,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* DAT3 */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_SDIO_CMD, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* CMD */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_SDIO_CLK_CPU, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), /* CLK */
	GPIO_CFG(SHOOTER_GPIO_V_WIMAX_1V2_RF_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),  /* 1v2RF */
	GPIO_CFG(SHOOTER_GPIO_V_WIMAX_DVDD_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),    /* DVDD */
	GPIO_CFG(SHOOTER_GPIO_V_WIMAX_PVDD_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),    /* PVDD */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_EXT_RST,   0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* EXT_RST */
};

static uint32_t wimax_initial_gpio_table[] = {
	GPIO_CFG(SHOOTER_GPIO_WIMAX_SDIO_D0,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* DAT0 */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_SDIO_D1,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* DAT1 */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_SDIO_D2,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* DAT2 */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_SDIO_D3,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* DAT3 */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_SDIO_CMD, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* CMD */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_SDIO_CLK_CPU, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), /* CLK */
	GPIO_CFG(SHOOTER_GPIO_V_WIMAX_1V2_RF_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),  /* 1v2RF */
	GPIO_CFG(SHOOTER_GPIO_V_WIMAX_DVDD_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),    /* DVDD */
	GPIO_CFG(SHOOTER_GPIO_V_WIMAX_PVDD_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),    /* PVDD */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_EXT_RST,   0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* EXT_RST */
};

static uint32_t wimax_uart_on_gpio_table[] = {
	/* UART3 TX */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_UART_SIN, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	/* UART3 RX , setting PULL_UP , NO_PULL is all ok */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_UART_SOUT, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
};

static uint32_t wimax_uart_off_gpio_table[] = {
	/* UART3 TX */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_UART_SIN, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	/* UART3 RX */
	GPIO_CFG(SHOOTER_GPIO_WIMAX_UART_SOUT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
};

struct pm8058_WIMAX_gpio_cfg {
	int gpio;
	/* struct pm8058_gpio cfg; */
	struct pm_gpio cfg; /* For Kernel 3.0 */
};

struct pm8058_WIMAX_gpio_cfg wimax_debug_gpio_cfgs[] =
{
	{ /* WIMAX_DEBIG12 */
		SHOOTER_WIMAX_DEBUG12,
		{
			.direction      = PM_GPIO_DIR_OUT,
			.output_value	= 0,
			.pull           = PM_GPIO_PULL_NO,
			.vin_sel        = PM8058_GPIO_VIN_S3,
		},
	},
	{ /* WIMAX_DEBIG14 */
		SHOOTER_WIMAX_DEBUG14,
		{
			.direction      = PM_GPIO_DIR_OUT,
			.output_value	= 0,
			.pull           = PM_GPIO_PULL_NO,
			.vin_sel        = PM8058_GPIO_VIN_S3,
		},
	},
	{ /* WIMAX_DEBIG15 */
		SHOOTER_WIMAX_DEBUG15,
		{
			.direction      = PM_GPIO_DIR_OUT,
			.output_value	= 0,
			.pull           = PM_GPIO_PULL_NO,
			.vin_sel        = PM8058_GPIO_VIN_S3,
		},
	},
};
/* for wimax power on */
struct pm8058_WIMAX_gpio_cfg wimax_debug_gpio_PWON_cfgs[] =
{
	{ /* WIMAX_DEBIG12 */
		SHOOTER_WIMAX_DEBUG12,
		{
			.direction      = PM_GPIO_DIR_OUT,
			.output_value	= 0,
			.pull           = PM_GPIO_PULL_NO,
			.vin_sel        = PM8058_GPIO_VIN_S3,
		},
	},
	{ /* WIMAX_DEBIG14 */
		SHOOTER_WIMAX_DEBUG14,
		{
			.direction      = PM_GPIO_DIR_IN,
			.output_value	= 0,
			.pull           = PM_GPIO_PULL_DN,
			.vin_sel        = PM8058_GPIO_VIN_S3,
		},
	},
	{ /* WIMAX_DEBIG15 */
		SHOOTER_WIMAX_DEBUG15,
		{
			.direction      = PM_GPIO_DIR_OUT,
			.output_value	= 0,
			.pull           = PM_GPIO_PULL_NO,
			.vin_sel        = PM8058_GPIO_VIN_S3,
		},
	},
};


static void (*wimax_status_cb)(int card_present, void *dev_id);
static void *wimax_status_cb_devid;
static int mmc_wimax_cd = 0;
static int mmc_wimax_hostwakeup_gpio = PM8058_GPIO_PM_TO_SYS(SHOOTER_WIMAX_HOST_WAKEUP);

static int mmc_wimax_status_register(void (*callback)(int card_present, void *dev_id), void *dev_id)
{
	if (wimax_status_cb)
		return -EAGAIN;
	printk(KERN_INFO "[WIMAX] %s\n", __func__);
	wimax_status_cb = callback;
	wimax_status_cb_devid = dev_id;
	return 0;
}

static unsigned int mmc_wimax_status(struct device *dev)
{
	printk(KERN_INFO "[WIMAX] %s\n", __func__);
	return mmc_wimax_cd;
}

void mmc_wimax_set_carddetect(int val)
{
	printk(KERN_INFO "[WIMAX] %s: %d\n", __func__, val);
	mmc_wimax_cd = val;
	if (wimax_status_cb)
		wimax_status_cb(val, wimax_status_cb_devid);
	else
		printk(KERN_WARNING "[WIMAX] %s: Nobody to notify\n", __func__);
}
EXPORT_SYMBOL(mmc_wimax_set_carddetect);

static unsigned int mmc_wimax_type = MMC_TYPE_SDIO_WIMAX;

static struct mmc_platform_data mmc_wimax_data = {
	.ocr_mask		= MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30,
	.status			= mmc_wimax_status,
	.register_status_notify	= mmc_wimax_status_register,
	.embedded_sdio		= NULL,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.msmsdcc_fmin   = 400000,
	.msmsdcc_fmid   = 24000000,
	.msmsdcc_fmax   = 48000000,
	.nonremovable   = 0,
	.slot_type		= &mmc_wimax_type,
	.pclk_src_dfab	= 1,
/*	.dummy52_required = 1,*/
};

struct _vreg {
	const char *name;
	unsigned id;
};

int mmc_wimax_power(int on)
{
	printk(KERN_INFO "[WIMAX] %s\n", __func__);

	if (on)	{
	/*Power ON sequence*/
		gpio_set_value(SHOOTER_GPIO_V_WIMAX_PVDD_EN, 1);  /* V_WIMAX_PVDD_EN */
		mdelay(10);
		gpio_set_value(SHOOTER_GPIO_V_WIMAX_DVDD_EN, 1);   /* V_WIMAX_DVDD_EN */
		mdelay(3);
		gpio_set_value(SHOOTER_GPIO_V_WIMAX_1V2_RF_EN, 1); /* V_WIMAX_1V2_RF_EN */
		mdelay(130);
		config_gpio_table(wimax_uart_on_gpio_table, ARRAY_SIZE(wimax_uart_on_gpio_table));/* Configure UART3 TX/RX */
		config_gpio_table(wimax_on_gpio_table, ARRAY_SIZE(wimax_on_gpio_table));
#if 0
		pm8058_gpio_config(wimax_debug_gpio_PWON_cfgs[0].gpio, &wimax_debug_gpio_PWON_cfgs[0].cfg);
		pm8058_gpio_config(wimax_debug_gpio_PWON_cfgs[1].gpio, &wimax_debug_gpio_PWON_cfgs[1].cfg);
		pm8058_gpio_config(wimax_debug_gpio_PWON_cfgs[2].gpio, &wimax_debug_gpio_PWON_cfgs[2].cfg);
#endif
		mdelay(3);
		gpio_set_value(SHOOTER_GPIO_WIMAX_EXT_RST, 1);     /* WIMAX_EXT_RSTz */
	} else {
		/*Power OFF sequence*/
		gpio_set_value(SHOOTER_GPIO_WIMAX_EXT_RST, 0);     /* WIMAX_EXT_RSTz */
		config_gpio_table(wimax_uart_off_gpio_table, ARRAY_SIZE(wimax_uart_off_gpio_table));/* Configure UART3 TX/RX */
		config_gpio_table(wimax_off_gpio_table, ARRAY_SIZE(wimax_off_gpio_table));
#if 0
		pm8058_gpio_config(wimax_debug_gpio_cfgs[0].gpio, &wimax_debug_gpio_cfgs[0].cfg);
		pm8058_gpio_config(wimax_debug_gpio_cfgs[1].gpio, &wimax_debug_gpio_cfgs[1].cfg);
		pm8058_gpio_config(wimax_debug_gpio_cfgs[2].gpio, &wimax_debug_gpio_cfgs[2].cfg);
#endif
		mdelay(5);
		gpio_set_value(SHOOTER_GPIO_V_WIMAX_1V2_RF_EN, 0); /* V_WIMAX_1V2_RF_EN */
		mdelay(3);
		gpio_set_value(SHOOTER_GPIO_V_WIMAX_DVDD_EN, 0);   /* V_WIMAX_DVDD_EN */
		mdelay(3);
		gpio_set_value(SHOOTER_GPIO_V_WIMAX_PVDD_EN, 0);  /* V_WIMAX_PVDD_EN */
	}

	return 0;
}
EXPORT_SYMBOL(mmc_wimax_power);

int wimax_uart_switch = 0;
int mmc_wimax_uart_switch(int uart)
{
	printk(KERN_INFO "[WIMAX] %s uart:%d\n", __func__, uart);
	wimax_uart_switch = uart;

	if (wimax_uart_switch == 0) { /* Initialize */
		gpio_set_value(SHOOTER_GPIO_CPU_WIMAX_UART_EN, 0);
		gpio_set_value(SHOOTER_GPIO_CPU_WIMAX_SW, 0);
		/* gpio_set_value(SHOOTER_GPIO_MHL_USB_EN, 1); */

	} else if (wimax_uart_switch == 1) { /* enable WIMAX UART to USB */
		config_gpio_table(wimax_uart_off_gpio_table,
			ARRAY_SIZE(wimax_uart_off_gpio_table)); /* Disable UART3 to GPIO */

		gpio_set_value(SHOOTER_GPIO_CPU_WIMAX_UART_EN, 0);
		gpio_set_value(SHOOTER_GPIO_CPU_WIMAX_SW, 1);
		/* gpio_set_value(SHOOTER_GPIO_MHL_USB_EN, 1); */
	} else if (wimax_uart_switch == 2) { /* enable WIMAX_UART to MSM */
		config_gpio_table(wimax_uart_on_gpio_table,
			ARRAY_SIZE(wimax_uart_on_gpio_table)); /* Enable UART3 to ALT1 */

		gpio_set_value(SHOOTER_GPIO_CPU_WIMAX_UART_EN, 1);
		gpio_set_value(SHOOTER_GPIO_CPU_WIMAX_SW, 0);
		/* gpio_set_value(SHOOTER_GPIO_MHL_USB_EN, 0); */
	} else { /* others, USB to USB */
		gpio_set_value(SHOOTER_GPIO_CPU_WIMAX_UART_EN, 1);
		gpio_set_value(SHOOTER_GPIO_CPU_WIMAX_SW, 0);
		/* gpio_set_value(SHOOTER_GPIO_MHL_USB_EN, 0); */
	}

	return 0;
}
EXPORT_SYMBOL(mmc_wimax_uart_switch);

int mmc_wimax_get_uart_switch(void)
{
	printk(KERN_INFO "[WIMAX] %s uart:%d\n", __func__, wimax_uart_switch);
	return wimax_uart_switch;
}
EXPORT_SYMBOL(mmc_wimax_get_uart_switch);

/*non-8X60 PROJECT need to use GPIO mapping to decode the IRQ number(id) for PMIC GPIO*/
int mmc_wimax_get_hostwakeup_gpio(void)
{
	return mmc_wimax_hostwakeup_gpio;
}
EXPORT_SYMBOL(mmc_wimax_get_hostwakeup_gpio);

/*8X60 PROJECT need to use Marco PM8058_GPIO_IRQ to decode the IRQ number(id) for PMIC GPIO*/
int mmc_wimax_get_hostwakeup_IRQ_ID(void)
{
	return PM8058_GPIO_IRQ(PM8058_IRQ_BASE, SHOOTER_WIMAX_HOST_WAKEUP);
}
EXPORT_SYMBOL(mmc_wimax_get_hostwakeup_IRQ_ID);

void mmc_wimax_enable_host_wakeup(int on)
{
	if (mmc_wimax_get_status()) {
		if (on) {
			if (!mmc_wimax_get_gpio_irq_enabled()) {
				if (printk_ratelimit())
					printk(KERN_INFO "[WIMAX] set PMIC GPIO%d as wakeup source on IRQ %d\n", SHOOTER_WIMAX_HOST_WAKEUP+1, mmc_wimax_get_hostwakeup_IRQ_ID());
				enable_irq(mmc_wimax_get_hostwakeup_IRQ_ID());
				enable_irq_wake(mmc_wimax_get_hostwakeup_IRQ_ID());
				mmc_wimax_set_gpio_irq_enabled(1);
			}
		} else {
			if (mmc_wimax_get_gpio_irq_enabled()) {
				if (printk_ratelimit())
					printk(KERN_INFO "[WIMAX] disable PMIC GPIO%d wakeup source\n", SHOOTER_WIMAX_HOST_WAKEUP+1);
				disable_irq_wake(mmc_wimax_get_hostwakeup_IRQ_ID());
				disable_irq_nosync(mmc_wimax_get_hostwakeup_IRQ_ID());
				mmc_wimax_set_gpio_irq_enabled(0);
			}
		}
	} else
		printk(KERN_INFO "[WIMAX] %s mmc_wimax_sdio_status is OFF\n", __func__);
}
EXPORT_SYMBOL(mmc_wimax_enable_host_wakeup);
#endif

int __init shooter_init_mmc()
{
	uint32_t id;
	wifi_status_cb = NULL;

	printk(KERN_INFO "shooter: %s\n", __func__);
#ifdef CONFIG_WIMAX
    /* SDC2: WiMAX */

    /* PM QoS for wimax */
    mmc_wimax_data.swfi_latency = msm_rpm_get_swfi_latency();
	msm_add_sdcc(2, &mmc_wimax_data);

	/* Re-initialize WiMAX GPIO */
	config_gpio_table(wimax_off_gpio_table,
				ARRAY_SIZE(wimax_off_gpio_table));

	/* Configure UART3 TX/RX */
	config_gpio_table(wimax_uart_off_gpio_table,
				ARRAY_SIZE(wimax_uart_off_gpio_table));
	/* Initialized WiMAX pins */
	config_gpio_table(wimax_initial_gpio_table, ARRAY_SIZE(wimax_initial_gpio_table));
#if 0
	pm8058_gpio_config(SHOOTER_WIMAX_DEBUG12, &wimax_debug_gpio_cfgs[0].cfg);
	pm8058_gpio_config(SHOOTER_WIMAX_DEBUG14, &wimax_debug_gpio_cfgs[1].cfg);
	pm8058_gpio_config(SHOOTER_WIMAX_DEBUG15, &wimax_debug_gpio_cfgs[2].cfg);
#endif
#endif
	/* initial WIFI_SHUTDOWN# */
	id = GPIO_CFG(shooter_GPIO_WIFI_SHUTDOWN_N, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA);
	gpio_tlmm_config(id, 0);
	gpio_set_value(shooter_GPIO_WIFI_SHUTDOWN_N, 0);

   /* PM QoS for wifi*/
   shooter_wifi_data.swfi_latency = msm_rpm_get_swfi_latency();
	msm_add_sdcc(4, &shooter_wifi_data);

	return 0;
}
