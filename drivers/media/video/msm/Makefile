GCC_VERSION      := $(shell $(CONFIG_SHELL) $(PWD)/scripts/gcc-version.sh $(CROSS_COMPILE)gcc)
ifeq ($(GCC_VERSION),0404)
CFLAGS_REMOVE_msm_vfe8x.o = -Wframe-larger-than=1024
endif

ifeq ($(CONFIG_MSM_CAMERA_V4L2),y)
  EXTRA_CFLAGS += -Idrivers/media/video/msm/csi
  EXTRA_CFLAGS += -Idrivers/media/video/msm/io
  EXTRA_CFLAGS += -Idrivers/media/video/msm/sensors
  obj-$(CONFIG_MSM_CAMERA) += msm_isp.o msm.o msm_mem.o msm_mctl.o msm_mctl_buf.o msm_mctl_pp.o
  obj-$(CONFIG_MSM_CAMERA) += rawchip-v4l2/ io/ sensors/ actuators/ csi/
else
ifeq ($(CONFIG_ARCH_MSM8X60),y)
  obj-$(CONFIG_MSM_CAMERA) += msm_camera-8x60.o sensors/

  ifeq ($(CONFIG_CAMERA_3D),y)
    obj-$(CONFIG_MSM_CAMERA) += msm_camera_liteon.o sensors/
  endif

else
ifeq ($(CONFIG_ARCH_MSM7X30),y)
  obj-$(CONFIG_MSM_CAMERA) += msm_camera-7x30.o sensors/ rawchip/
else
  obj-$(CONFIG_MSM_CAMERA) += msm_camera.o
endif
endif
endif
obj-$(CONFIG_MSM_CAMERA) += msm_axi_qos.o

ifeq ($(CONFIG_MSM_CAMERA_V4L2),y)
	obj-$(CONFIG_MSM_CAMERA) += gemini/
	obj-$(CONFIG_MSM_CAMERA_FLASH) += flash_v4l2.o
else
	obj-$(CONFIG_MSM_CAMERA) += gemini_8x60/
ifeq ($(CONFIG_ARCH_MSM7X27A),y)
	obj-$(CONFIG_MSM_CAMERA_FLASH) += flash.o
else
	obj-$(CONFIG_MSM_CAMERA_FLASH) += flash_8x60.o
endif
endif
obj-$(CONFIG_ARCH_MSM_ARM11) += msm_vfe7x.o msm_io7x.o
obj-$(CONFIG_ARCH_MSM7X27A) += msm_vfe7x27a.o msm_io_7x27a.o
obj-$(CONFIG_ARCH_MSM7X30) += msm_vfe_7x30.o msm_io_7x30.o msm_vpe1_7x30.o
obj-$(CONFIG_ARCH_QSD8X50) += msm_vfe8x.o msm_vfe8x_proc.o msm_io8x.o

ifeq ($(CONFIG_MSM_CAMERA_V4L2),y)
  obj-$(CONFIG_ARCH_MSM8X60) += msm_io_8x60_v4l2.o msm_vfe31_v4l2.o msm_vpe_8x60_v4l2.o
else
ifdef CONFIG_CAMERA_ZSL
	obj-$(CONFIG_ARCH_MSM8X60) += msm_io_8x60.o msm_vfe_8x60_ZSL.o msm_vpe1_8x60.o
else
	ifdef CONFIG_CAMERA_3D
		obj-$(CONFIG_ARCH_MSM8X60) += msm_io_8x60.o msm_vfe_8x60.o msm_vpe1_8x60.o
		obj-$(CONFIG_ARCH_MSM8X60) += msm_vfe31_liteon.o msm_vpe1_liteon.o
	else
		obj-$(CONFIG_ARCH_MSM8X60) += msm_io_8x60.o msm_vfe_8x60.o msm_vpe1_8x60.o
	endif
endif
endif

obj-$(CONFIG_ARCH_MSM8960) += msm_io_8960.o msm_ispif.o msm_vfe32.o msm_vpe.o
