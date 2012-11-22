/* linux/arch/arm/mach-msm/board-shooter_u.h
 *
 * Copyright (C) 2010-2011 HTC Corporation.
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

#ifndef __ARCH_ARM_MACH_MSM_BOARD_SHOOTER_U_H
#define __ARCH_ARM_MACH_MSM_BOARD_SHOOTER_U_H

#include <mach/board.h>

#define SHOOTER_U_PROJECT_NAME	"shooter_u"

#define MSM_RAM_CONSOLE_BASE	MSM_HTC_RAM_CONSOLE_PHYS
#define MSM_RAM_CONSOLE_SIZE	MSM_HTC_RAM_CONSOLE_SIZE

/* Memory map */

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)
#define QCE_SIZE		0x10000
#define QCE_0_BASE		0x18500000
#endif

#ifdef CONFIG_FB_MSM_LCDC_DSUB
/* VGA = 1440 x 900 x 4(bpp) x 2(pages)
   prim = 1024 x 600 x 4(bpp) x 2(pages)
   This is the difference. */
#define MSM_FB_DSUB_PMEM_ADDER (0x9E3400-0x4B0000)
#else
#define MSM_FB_DSUB_PMEM_ADDER (0)
#endif

#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
/* prim = 960 x 540 x 4(bpp) x 3(pages) */
#define MSM_FB_PRIM_BUF_SIZE (960 * ALIGN(540, 32) * 4 * 3)
#else
/* prim = 960 x 540 x 4(bpp) x 2(pages) */
#define MSM_FB_PRIM_BUF_SIZE (960 * ALIGN(540, 32) * 4 * 2)
#endif


#ifdef CONFIG_FB_MSM_OVERLAY_WRITEBACK
/* width x height x 3 bpp x 2 frame buffer */
#define MSM_FB_WRITEBACK_SIZE roundup(960 * ALIGN(540, 32) * 3 * 2, 4096)
#else
#define MSM_FB_WRITEBACK_SIZE 0
#endif

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
/* prim = 1024 x 600 x 4(bpp) x 2(pages)
 * hdmi = 1920 x 1080 x 2(bpp) x 1(page)
 * Note: must be multiple of 4096 */
#define MSM_FB_SIZE roundup(MSM_FB_PRIM_BUF_SIZE + 0x3F4800 + MSM_FB_DSUB_PMEM_ADDER, 4096)
#elif defined(CONFIG_FB_MSM_TVOUT)
/* prim = 1024 x 600 x 4(bpp) x 2(pages)
 * tvout = 720 x 576 x 2(bpp) x 2(pages)
 * Note: must be multiple of 4096 */
#define MSM_FB_SIZE roundup(MSM_FB_PRIM_BUF_SIZE + 0x195000 + MSM_FB_DSUB_PMEM_ADDER, 4096)
#else /* CONFIG_FB_MSM_HDMI_MSM_PANEL */
#define MSM_FB_SIZE roundup(MSM_FB_PRIM_BUF_SIZE + MSM_FB_DSUB_PMEM_ADDER, 4096)
#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL */
#define MSM_PMEM_SF_SIZE 0x4000000 /* 64 Mbytes */
#define MSM_OVERLAY_BLT_SIZE   roundup(960 * ALIGN(540, 32) * 3 * 2, 4096)

#define MSM_PMEM_ADSP_SIZE	0x239C000
#define MSM_PMEM_ADSP2_SIZE	0x664000 /* ((1408 * 792 * 1.5) Align 2K) * 2 * 2 */
#define MSM_PMEM_AUDIO_SIZE	0x239000

#define MSM_PMEM_SF_BASE		(0x70000000 - MSM_PMEM_SF_SIZE)
#define MSM_OVERLAY_BLT_BASE	(MSM_PMEM_SF_BASE - MSM_OVERLAY_BLT_SIZE)
#define MSM_PMEM_AUDIO_BASE	    (MSM_OVERLAY_BLT_BASE - MSM_PMEM_AUDIO_SIZE)


#define MSM_PMEM_ADSP_BASE	(0x40400000)
#define MSM_PMEM_ADSP2_BASE	(MSM_PMEM_ADSP_BASE + MSM_PMEM_ADSP_SIZE)

#define MSM_FB_BASE             (0x6B000000)  /*MSM_PMEM_AUDIO_BASE is 0x6BACA000*/
                                              /*to avoid alignment,  use 0x6BA00000 - 0xA00000*/

#define MSM_PMEM_KERNEL_EBI1_BASE	(MSM_PMEM_AUDIO_BASE + MSM_PMEM_AUDIO_SIZE)

#define MSM_SMI_BASE          0x38000000
#define MSM_SMI_SIZE          0x4000000

/* Kernel SMI PMEM Region for video core, used for Firmware */
/* and encoder,decoder scratch buffers */
/* Kernel SMI PMEM Region Should always precede the user space */
/* SMI PMEM Region, as the video core will use offset address */
/* from the Firmware base */
#define KERNEL_SMI_BASE       (MSM_SMI_BASE)
#define KERNEL_SMI_SIZE       0x400000

/* User space SMI PMEM Region for video core*/
/* used for encoder, decoder input & output buffers  */
#define USER_SMI_BASE         (KERNEL_SMI_BASE + KERNEL_SMI_SIZE)
#define USER_SMI_SIZE         (MSM_SMI_SIZE - KERNEL_SMI_SIZE)
#define MSM_PMEM_SMIPOOL_BASE USER_SMI_BASE
#define MSM_PMEM_SMIPOOL_SIZE USER_SMI_SIZE

#define PHY_BASE_ADDR1  0x48000000
#define SIZE_ADDR1      0x23000000

/* GPIO definition */

/* Direct Keys */
#define SHOOTER_U_GPIO_KEY_POWER	(125)
#define SHOOTER_U_GPIO_SW_LCM_3D       (64)
#define SHOOTER_U_GPIO_SW_LCM_2D       (68)
#define SHOOTER_U_GPIO_KEY_VOL_DOWN    (103)
#define SHOOTER_U_GPIO_KEY_VOL_UP      (104)
#define SHOOTER_U_GPIO_KEY_CAM_STEP1   	(123)
#define SHOOTER_U_GPIO_KEY_CAM_STEP2   	(115)

/* Battery */
#define SHOOTER_U_GPIO_MBAT_IN            (61)
#define SHOOTER_U_GPIO_CHG_INT		(126)

/* Wifi */
#define SHOOTER_U_GPIO_WIFI_IRQ              (46)
#define SHOOTER_U_GPIO_WIFI_SHUTDOWN_N       (96)
/* Sensors */
#define SHOOTER_U_SENSOR_I2C_SDA		(72)
#define SHOOTER_U_SENSOR_I2C_SCL		(73)
#define SHOOTER_U_GYRO_INT               (127)
#define SHOOTER_U_ECOMPASS_INT           (128)
#define SHOOTER_U_GSENSOR_INT           (129)

/* Microp */

/* TP */
#define SHOOTER_U_TP_I2C_SDA           (51)
#define SHOOTER_U_TP_I2C_SCL           (52)
#define SHOOTER_U_TP_ATT_N             (57)

/* LCD */
#define GPIO_LCM_ID	50
#define GPIO_LCM_RST_N	66

/* Audio */
#define SHOOTER_U_AUD_CODEC_RST        (67)
#define SHOOTER_U_AUD_CDC_LDO_SEL      (116)

/* BT */
#define SHOOTER_U_GPIO_BT_HOST_WAKE      (45)
#define SHOOTER_U_GPIO_BT_UART1_TX       (53)
#define SHOOTER_U_GPIO_BT_UART1_RX       (54)
#define SHOOTER_U_GPIO_BT_UART1_CTS      (55)
#define SHOOTER_U_GPIO_BT_UART1_RTS      (56)
#define SHOOTER_U_GPIO_BT_SHUTDOWN_N     (100)
#define SHOOTER_U_GPIO_BT_CHIP_WAKE      (130)
#define SHOOTER_U_GPIO_BT_RESET_N        (142)

/* USB */
#define SHOOTER_U_GPIO_USB_ID        (63)
#define SHOOTER_U_GPIO_MHL_RESET        (70)
#define SHOOTER_U_GPIO_MHL_INT        (71)
#define SHOOTER_U_GPIO_MHL_USB_SWITCH        (99)

/* General */
#define SHOOTER_U_GENERAL_I2C_SDA		(59)
#define SHOOTER_U_GENERAL_I2C_SCL		(60)

/* Flashlight */
#define SHOOTER_U_FLASH_EN             (29)
#define SHOOTER_U_TORCH_EN             (30)

/* Accessory */
#define SHOOTER_U_GPIO_AUD_HP_DET        (31)

/* SPI */
#define SHOOTER_U_SPI_DO                 (33)
#define SHOOTER_U_SPI_DI                 (34)
#define SHOOTER_U_SPI_CS                 (35)
#define SHOOTER_U_SPI_CLK                (36)

/* LCM */
#define SHOOTER_U_CTL_3D_1		(131)
#define SHOOTER_U_CTL_3D_2		(132)
#define SHOOTER_U_CTL_3D_3		(133)
#define SHOOTER_U_CTL_3D_4		(134)
#define SHOOTER_U_LCM_3D_PDz		(135)

/* CAMERA SPI */
#define SHOOTER_U_SP3D_SPI_DO                 (41)
#define SHOOTER_U_SP3D_SPI_DI                 (42)
#define SHOOTER_U_SP3D_SPI_CS                 (43)
#define SHOOTER_U_SP3D_SPI_CLK                (44)

/* CAMERA GPIO */
#define SHOOTER_U_CAM_I2C_SDA           (47)
#define SHOOTER_U_CAM_I2C_SCL           (48)
#define SHOOTER_U_SP3D_GATE              (107)
#define SHOOTER_U_SP3D_CORE_GATE         (58)
#define SHOOTER_U_SP3D_SYS_RST           (102)
#define SHOOTER_U_SP3D_PDX               (137)
#define SHOOTER_U_S5K4E1_PD				(137)
#define SHOOTER_U_S5K4E1_INTB				(102)
#define SHOOTER_U_S5K4E1_VCM_PD			(58)
#define SHOOTER_U_SP3D_MCLK           (32)
#define SHOOTER_U_WEBCAM_STB          (140)
#define SHOOTER_U_WEBCAM_RST           (138)
#define SHOOTER_U_CAM_SEL           (141)
#define SHOOTER_U_SP3D_INT	(106)

/* PMIC */

/* PMIC GPIO definition */
#define PMGPIO(x) (x-1)
#define SHOOTER_U_VOL_UP             (104)
#define SHOOTER_U_VOL_DN             (103)
#define SHOOTER_U_AUD_HP_EN          PMGPIO(18)
#define SHOOTER_U_AUD_SPK_ENO         PMGPIO(19)
#define SHOOTER_U_3DLCM_PD           PMGPIO(20)
#define SHOOTER_U_AUD_QTR_RESET      PMGPIO(21)
#define SHOOTER_U_TP_RST             PMGPIO(23)
#define SHOOTER_U_GREEN_LED          PMGPIO(24)
#define SHOOTER_U_AMBER_LED          PMGPIO(25)
#define SHOOTER_U_3DCLK              PMGPIO(26)
#define SHOOTER_U_AUD_MIC_SEL        PMGPIO(14)
#define SHOOTER_U_CHG_STAT	   PMGPIO(33)
#define SHOOTER_U_SDC3_DET           PMGPIO(34)
/*#define SHOOTER_U_PLS_INT            PMGPIO(35)*/
#define SHOOTER_U_AUD_REMO_PRES      PMGPIO(37)
#define SHOOTER_U_WIFI_BT_SLEEP_CLK  PMGPIO(38)
#define SHOOTER_U_PS_VOUT            PMGPIO(22)
#define SHOOTER_U_TORCH_SET1         PMGPIO(40)
#define SHOOTER_U_TORCH_SET2         PMGPIO(31)
#define SHOOTER_U_AUD_REMO_EN        PMGPIO(15)

/* Macros assume PMIC GPIOs start at 0 */
#define PM8058_GPIO_BASE			NR_MSM_GPIOS
#define PM8058_GPIO_PM_TO_SYS(pm_gpio)		(pm_gpio + PM8058_GPIO_BASE)
#define PM8058_GPIO_SYS_TO_PM(sys_gpio)		(sys_gpio - PM8058_GPIO_BASE)
#define PM8058_MPP_BASE			(PM8058_GPIO_BASE + PM8058_GPIOS)
#define PM8058_MPP_PM_TO_SYS(pm_gpio)		(pm_gpio + PM8058_MPP_BASE)
#define PM8058_MPP_SYS_TO_PM(sys_gpio)		(sys_gpio - PM8058_MPP_BASE)

#define PM8901_GPIO_BASE			(PM8058_GPIO_BASE + \
						PM8058_GPIOS + PM8058_MPPS)
#define PM8901_GPIO_PM_TO_SYS(pm_gpio)		(pm_gpio + PM8901_GPIO_BASE)
#define PM8901_GPIO_SYS_TO_PM(sys_gpio)		(sys_gpio - PM901_GPIO_BASE)
#define PM8901_IRQ_BASE				(PM8058_IRQ_BASE + \
						NR_PMIC8058_IRQS)

int __init shooter_u_init_mmc(void);
void __init shooter_u_audio_init(void);
int __init shooter_u_init_keypad(void);
int __init shooter_u_wifi_init(void);
int __init shooter_u_init_panel(struct resource *res, size_t size);

#endif /* __ARCH_ARM_MACH_MSM_BOARD_SHOOTER_U_H */
