/* linux/drivers/media/video/samsung/tv20/s5pv210/hdmi_s5pv210.c
 *
 * hdmi raw ftn  file for Samsung TVOut driver
 *
 * Copyright (c) 2010 Samsung Electronics
 *	         http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/slab.h>

#include <plat/clock.h>
#include <plat/regs-hdmi.h>
#include <mach/map.h>

#include "tv_out_s5pv210.h"
#include "hdmi_param.h"

#include "../hpd.h"

#ifdef S5P_HDMI_DEBUG
#define HDMIPRINTK(fmt, args...) \
	pr_debug("\t\t[HDMI] %s: " fmt, __func__ , ## args)
#else
#define HDMIPRINTK(fmt, args...)
#endif

static struct resource	*g_hdmi_mem;
/*static void __iomem	*g_hdmi_base; */
void __iomem	*g_hdmi_base;

static struct resource	*g_i2c_hdmi_phy_mem;
static void __iomem	*g_i2c_hdmi_phy_base;

static spinlock_t        g_lock_hdmi;

/*
 * i2c_hdmi_phy related
 */

#define PHY_I2C_ADDRESS			0x70

#define I2C_ACK				(1<<7)
#define I2C_INT				(1<<5)
#define I2C_PEND			(1<<4)
#define I2C_INT_CLEAR			(0<<4)
#define I2C_CLK				(0x41)
#define I2C_CLK_PEND_INT		(I2C_CLK|I2C_INT_CLEAR|I2C_INT)
#define I2C_ENABLE			(1<<4)
#define I2C_START			(1<<5)
#define I2C_MODE_MTX			0xC0
#define I2C_MODE_MRX			0x80
#define I2C_IDLE			0

static struct {
	s32    state;
	u8    *buffer;
	s32    bytes;
} i2c_hdmi_phy_context;

#define STATE_IDLE		0
#define STATE_TX_EDDC_SEGADDR	1
#define STATE_TX_EDDC_SEGNUM	2
#define STATE_TX_DDC_ADDR	3
#define STATE_TX_DDC_OFFSET	4
#define STATE_RX_DDC_ADDR	5
#define STATE_RX_DDC_DATA	6
#define STATE_RX_ADDR		7
#define STATE_RX_DATA		8
#define STATE_TX_ADDR		9
#define STATE_TX_DATA		10
#define STATE_TX_STOP		11
#define STATE_RX_STOP		12

static s32 i2c_hdmi_phy_interruptwait(void)
{
	u8 status, reg;
	s32 retval = 0;
	/* time_out 1000 is faster than msleep(1) in while */
	s32 time_out = 1000;

	do {
		status = readb(g_i2c_hdmi_phy_base + I2C_HDMI_CON);

		if (status & I2C_PEND) {
			/* check status flags */
			reg = readb(g_i2c_hdmi_phy_base + I2C_HDMI_STAT);
			break;
		}
		time_out--;
	} while (time_out);

	if (time_out <= 0)
		pr_err("read I2C_HDMI_CON for I2C_PEND fail\n");

	return retval;
}

static s32 i2c_hdmi_phy_read(u8 addr, u8 nbytes, u8 *buffer)
{
	u8 reg;
	s32 ret = 0;
	u32 proc = true;
	s32 time_out = HDMI_TIME_OUT;

	i2c_hdmi_phy_context.state  = STATE_RX_ADDR;
	i2c_hdmi_phy_context.buffer = buffer;
	i2c_hdmi_phy_context.bytes  = nbytes;

	writeb(I2C_CLK | I2C_INT | I2C_ACK,
		g_i2c_hdmi_phy_base + I2C_HDMI_CON);
	writeb(I2C_ENABLE | I2C_MODE_MRX,
		g_i2c_hdmi_phy_base + I2C_HDMI_STAT);
	writeb(addr & 0xFE,
		g_i2c_hdmi_phy_base + I2C_HDMI_DS);

	writeb(I2C_ENABLE | I2C_START | I2C_MODE_MRX,
		g_i2c_hdmi_phy_base + I2C_HDMI_STAT);

	while (proc) {
		if (i2c_hdmi_phy_context.state != STATE_RX_STOP) {
			if (i2c_hdmi_phy_interruptwait() != 0) {
				pr_err("%s::interrupt wait failed!!!\n",
					__func__);
				ret = EINVAL;
				break;
			}
		}

		switch (i2c_hdmi_phy_context.state) {
		case STATE_RX_DATA:
			reg = readb(g_i2c_hdmi_phy_base + I2C_HDMI_DS);
			*(i2c_hdmi_phy_context.buffer) = reg;

			i2c_hdmi_phy_context.buffer++;
			--(i2c_hdmi_phy_context.bytes);

			if (i2c_hdmi_phy_context.bytes == 1) {
				i2c_hdmi_phy_context.state = STATE_RX_STOP;
				writeb(I2C_CLK_PEND_INT,
					g_i2c_hdmi_phy_base + I2C_HDMI_CON);
			} else {
				writeb(I2C_CLK_PEND_INT | I2C_ACK,
					g_i2c_hdmi_phy_base + I2C_HDMI_CON);
			}
			break;
		case STATE_RX_ADDR:
			i2c_hdmi_phy_context.state = STATE_RX_DATA;

			if (i2c_hdmi_phy_context.bytes == 1) {
				i2c_hdmi_phy_context.state = STATE_RX_STOP;
				writeb(I2C_CLK_PEND_INT,
					g_i2c_hdmi_phy_base + I2C_HDMI_CON);
			} else {
				writeb(I2C_CLK_PEND_INT | I2C_ACK,
					g_i2c_hdmi_phy_base + I2C_HDMI_CON);
			}
			break;
		case STATE_RX_STOP:
			i2c_hdmi_phy_context.state = STATE_IDLE;

			reg = readb(g_i2c_hdmi_phy_base + I2C_HDMI_DS);

			*(i2c_hdmi_phy_context.buffer) = reg;

			writeb(I2C_MODE_MRX|I2C_ENABLE,
				g_i2c_hdmi_phy_base + I2C_HDMI_STAT);

			writeb(I2C_CLK_PEND_INT,
				g_i2c_hdmi_phy_base + I2C_HDMI_CON);

			writeb(I2C_MODE_MRX,
				g_i2c_hdmi_phy_base + I2C_HDMI_STAT);

			time_out = HDMI_TIME_OUT;

			while (readb(g_i2c_hdmi_phy_base + I2C_HDMI_STAT) &
				(1<<5) && time_out) {
				msleep(1);
				time_out--;
			}

			if (time_out <= 0) {
				pr_err("read I2C_HDMI_STAT for (1<<5) fail\n");
				ret = EINVAL;
			}

			proc = false;
			break;

		case STATE_IDLE:
		default:
			pr_err("%s::error state!!!\n", __func__);
			proc = false;
			ret = EINVAL;
			break;
		}
	}

	return ret;
}

static s32 i2c_hdmi_phy_write(u8 addr, u8 nbytes, u8 *buffer)
{
	u8 reg;
	s32 ret = 0;
	u32 proc = true;
	s32 time_out = HDMI_TIME_OUT;

	i2c_hdmi_phy_context.state  = STATE_TX_ADDR;
	i2c_hdmi_phy_context.buffer = buffer;
	i2c_hdmi_phy_context.bytes  = nbytes;

	writeb(I2C_CLK | I2C_INT | I2C_ACK,
		g_i2c_hdmi_phy_base + I2C_HDMI_CON);
	writeb(I2C_ENABLE | I2C_MODE_MTX,
		g_i2c_hdmi_phy_base + I2C_HDMI_STAT);
	writeb(addr & 0xFE,
		g_i2c_hdmi_phy_base + I2C_HDMI_DS);
	writeb(I2C_ENABLE | I2C_START | I2C_MODE_MTX,
		g_i2c_hdmi_phy_base + I2C_HDMI_STAT);

	while (proc) {
		if (i2c_hdmi_phy_interruptwait() != 0) {
			pr_err("%s::interrupt wait failed!!!\n",
				__func__);
			ret = EINVAL;
			break;
		}

		switch (i2c_hdmi_phy_context.state) {
		case STATE_TX_ADDR:
		case STATE_TX_DATA:
			i2c_hdmi_phy_context.state = STATE_TX_DATA;

			reg = *(i2c_hdmi_phy_context.buffer);

			writeb(reg, g_i2c_hdmi_phy_base + I2C_HDMI_DS);

			i2c_hdmi_phy_context.buffer++;
			--(i2c_hdmi_phy_context.bytes);

			if (i2c_hdmi_phy_context.bytes == 0) {
				i2c_hdmi_phy_context.state = STATE_TX_STOP;
				writeb(I2C_CLK_PEND_INT,
					g_i2c_hdmi_phy_base + I2C_HDMI_CON);
			} else
				writeb(I2C_CLK_PEND_INT | I2C_ACK,
					g_i2c_hdmi_phy_base + I2C_HDMI_CON);
			break;
		case STATE_TX_STOP:
			i2c_hdmi_phy_context.state = STATE_IDLE;

			writeb(I2C_MODE_MTX | I2C_ENABLE,
				g_i2c_hdmi_phy_base + I2C_HDMI_STAT);

			writeb(I2C_CLK_PEND_INT,
				g_i2c_hdmi_phy_base + I2C_HDMI_CON);

			writeb(I2C_MODE_MTX,
				g_i2c_hdmi_phy_base + I2C_HDMI_STAT);

			time_out = HDMI_TIME_OUT;

			while (readb(g_i2c_hdmi_phy_base + I2C_HDMI_STAT) &
				(1<<5) && time_out) {
				msleep(1);
				time_out--;
			}

			if (time_out <= 0) {
				pr_err("read I2C_HDMI_STAT for (1<<5) fail\n");
				ret = EINVAL;
			}

			proc = false;

			break;
		case STATE_IDLE:
		default:
			pr_err("%s::error state!!!\n", __func__);
			proc = false;
			ret = EINVAL;
			break;
		}
	}

	return ret;
}

static int hdmi_phy_down(bool on, u8 addr, u8 offset, u8 *read_buffer)
{
	u8 buff[2] = {0};

	buff[0] = addr;
	buff[1] = (on) ? (read_buffer[addr] & (~(1<<offset))) :
			(read_buffer[addr] | (1<<offset));

	if (i2c_hdmi_phy_write(PHY_I2C_ADDRESS, 2, buff) != 0)
		return EINVAL;

	return 0;
}

static s32 hdmi_corereset(void)
{
	writeb(0x0, g_hdmi_base + S5P_HDMI_CTRL_CORE_RSTOUT);

	msleep(10);

	writeb(0x1, g_hdmi_base + S5P_HDMI_CTRL_CORE_RSTOUT);

	return 0;
}

static s32 hdmi_phy_config(enum phy_freq freq, enum s5p_hdmi_color_depth cd)
{
	s32 index;
	s32 size;
	u8 buffer[32] = {0, };
	u8 reg;

	switch (cd) {
	case HDMI_CD_24:
		index = 0;
		break;
	case HDMI_CD_30:
		index = 1;
		break;
	case HDMI_CD_36:
		index = 2;
		break;
	default:
		return false;
	}

	/* i2c_hdmi init - set i2c filtering */
	buffer[0] = PHY_REG_MODE_SET_DONE;
	buffer[1] = 0x00;

	if (i2c_hdmi_phy_write(PHY_I2C_ADDRESS, 2, buffer) != 0) {
		pr_err("%s::i2c_hdmi_phy_write failed.\n", __func__);
		return EINVAL;
	}

	writeb(0x5, g_i2c_hdmi_phy_base + I2C_HDMI_LC);

	size = sizeof(phy_config[freq][index])
		/ sizeof(phy_config[freq][index][0]);

	memcpy(buffer, phy_config[freq][index], sizeof(buffer));

	if (i2c_hdmi_phy_write(PHY_I2C_ADDRESS, size, buffer) != 0)
		return EINVAL;

	/* write offset */
	buffer[0] = 0x01;

	if (i2c_hdmi_phy_write(PHY_I2C_ADDRESS, 1, buffer) != 0) {
		pr_err("%s::i2c_hdmi_phy_write failed.\n", __func__);
		return EINVAL;
	}

	hdmi_corereset();

	do {
		reg = readb(g_hdmi_base + HDMI_PHY_STATUS);
	} while (!(reg & HDMI_PHY_READY));

	writeb(I2C_CLK_PEND_INT, g_i2c_hdmi_phy_base + I2C_HDMI_CON);
	/* disable */
	writeb(I2C_IDLE, g_i2c_hdmi_phy_base + I2C_HDMI_STAT);

	return 0;
}

static s32 hdmi_set_tg(enum s5p_hdmi_v_fmt mode)
{
	u16 temp_reg;
	u8 tc_cmd;

	temp_reg = hdmi_tg_param[mode].h_fsz;
	writeb((u8)(temp_reg&0xff) , g_hdmi_base + S5P_TG_H_FSZ_L);
	writeb((u8)(temp_reg >> 8) , g_hdmi_base + S5P_TG_H_FSZ_H);

	/* set Horizontal Active Start Position */
	temp_reg = hdmi_tg_param[mode].hact_st ;
	writeb((u8)(temp_reg&0xff) , g_hdmi_base + S5P_TG_HACT_ST_L);
	writeb((u8)(temp_reg >> 8) , g_hdmi_base + S5P_TG_HACT_ST_H);

	/* set Horizontal Active Size */
	temp_reg = hdmi_tg_param[mode].hact_sz ;
	writeb((u8)(temp_reg&0xff) , g_hdmi_base + S5P_TG_HACT_SZ_L);
	writeb((u8)(temp_reg >> 8) , g_hdmi_base + S5P_TG_HACT_SZ_H);

	/* set Vertical Full Size */
	temp_reg = hdmi_tg_param[mode].v_fsz ;
	writeb((u8)(temp_reg&0xff) , g_hdmi_base + S5P_TG_V_FSZ_L);
	writeb((u8)(temp_reg >> 8) , g_hdmi_base + S5P_TG_V_FSZ_H);

	/* set VSYNC Position */
	temp_reg = hdmi_tg_param[mode].vsync ;
	writeb((u8)(temp_reg&0xff) , g_hdmi_base + S5P_TG_VSYNC_L);
	writeb((u8)(temp_reg >> 8) , g_hdmi_base + S5P_TG_VSYNC_H);

	/* set Bottom Field VSYNC Position */
	temp_reg = hdmi_tg_param[mode].vsync2;
	writeb((u8)(temp_reg&0xff) , g_hdmi_base + S5P_TG_VSYNC2_L);
	writeb((u8)(temp_reg >> 8) , g_hdmi_base + S5P_TG_VSYNC2_H);

	/* set Vertical Active Start Position */
	temp_reg = hdmi_tg_param[mode].vact_st ;
	writeb((u8)(temp_reg&0xff) , g_hdmi_base + S5P_TG_VACT_ST_L);
	writeb((u8)(temp_reg >> 8) , g_hdmi_base + S5P_TG_VACT_ST_H);

	/* set Vertical Active Size */
	temp_reg = hdmi_tg_param[mode].vact_sz ;
	writeb((u8)(temp_reg&0xff) , g_hdmi_base + S5P_TG_VACT_SZ_L);
	writeb((u8)(temp_reg >> 8) , g_hdmi_base + S5P_TG_VACT_SZ_H);

	/* set Field Change Position */
	temp_reg = hdmi_tg_param[mode].field_chg ;
	writeb((u8)(temp_reg&0xff) , g_hdmi_base + S5P_TG_FIELD_CHG_L);
	writeb((u8)(temp_reg >> 8) , g_hdmi_base + S5P_TG_FIELD_CHG_H);

	/* set Bottom Field Vertical Active Start Position */
	temp_reg = hdmi_tg_param[mode].vact_st2;
	writeb((u8)(temp_reg&0xff) , g_hdmi_base + S5P_TG_VACT_ST2_L);
	writeb((u8)(temp_reg >> 8) , g_hdmi_base + S5P_TG_VACT_ST2_H);

	/* set VSYNC Position for HDMI */
	temp_reg = hdmi_tg_param[mode].vsync_top_hdmi;
	writeb((u8)(temp_reg&0xff) , g_hdmi_base + S5P_TG_VSYNC_TOP_HDMI_L);
	writeb((u8)(temp_reg >> 8) , g_hdmi_base + S5P_TG_VSYNC_TOP_HDMI_H);

	/* set Bottom Field VSYNC Position */
	temp_reg = hdmi_tg_param[mode].vsync_bot_hdmi;
	writeb((u8)(temp_reg&0xff) , g_hdmi_base + S5P_TG_VSYNC_BOT_HDMI_L);
	writeb((u8)(temp_reg >> 8) , g_hdmi_base + S5P_TG_VSYNC_BOT_HDMI_H);

	/* set Top Field Change Position for HDMI */
	temp_reg = hdmi_tg_param[mode].field_top_hdmi ;
	writeb((u8)(temp_reg&0xff) , g_hdmi_base + S5P_TG_FIELD_TOP_HDMI_L);
	writeb((u8)(temp_reg >> 8) , g_hdmi_base + S5P_TG_FIELD_TOP_HDMI_H);

	/* set Bottom Field Change Position for HDMI */
	temp_reg = hdmi_tg_param[mode].field_bot_hdmi ;
	writeb((u8)(temp_reg&0xff) , g_hdmi_base + S5P_TG_FIELD_BOT_HDMI_L);
	writeb((u8)(temp_reg >> 8) , g_hdmi_base + S5P_TG_FIELD_BOT_HDMI_H);

	tc_cmd = readb(g_hdmi_base + S5P_TG_CMD);

	if (video_params[mode].interlaced == 1)
		/* Field Mode enable(interlace mode) */
		tc_cmd |= (1<<1);
	else
		/* Field Mode disable */
		tc_cmd &= ~(1<<1);

	writeb(tc_cmd, g_hdmi_base + S5P_TG_CMD);

	return 0;
}

/**
 * Set registers related to color depth.
 */
static s32 hdmi_set_clr_depth(enum s5p_hdmi_color_depth cd)
{
	/* if color depth is supported by RX, set GCP packet */
	switch (cd) {
	case HDMI_CD_48:
		writeb(GCP_CD_48BPP, g_hdmi_base + S5P_GCP_BYTE2);
		break;
	case HDMI_CD_36:
		writeb(GCP_CD_36BPP, g_hdmi_base + S5P_GCP_BYTE2);
		/* set DC register */
		writeb(HDMI_DC_CTL_12, g_hdmi_base + S5P_HDMI_DC_CONTROL);
		break;
	case HDMI_CD_30:
		writeb(GCP_CD_30BPP, g_hdmi_base + S5P_GCP_BYTE2);
		/* set DC register */
		writeb(HDMI_DC_CTL_10, g_hdmi_base + S5P_HDMI_DC_CONTROL);
		break;
	case HDMI_CD_24:
		writeb(GCP_CD_24BPP, g_hdmi_base + S5P_GCP_BYTE2);
		/* set DC register */
		writeb(HDMI_DC_CTL_8, g_hdmi_base + S5P_HDMI_DC_CONTROL);
		/* disable GCP */
		writeb(DO_NOT_TRANSMIT, g_hdmi_base + S5P_GCP_CON);
		break;
	default:
		pr_err("%s::HDMI core does not support \
			requested Deep Color mode\n",
			__func__);
		return -EINVAL;
	}

	return 0;
}

static s32 hdmi_set_video_mode(enum s5p_hdmi_v_fmt mode,
			enum s5p_hdmi_color_depth cd,
			enum s5p_tv_hdmi_pxl_aspect pxl_ratio,
			u8 *avidata)
{
	u8  temp_reg8;
	u16 temp_reg16;
	u32 temp_reg32, temp_sync2, temp_sync3;

	/* check if HDMI code support that mode */
	if (mode > (sizeof(video_params)/sizeof(struct hdmi_v_params)) ||
		(s32)mode < 0) {
		pr_err("%s::This video mode is not Supported\n", __func__);
		return -EINVAL;
	}

	hdmi_set_tg(mode);

	/* set HBlank */
	temp_reg16 = video_params[mode].h_blank;
	writeb((u8)(temp_reg16&0xff), g_hdmi_base + S5P_H_BLANK_0);
	writeb((u8)(temp_reg16 >> 8), g_hdmi_base + S5P_H_BLANK_1);

	/* set VBlank */
	temp_reg32 = video_params[mode].v_blank;
	writeb((u8)(temp_reg32&0xff), g_hdmi_base + S5P_V_BLANK_0);
	writeb((u8)(temp_reg32 >> 8), g_hdmi_base + S5P_V_BLANK_1);
	writeb((u8)(temp_reg32 >> 16), g_hdmi_base + S5P_V_BLANK_2);

	/* set HVLine */
	temp_reg32 = video_params[mode].hvline;
	writeb((u8)(temp_reg32&0xff), g_hdmi_base + S5P_H_V_LINE_0);
	writeb((u8)(temp_reg32 >> 8), g_hdmi_base + S5P_H_V_LINE_1);
	writeb((u8)(temp_reg32 >> 16), g_hdmi_base + S5P_H_V_LINE_2);

	/* set VSYNC polarity */
	writeb(video_params[mode].polarity, g_hdmi_base + S5P_SYNC_MODE);

	/* set HSyncGen */
	temp_reg32 = video_params[mode].h_sync_gen;
	writeb((u8)(temp_reg32&0xff), g_hdmi_base + S5P_H_SYNC_GEN_0);
	writeb((u8)(temp_reg32 >> 8), g_hdmi_base + S5P_H_SYNC_GEN_1);
	writeb((u8)(temp_reg32 >> 16), g_hdmi_base + S5P_H_SYNC_GEN_2);

	/* set VSyncGen1 */
	temp_reg32 = video_params[mode].v_sync_gen;
	writeb((u8)(temp_reg32&0xff), g_hdmi_base + S5P_V_SYNC_GEN_1_0);
	writeb((u8)(temp_reg32 >> 8), g_hdmi_base + S5P_V_SYNC_GEN_1_1);
	writeb((u8)(temp_reg32 >> 16), g_hdmi_base + S5P_V_SYNC_GEN_1_2);

	if (video_params[mode].interlaced) {
		/* set up v_blank_f, v_sync_gen2, v_sync_gen3 */
		temp_reg32 = video_params[mode].v_blank_f;
		temp_sync2 = video_params[mode].v_sync_gen2;
		temp_sync3 = video_params[mode].v_sync_gen3;

		writeb((u8)(temp_reg32 & 0xff), g_hdmi_base + S5P_V_BLANK_F_0);
		writeb((u8)(temp_reg32 >> 8), g_hdmi_base + S5P_V_BLANK_F_1);
		writeb((u8)(temp_reg32 >> 16), g_hdmi_base + S5P_V_BLANK_F_2);

		writeb((u8)(temp_sync2 & 0xff),
			g_hdmi_base + S5P_V_SYNC_GEN_2_0);
		writeb((u8)(temp_sync2 >> 8), g_hdmi_base + S5P_V_SYNC_GEN_2_1);
		writeb((u8)(temp_sync2 >> 16), g_hdmi_base + S5P_V_SYNC_GEN_2_2);

		writeb((u8)(temp_sync3 & 0xff),
			g_hdmi_base + S5P_V_SYNC_GEN_3_0);
		writeb((u8)(temp_sync3 >> 8), g_hdmi_base + S5P_V_SYNC_GEN_3_1);
		writeb((u8)(temp_sync3 >> 16), g_hdmi_base + S5P_V_SYNC_GEN_3_2);
	} else {
		/* progressive mode */
		writeb(0x00, g_hdmi_base + S5P_V_BLANK_F_0);
		writeb(0x00, g_hdmi_base + S5P_V_BLANK_F_1);
		writeb(0x00, g_hdmi_base + S5P_V_BLANK_F_2);

		writeb(0x01, g_hdmi_base + S5P_V_SYNC_GEN_2_0);
		writeb(0x10, g_hdmi_base + S5P_V_SYNC_GEN_2_1);
		writeb(0x00, g_hdmi_base + S5P_V_SYNC_GEN_2_2);

		writeb(0x01, g_hdmi_base + S5P_V_SYNC_GEN_3_0);
		writeb(0x10, g_hdmi_base + S5P_V_SYNC_GEN_3_1);
		writeb(0x00, g_hdmi_base + S5P_V_SYNC_GEN_3_2);
	}

	/* set interlaced mode */
	writeb(video_params[mode].interlaced, g_hdmi_base + S5P_INT_PRO_MODE);

	/* pixel repetition */
	temp_reg8 = readb(g_hdmi_base + S5P_HDMI_CON_1);

	/* clear */
	temp_reg8 &= ~HDMI_CON_PXL_REP_RATIO_MASK;

	if (video_params[mode].repetition) {
		/* set pixel repetition */
		temp_reg8 |= HDMI_DOUBLE_PIXEL_REPETITION;
		/* AVI Packet */
		avidata[4] = AVI_PIXEL_REPETITION_DOUBLE;
	} else { /* clear pixel repetition */
		/* AVI Packet */
		avidata[4] = 0;
	}

	/* set pixel repetition */
	writeb(temp_reg8, g_hdmi_base + S5P_HDMI_CON_1);

	/* set AVI packet VIC */
	if (pxl_ratio == HDMI_PIXEL_RATIO_16_9)
		avidata[3] = video_params[mode].avi_vic_16_9;
	else
		avidata[3] = video_params[mode].avi_vic;

	/* clear */
	temp_reg8 = readb(g_hdmi_base + S5P_AVI_BYTE2) &
			~(AVI_PICTURE_ASPECT_4_3 | AVI_PICTURE_ASPECT_16_9);

	if (pxl_ratio == HDMI_PIXEL_RATIO_16_9)
		temp_reg8 |= AVI_PICTURE_ASPECT_16_9;
	else
		temp_reg8 |= AVI_PICTURE_ASPECT_4_3;

	/* set color depth */
	if (hdmi_set_clr_depth(cd) != 0) {
		pr_err("%s::Can't set hdmi clr. depth.\n", __func__);
		return -EINVAL;
	}

	if (video_params[mode].interlaced == 1) {
		u32 gcp_con;

		gcp_con = readb(g_hdmi_base + S5P_GCP_CON);
		gcp_con |=  (3<<2);

		writeb(gcp_con, g_hdmi_base + S5P_GCP_CON);
	} else {
		u32 gcp_con;

		gcp_con = readb(g_hdmi_base + S5P_GCP_CON);
		gcp_con &= (~(3<<2));

		writeb(gcp_con, g_hdmi_base + S5P_GCP_CON);
	}

#if 0
	/* config Phy */
	if (hdmi_phy_config(video_params[mode].pixel_clock, cd) == EINVAL) {
		pr_err("%s::hdmi_phy_config() failed.\n", __func__);
		return EINVAL;
	}
#endif
	return 0;
}

int s5p_hdmi_phy_power(bool on)
{
	u32 size;
	u8 *buffer;
	u8 read_buffer[0x40] = {0, };

	size = sizeof(phy_config[0][0])
		/ sizeof(phy_config[0][0][0]);

	buffer = (u8 *) phy_config[0][0];

	/* write offset */
	if (i2c_hdmi_phy_write(PHY_I2C_ADDRESS, 1, buffer) != 0)
		return EINVAL;

	/* read data */
	if (i2c_hdmi_phy_read(PHY_I2C_ADDRESS, size, read_buffer) != 0) {
		pr_err("%s::i2c_hdmi_phy_read failed.\n", __func__);
		return EINVAL;
	}

	/* i can't get the information about phy setting */
	if (on) {
		/* biaspd */
		hdmi_phy_down(true, 0x1, 0x5, read_buffer);
		/* clockgenpd */
		hdmi_phy_down(true, 0x1, 0x7, read_buffer);
		/* pllpd */
		hdmi_phy_down(true, 0x5, 0x5, read_buffer);
		/* pcgpd */
		hdmi_phy_down(true, 0x17, 0x0, read_buffer);
		/* txpd */
		hdmi_phy_down(true, 0x17, 0x1, read_buffer);
	} else {
		/* biaspd */
		hdmi_phy_down(false, 0x1, 0x5, read_buffer);
		/* clockgenpd */
		hdmi_phy_down(false, 0x1, 0x7, read_buffer);
		/* pllpd */
		hdmi_phy_down(false, 0x5, 0x5, read_buffer);
		/* pcgpd */
		hdmi_phy_down(false, 0x17, 0x0, read_buffer);
		/* txpd */
		hdmi_phy_down(false, 0x17, 0x1, read_buffer);
	}

	return 0;
}


void s5p_hdmi_set_hpd_onoff(bool on_off)
{
	HDMIPRINTK("hpd is %s\n", on_off ? "on" : "off");

	if (on_off)
		writel(SW_HPD_PLUGGED, g_hdmi_base + S5P_HPD);
	else
		writel(SW_HPD_UNPLUGGED, g_hdmi_base + S5P_HPD);


	HDMIPRINTK("0x%08x\n", readl(g_hdmi_base + S5P_HPD));
}

void s5p_hdmi_audio_set_config(enum s5p_tv_audio_codec_type audio_codec)
{
	u32 data_type = (audio_codec == PCM) ? CONFIG_LINEAR_PCM_TYPE :
			(audio_codec == AC3) ? CONFIG_NON_LINEAR_PCM_TYPE :
				0xff;

	HDMIPRINTK("audio codec type = %s\n",
		(audio_codec&PCM) ? "PCM" :
		(audio_codec&AC3) ? "AC3" :
		(audio_codec&MP3) ? "MP3" :
		(audio_codec&WMA) ? "WMA" : "Unknown");

	/* open SPDIF path on HDMI_I2S */
	writel(0x01, g_hdmi_base + S5P_HDMI_I2S_CLK_CON);
	writel(readl(g_hdmi_base + S5P_HDMI_I2S_MUX_CON) | 0x11,
		g_hdmi_base + S5P_HDMI_I2S_MUX_CON);
	writel(0xFF, g_hdmi_base + S5P_HDMI_I2S_MUX_CH);
	writel(0x03, g_hdmi_base + S5P_HDMI_I2S_MUX_CUV);

	writel(CONFIG_FILTER_2_SAMPLE | data_type
	       | CONFIG_PCPD_MANUAL_SET | CONFIG_WORD_LENGTH_MANUAL_SET
	       | CONFIG_U_V_C_P_REPORT | CONFIG_BURST_SIZE_2
	       | CONFIG_DATA_ALIGN_32BIT
	       , g_hdmi_base + S5P_SPDIFIN_CONFIG_1);
	writel(0, g_hdmi_base + S5P_SPDIFIN_CONFIG_2);
}

void s5p_hdmi_audio_set_acr(u32 sample_rate)
{
	u32 value_n = (sample_rate == 32000) ? 4096 :
		(sample_rate == 44100) ? 6272 :
		(sample_rate == 88200) ? 12544 :
		(sample_rate == 176400) ? 25088 :
		(sample_rate == 48000) ? 6144 :
		(sample_rate == 96000) ? 12288 :
		(sample_rate == 192000) ? 24576 : 0;

	u32 cts = (sample_rate == 32000) ? 27000 :
		(sample_rate == 44100) ? 30000 :
		(sample_rate == 88200) ? 30000 :
		(sample_rate == 176400) ? 30000 :
		(sample_rate == 48000) ? 27000 :
		(sample_rate == 96000) ? 27000 :
		(sample_rate == 192000) ? 27000 : 0;

	HDMIPRINTK("sample rate = %d\n", sample_rate);
	HDMIPRINTK("cts = %d\n", cts);

	writel(value_n & 0xff, g_hdmi_base + S5P_ACR_N0);
	writel((value_n >> 8) & 0xff, g_hdmi_base + S5P_ACR_N1);
	writel((value_n >> 16) & 0xff, g_hdmi_base + S5P_ACR_N2);

	writel(cts & 0xff, g_hdmi_base + S5P_ACR_MCTS0);
	writel((cts >> 8) & 0xff, g_hdmi_base + S5P_ACR_MCTS1);
	writel((cts >> 16) & 0xff, g_hdmi_base + S5P_ACR_MCTS2);

	writel(cts & 0xff, g_hdmi_base + S5P_ACR_CTS0);
	writel((cts >> 8) & 0xff, g_hdmi_base + S5P_ACR_CTS1);
	writel((cts >> 16) & 0xff, g_hdmi_base + S5P_ACR_CTS2);

	writel(4, g_hdmi_base + S5P_ACR_CON);
}

void s5p_hdmi_audio_set_asp(void)
{
	writel(0x0, g_hdmi_base + S5P_ASP_CON);
	/* All Subpackets contain audio samples */
	writel(0x0, g_hdmi_base + S5P_ASP_SP_FLAT);

	writel(1 << 3 | 0, g_hdmi_base + S5P_ASP_CHCFG0);
	writel(1 << 3 | 0, g_hdmi_base + S5P_ASP_CHCFG1);
	writel(1 << 3 | 0, g_hdmi_base + S5P_ASP_CHCFG2);
	writel(1 << 3 | 0, g_hdmi_base + S5P_ASP_CHCFG3);
}

void  s5p_hdmi_audio_clock_enable(void)
{
	writel(0x1, g_hdmi_base + S5P_SPDIFIN_CLK_CTRL);
	/* HDMI operation mode */
	writel(0x3, g_hdmi_base + S5P_SPDIFIN_OP_CTRL);
}

void s5p_hdmi_audio_set_repetition_time(
	enum s5p_tv_audio_codec_type audio_codec,
	u32 bits, u32 frame_size_code)
{
	/* Only 4'b1011  24bit */
	u32 wl = 5 << 1 | 1;
	u32 rpt_cnt = (audio_codec == AC3) ? 1536 * 2 - 1 : 0;

	HDMIPRINTK("repetition count = %d\n", rpt_cnt);

	/* 24bit and manual mode */
	writel(((rpt_cnt&0xf) << 4) | wl,
		g_hdmi_base + S5P_SPDIFIN_USER_VALUE_1);
	/* if PCM this value is 0 */
	writel((rpt_cnt >> 4)&0xff,
		g_hdmi_base + S5P_SPDIFIN_USER_VALUE_2);
	/* if PCM this value is 0 */
	writel(frame_size_code&0xff,
		g_hdmi_base + S5P_SPDIFIN_USER_VALUE_3);
	/* if PCM this value is 0 */
	writel((frame_size_code >> 8)&0xff,
		g_hdmi_base + S5P_SPDIFIN_USER_VALUE_4);
}

void s5p_hdmi_audio_irq_enable(u32 irq_en)
{
	writel(irq_en, g_hdmi_base + S5P_SPDIFIN_IRQ_MASK);
}

void s5p_hdmi_audio_set_aui(enum s5p_tv_audio_codec_type audio_codec,
			u32 sample_rate,
			u32 bits)
{
	u8 sum_of_bits, bytes1, bytes2, bytes3, check_sum;
	u32 bit_rate;
#if 1
	u32 type = 0;
	u32 ch = (audio_codec == PCM) ? 1 : 0;
	u32 sample = 0;
	u32 bps_type = 0;
#else

	/* Ac3 16bit only */
	u32 bps = (audio_codec == PCM) ? bits : 16;

	/* AUI Packet set. */
	u32 type = (audio_codec == PCM) ? 1 : /* PCM */
		(audio_codec == AC3) ? 2 : 0;

	/* AC3 or Refer stream header */
	u32 ch = (audio_codec == PCM) ? 1 : 0;

	/* 2ch or refer to stream header */
	u32 sample = (sample_rate == 32000) ? 1 :
		(sample_rate == 44100) ? 2 :
		(sample_rate == 48000) ? 3 :
		(sample_rate == 88200) ? 4 :
		(sample_rate == 96000) ? 5 :
		(sample_rate == 176400) ? 6 :
		(sample_rate == 192000) ? 7 : 0;

	u32 bps_type = (bps == 16) ? 1 :
			(bps == 20) ? 2 :
			(bps == 24) ? 3 : 0;

#endif
	bps_type = (audio_codec == PCM) ? bps_type : 0;

	sum_of_bits = (0x84 + 0x1 + 10);

	bytes1 = (u8)((type << 4) | ch);

	bytes2 = (u8)((sample << 2) | bps_type);

	bit_rate = 384;

	bytes3 = (audio_codec == PCM) ? (u8)0 : (u8)(bit_rate / 8) ;

	sum_of_bits += (bytes1 + bytes2 + bytes3);
	check_sum = 256 - sum_of_bits;

	/* AUI Packet set. */
	writel(check_sum , g_hdmi_base + S5P_AUI_CHECK_SUM);
	writel(bytes1 , g_hdmi_base + S5P_AUI_BYTE1);
	writel(bytes2 , g_hdmi_base + S5P_AUI_BYTE2);
	writel(bytes3 , g_hdmi_base + S5P_AUI_BYTE3);/* Pcm or stream */
	writel(0x00 , g_hdmi_base + S5P_AUI_BYTE4); /* 2ch pcm or Stream */
	writel(0x00 , g_hdmi_base + S5P_AUI_BYTE5); /* 2ch pcm or Stream */


	writel(HDMI_DO_NOT_TANS, g_hdmi_base + S5P_ACP_CON);
	writel(1 , g_hdmi_base + S5P_ACP_TYPE);

	writel(0x10 , g_hdmi_base + S5P_GCP_BYTE1);
	/*
	 * packet will be transmitted within 384 cycles
	 * after active sync.
	 */
}

void s5p_hdmi_video_set_bluescreen(bool en,
				     u8 cb_b,
				     u8 y_g,
				     u8 cr_r)
{
	HDMIPRINTK("%d, %d, %d, %d\n", en, cb_b, y_g, cr_r);

	if (en) {
		writel(SET_BLUESCREEN_0(cb_b), g_hdmi_base + S5P_BLUE_SCREEN_0);
		writel(SET_BLUESCREEN_1(y_g), g_hdmi_base + S5P_BLUE_SCREEN_1);
		writel(SET_BLUESCREEN_2(cr_r), g_hdmi_base + S5P_BLUE_SCREEN_2);
		writel(readl(g_hdmi_base + S5P_HDMI_CON_0) | BLUE_SCR_EN,
			g_hdmi_base + S5P_HDMI_CON_0);

		HDMIPRINTK("HDMI_BLUE_SCREEN0 = 0x%08x\n",
			readl(g_hdmi_base + S5P_BLUE_SCREEN_0));
		HDMIPRINTK("HDMI_BLUE_SCREEN1 = 0x%08x\n",
			readl(g_hdmi_base + S5P_BLUE_SCREEN_1));
		HDMIPRINTK("HDMI_BLUE_SCREEN2 = 0x%08x\n",
			readl(g_hdmi_base + S5P_BLUE_SCREEN_2));
	} else {
		writel(readl(g_hdmi_base + S5P_HDMI_CON_0)&~BLUE_SCR_EN,
			g_hdmi_base + S5P_HDMI_CON_0);
	}

	HDMIPRINTK("HDMI_CON0 = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_CON_0));
}

enum s5p_tv_hdmi_err s5p_hdmi_init_spd_infoframe(
	enum s5p_hdmi_transmit trans_type,
	u8 *spd_header,
	u8 *spd_data)
{
	u8 header[3] = {0x83, 0x1, 25};
	u8 vendor[8] = {0,};
	u8 product[17] = {0,};
	int checksum = 0;
	int i;

	switch (trans_type) {
	case HDMI_DO_NOT_TANS:
		writel(SPD_TX_CON_NO_TRANS, g_hdmi_base + S5P_SPD_CON);
		break;
	case HDMI_TRANS_ONCE:
		writel(SPD_TX_CON_TRANS_ONCE, g_hdmi_base + S5P_SPD_CON);
		break;
	case HDMI_TRANS_EVERY_SYNC:
		writel(SPD_TX_CON_TRANS_EVERY_VSYNC, g_hdmi_base + S5P_SPD_CON);
		break;
	default:
		pr_err("%s::invalid out_mode parameter(%d)\n",
			__func__, trans_type);
		return S5P_TV_HDMI_ERR_INVALID_PARAM;
		break;
	}

	/*
	 * spd_data, spd_header be specified by Vendor's specific
	 * data. below codes is sample usage
	 */

	if (spd_data == NULL || spd_header == NULL) {
		memcpy(vendor, "SAMSUNG", 7);
		memcpy(product, "S5PC110", 7);
	} else {
		memcpy(header, spd_header, 3);
		memcpy(vendor, spd_data, 8);
		memcpy(product, spd_data + 8, 17);
	}

	checksum = header[0] + header[1] + header[2];

	writel(SET_SPD_HEADER(header[0]), g_hdmi_base + S5P_SPD_HEADER0);
	writel(SET_SPD_HEADER(header[1]), g_hdmi_base + S5P_SPD_HEADER1);
	writel(SET_SPD_HEADER(header[2]), g_hdmi_base + S5P_SPD_HEADER2);

	for (i = 0; i < 8; i++) {
		checksum += vendor[i];
		writeb(SET_SPD_DATA(vendor[i]),
			g_hdmi_base + S5P_SPD_DATA1 + (i * 4));
	}

	product[16] = 0x1;	/* Source Device Information */

	for (i = 0; i < 17; i++) {
		checksum += product[i];
		writeb(SET_SPD_DATA(product[i]),
			g_hdmi_base + S5P_SPD_DATA9 + (i * 4));
	}

	checksum = 0x100 - (checksum & 0xff);

	writeb(SET_SPD_DATA(checksum), g_hdmi_base + S5P_SPD_DATA0);

	HDMIPRINTK("SPD_CON = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_CON));
	HDMIPRINTK("SPD_HEADER0 = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_HEADER0));
	HDMIPRINTK("SPD_HEADER1 = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_HEADER1));
	HDMIPRINTK("SPD_HEADER2 = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_HEADER2));
	HDMIPRINTK("SPD_DATA0  = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA0));
	HDMIPRINTK("SPD_DATA1  = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA1));
	HDMIPRINTK("SPD_DATA2  = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA2));
	HDMIPRINTK("SPD_DATA3  = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA3));
	HDMIPRINTK("SPD_DATA4  = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA4));
	HDMIPRINTK("SPD_DATA5  = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA5));
	HDMIPRINTK("SPD_DATA6  = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA6));
	HDMIPRINTK("SPD_DATA7  = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA7));
	HDMIPRINTK("SPD_DATA8  = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA8));
	HDMIPRINTK("SPD_DATA9  = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA9));
	HDMIPRINTK("SPD_DATA10 = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA10));
	HDMIPRINTK("SPD_DATA11 = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA11));
	HDMIPRINTK("SPD_DATA12 = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA12));
	HDMIPRINTK("SPD_DATA13 = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA13));
	HDMIPRINTK("SPD_DATA14 = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA14));
	HDMIPRINTK("SPD_DATA15 = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA15));
	HDMIPRINTK("SPD_DATA16 = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA16));
	HDMIPRINTK("SPD_DATA17 = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA17));
	HDMIPRINTK("SPD_DATA18 = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA18));
	HDMIPRINTK("SPD_DATA19 = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA19));
	HDMIPRINTK("SPD_DATA20 = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA20));
	HDMIPRINTK("SPD_DATA21 = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA21));
	HDMIPRINTK("SPD_DATA22 = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA22));
	HDMIPRINTK("SPD_DATA23 = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA23));
	HDMIPRINTK("SPD_DATA24 = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA24));
	HDMIPRINTK("SPD_DATA25 = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA25));
	HDMIPRINTK("SPD_DATA26 = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA26));
	HDMIPRINTK("SPD_DATA27 = 0x%08x\n",
		readl(g_hdmi_base + S5P_SPD_DATA27));

	return HDMI_NO_ERROR;
}

void s5p_hdmi_init_hpd_onoff(bool on_off)
{
	HDMIPRINTK("%d\n", on_off);
	s5p_hdmi_set_hpd_onoff(on_off);
	HDMIPRINTK("0x%08x\n", readl(g_hdmi_base + S5P_HPD));
}

#ifdef CONFIG_SND_SOC_SPDIF
#else
static void s5p_hdmi_audio_i2s_config(
	enum s5p_tv_audio_codec_type audio_codec,
	u32 sample_rate,
	u32 bits_per_sample,
	u32 frame_size_code)
{
	u32 data_num, bit_ch, sample_frq;

	if (bits_per_sample == 20) {
		data_num = 2;
		bit_ch   = 1;
	} else if (bits_per_sample == 24) {
		data_num = 3;
		bit_ch   = 1;
	} else {
		data_num = 1;
		bit_ch   = 0;
	}

	writel((readl(g_hdmi_base + S5P_HDMI_I2S_CH_ST_CON) &
		~(1<<0)) | (1<<0),
		g_hdmi_base + S5P_HDMI_I2S_CH_ST_CON);

	writel((readl(g_hdmi_base + S5P_HDMI_I2S_MUX_CON) &
		~(1<<4 | 3<<2 | 1<<1 | 1<<0))
		| (1<<4 | 1<<2 | 1<<1 | 1<<0),
		g_hdmi_base + S5P_HDMI_I2S_MUX_CON);

	writel((readl(g_hdmi_base + S5P_HDMI_I2S_MUX_CH) &
		~(0xff<<0)) | (0x3f<<0),
		g_hdmi_base + S5P_HDMI_I2S_MUX_CH);

	writel((readl(g_hdmi_base + S5P_HDMI_I2S_MUX_CUV) &
		~(0x3<<0)) | (0x3<<0),
		g_hdmi_base + S5P_HDMI_I2S_MUX_CUV);

	sample_frq = (sample_rate == 44100) ? 0 :
			(sample_rate == 48000) ? 2 :
			(sample_rate == 32000) ? 3 :
			(sample_rate == 96000) ? 0xa : 0x0;

	/* readl(g_hdmi_base + S5P_HDMI_YMAX) */
	writel(0x00, g_hdmi_base + S5P_HDMI_I2S_CLK_CON);
	writel(0x01, g_hdmi_base + S5P_HDMI_I2S_CLK_CON);

	writel(readl(g_hdmi_base + S5P_HDMI_I2S_DSD_CON) | 0x01,
		g_hdmi_base + S5P_HDMI_I2S_DSD_CON);

	/* Configuration I2S input ports. Configure I2S_PIN_SEL_0~4 */
	writel((readl(g_hdmi_base + S5P_HDMI_I2S_PIN_SEL_0) &
		~(7<<4 | 7<<0)) | (5<<4|6<<0),
		g_hdmi_base + S5P_HDMI_I2S_PIN_SEL_0);
	writel((readl(g_hdmi_base + S5P_HDMI_I2S_PIN_SEL_1) &
		~(7<<4 | 7<<0)) | (1<<4|4<<0),
		g_hdmi_base + S5P_HDMI_I2S_PIN_SEL_1);
	writel((readl(g_hdmi_base + S5P_HDMI_I2S_PIN_SEL_2) &
		~(7<<4 | 7<<0)) | (1<<4|2<<0),
		g_hdmi_base + S5P_HDMI_I2S_PIN_SEL_2);
	writel((readl(g_hdmi_base + S5P_HDMI_I2S_PIN_SEL_3) &
		~(7<<0)) | (0<<0),
		g_hdmi_base + S5P_HDMI_I2S_PIN_SEL_3);

	/* I2S_CON_1 & 2 */
	writel((readl(g_hdmi_base + S5P_HDMI_I2S_CON_1) &
		~(1<<1 | 1<<0)) | (1<<1|0<<0),
		g_hdmi_base + S5P_HDMI_I2S_CON_1);
	writel((readl(g_hdmi_base + S5P_HDMI_I2S_CON_2) &
		~(1<<6 | 3<<4 | 3<<2 | 3<<0))
		| (0<<6 | bit_ch<<4 | data_num<<2 | 0<<0),
		g_hdmi_base + S5P_HDMI_I2S_CON_2);

	/* Configure register related to CUV information */
	writel((readl(g_hdmi_base + S5P_HDMI_I2S_CH_ST_0) &
		~(3<<6 | 7<<3 | 1<<2 | 1<<1 | 1<<0))
		| (0<<6 | 0<<3 | 0<<2 | 0<<1 | 1<<0),
		g_hdmi_base + S5P_HDMI_I2S_CH_ST_0);
	writel((readl(g_hdmi_base + S5P_HDMI_I2S_CH_ST_1) &
		~(0xff<<0)) | (0<<0),
		g_hdmi_base + S5P_HDMI_I2S_CH_ST_1);
	writel((readl(g_hdmi_base + S5P_HDMI_I2S_CH_ST_2) &
		~(0xff<<0)) | (0<<0),
		g_hdmi_base + S5P_HDMI_I2S_CH_ST_2);
	writel((readl(g_hdmi_base + S5P_HDMI_I2S_CH_ST_3) &
		~(3<<4 | 0xf<<0))
			| (0<<4 | sample_frq<<0),
		g_hdmi_base + S5P_HDMI_I2S_CH_ST_3);
	writel((readl(g_hdmi_base + S5P_HDMI_I2S_CH_ST_4) &
		~(0xf<<4 | 7<<1 | 1<<0))
			| (0xf<<4 | 5<<1 | 1<<0),
		g_hdmi_base + S5P_HDMI_I2S_CH_ST_4);

	writel(0x00, g_hdmi_base + S5P_HDMI_I2S_CH_ST_SH_0);
	writel(0x00, g_hdmi_base + S5P_HDMI_I2S_CH_ST_SH_1);
	writel(0x00, g_hdmi_base + S5P_HDMI_I2S_CH_ST_SH_2);
	writel(0x00, g_hdmi_base + S5P_HDMI_I2S_CH_ST_SH_3);
	writel(0x00, g_hdmi_base + S5P_HDMI_I2S_CH_ST_SH_4);
/*
	writel((readl(g_hdmi_base + S5P_HDMI_I2S_CH_ST_CON) &
		~(1<<0)) | (1<<0),
		g_hdmi_base + S5P_HDMI_I2S_CH_ST_CON);

	writel((readl(g_hdmi_base + S5P_HDMI_I2S_MUX_CON) &
		~(1<<4 | 3<<2 | 1<<1 | 1<<0))
		| (1<<4 | 1<<2 | 1<<1 | 1<<0),
		g_hdmi_base + S5P_HDMI_I2S_MUX_CON);

	writel((readl(g_hdmi_base + S5P_HDMI_I2S_MUX_CH) &
		~(0xff<<0)) | (0x3f<<0),
		g_hdmi_base + S5P_HDMI_I2S_MUX_CH);

	writel((readl(g_hdmi_base + S5P_HDMI_I2S_MUX_CUV) &
		~(0x3<<0)) | (0x3<<0),
		g_hdmi_base + S5P_HDMI_I2S_MUX_CUV);
*/

/*
	writel(0x00, g_hdmi_base + S5P_ASP_SP_FLAT);
	writel(0x08, g_hdmi_base + S5P_ASP_CHCFG0);
	writel(0x1a, g_hdmi_base + S5P_ASP_CHCFG1);
	writel(0x2c, g_hdmi_base + S5P_ASP_CHCFG2);
	writel(0x3e, g_hdmi_base + S5P_ASP_CHCFG3);

	writel((readl(g_hdmi_base + S5P_ASP_CON) &
		~(1<<7 | 3<<5 | 1<<4 | 0xf<<0))
			| (0<<7 | 0<<5 | 1<<4 | 0x7<<0),
		g_hdmi_base + S5P_ASP_CON);

	writel((readl(g_hdmi_base + S5P_ACR_CON) &
		~(3<<3 | 7<<0)) | (0<<3 | 4<<0),
		g_hdmi_base + S5P_ACR_CON);

	writel((readl(g_hdmi_base + S5P_HDMI_CON_0) &
		~(1<<7 | 1<<6 | 1<<2 |1<<0))
		| (1<<7 | 1<<6 | 1<<2 | 1<<0),
		g_hdmi_base + S5P_HDMI_CON_0);
*/
}
#endif

enum s5p_tv_hdmi_err s5p_hdmi_audio_init(
	enum s5p_tv_audio_codec_type audio_codec,
	u32 sample_rate, u32 bits, u32 frame_size_code)
{
#ifdef CONFIG_SND_SOC_SPDIF
	s5p_hdmi_audio_set_config(audio_codec);
	s5p_hdmi_audio_set_repetition_time(audio_codec, bits,
		frame_size_code);
	s5p_hdmi_audio_irq_enable(IRQ_BUFFER_OVERFLOW_ENABLE);
	s5p_hdmi_audio_clock_enable();

/*
	s5p_hdmi_audio_set_asp();
	s5p_hdmi_audio_set_acr(sample_rate);
*/

#else
	s5p_hdmi_audio_i2s_config(audio_codec, sample_rate, bits,
		frame_size_code);
#endif
	s5p_hdmi_audio_set_asp();
	s5p_hdmi_audio_set_acr(sample_rate);

	s5p_hdmi_audio_set_aui(audio_codec, sample_rate, bits);

	return HDMI_NO_ERROR;
}

enum s5p_tv_hdmi_err s5p_hdmi_video_init_display_mode(
	enum s5p_tv_disp_mode disp_mode,
	enum s5p_tv_o_mode out_mode, u8 *avidata)
{
	enum s5p_hdmi_v_fmt hdmi_v_fmt;
	enum s5p_tv_hdmi_pxl_aspect aspect;

	HDMIPRINTK("disp mode %d, output mode%d\n", disp_mode, out_mode);

	aspect = HDMI_PIXEL_RATIO_16_9;

	switch (disp_mode) {
	/* 480p */
	case TVOUT_480P_60_16_9:
		hdmi_v_fmt = v720x480p_60Hz;
		break;
	case TVOUT_480P_60_4_3:
		hdmi_v_fmt = v720x480p_60Hz;
		aspect = HDMI_PIXEL_RATIO_4_3;
		break;
	case TVOUT_480P_59:
		hdmi_v_fmt = v720x480p_59Hz;
		break;
	/* 576p */
	case TVOUT_576P_50_16_9:
		hdmi_v_fmt = v720x576p_50Hz;
		break;
	case TVOUT_576P_50_4_3:
		hdmi_v_fmt = v720x576p_50Hz;
		aspect = HDMI_PIXEL_RATIO_4_3;
		break;
	/* 720p */
	case TVOUT_720P_60:
		hdmi_v_fmt = v1280x720p_60Hz;
		break;
	case TVOUT_720P_59:
		hdmi_v_fmt = v1280x720p_59Hz;
		break;
	case TVOUT_720P_50:
		hdmi_v_fmt = v1280x720p_50Hz;
		break;
	/* 1080p */
	case TVOUT_1080P_30:
		hdmi_v_fmt = v1920x1080p_30Hz;
		break;
	case TVOUT_1080P_60:
		hdmi_v_fmt = v1920x1080p_60Hz;
		break;
	case TVOUT_1080P_59:
		hdmi_v_fmt = v1920x1080p_59Hz;
		break;
	case TVOUT_1080P_50:
		hdmi_v_fmt = v1920x1080p_50Hz;
		break;
	/* 1080i */
	case TVOUT_1080I_60:
		hdmi_v_fmt = v1920x1080i_60Hz;
		break;
	case TVOUT_1080I_59:
		hdmi_v_fmt = v1920x1080i_59Hz;
		break;
	case TVOUT_1080I_50:
		hdmi_v_fmt = v1920x1080i_50Hz;
		break;
	default:
		pr_err("%s::invalid disp_mode parameter(%d)\n",
			__func__, disp_mode);
		return S5P_TV_HDMI_ERR_INVALID_PARAM;
		break;
	}

	if (hdmi_phy_config(video_params[hdmi_v_fmt].pixel_clock, HDMI_CD_24)
		== EINVAL) {
		pr_err("%s::hdmi_phy_config() failed.\n", __func__);
		return EINVAL;
	}

	switch (out_mode) {
	case TVOUT_OUTPUT_HDMI_RGB:
		s5p_hdmi_set_dvi(false);
		writel(PX_LMT_CTRL_RGB, g_hdmi_base + S5P_HDMI_CON_1);
		writel(VID_PREAMBLE_EN | GUARD_BAND_EN,
			g_hdmi_base + S5P_HDMI_CON_2);
		writel(HDMI_MODE_EN | DVI_MODE_DIS,
			g_hdmi_base + S5P_MODE_SEL);

		/* there's no ACP packet api */
		writel(HDMI_DO_NOT_TANS , g_hdmi_base + S5P_ACP_CON);
		writel(HDMI_TRANS_EVERY_SYNC , g_hdmi_base + S5P_AUI_CON);
		break;
	case TVOUT_OUTPUT_HDMI:
		s5p_hdmi_set_dvi(false);
		writel(PX_LMT_CTRL_BYPASS, g_hdmi_base + S5P_HDMI_CON_1);
		writel(VID_PREAMBLE_EN | GUARD_BAND_EN,
			g_hdmi_base + S5P_HDMI_CON_2);
		writel(HDMI_MODE_EN | DVI_MODE_DIS,
			g_hdmi_base + S5P_MODE_SEL);

		/* there's no ACP packet api */
		writel(HDMI_DO_NOT_TANS , g_hdmi_base + S5P_ACP_CON);
		writel(HDMI_TRANS_EVERY_SYNC , g_hdmi_base + S5P_AUI_CON);
		break;
	case TVOUT_OUTPUT_DVI:
		s5p_hdmi_set_dvi(true);
		writel(PX_LMT_CTRL_RGB, g_hdmi_base + S5P_HDMI_CON_1);
		writel(VID_PREAMBLE_DIS | GUARD_BAND_DIS,
			g_hdmi_base + S5P_HDMI_CON_2);
		writel(HDMI_MODE_DIS | DVI_MODE_EN,
			g_hdmi_base + S5P_MODE_SEL);

		/* disable ACP & Audio Info.frame packet */
		writel(HDMI_DO_NOT_TANS , g_hdmi_base + S5P_ACP_CON);
		writel(HDMI_DO_NOT_TANS , g_hdmi_base + S5P_AUI_CON);
		break;
	default:
		pr_err("%s::invalid out_mode parameter(%d)\n",
			__func__, out_mode);
		return S5P_TV_HDMI_ERR_INVALID_PARAM;
		break;
	}

	hdmi_set_video_mode(hdmi_v_fmt, HDMI_CD_24, aspect, avidata);

	return HDMI_NO_ERROR;
}

void s5p_hdmi_video_init_bluescreen(bool en,
				    u8 cb_b,
				    u8 y_g,
				    u8 cr_r)
{
	s5p_hdmi_video_set_bluescreen(en, cb_b, y_g, cr_r);

}

void s5p_hdmi_video_init_color_range(u8 y_min,
				     u8 y_max,
				     u8 c_min,
				     u8 c_max)
{
	HDMIPRINTK("%d, %d, %d, %d\n", y_max, y_min, c_max, c_min);

	writel(y_max, g_hdmi_base + S5P_HDMI_YMAX);
	writel(y_min, g_hdmi_base + S5P_HDMI_YMIN);
	writel(c_max, g_hdmi_base + S5P_HDMI_CMAX);
	writel(c_min, g_hdmi_base + S5P_HDMI_CMIN);

	HDMIPRINTK("HDMI_YMAX = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_YMAX));
	HDMIPRINTK("HDMI_YMIN = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_YMIN));
	HDMIPRINTK("HDMI_CMAX = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_CMAX));
	HDMIPRINTK("HDMI_CMIN = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_CMIN));
}

enum s5p_tv_hdmi_err s5p_hdmi_video_init_csc(
	enum s5p_tv_hdmi_csc_type csc_type)
{
	unsigned short us_csc_coeff[10];

	HDMIPRINTK("%d)\n", csc_type);

	switch (csc_type) {
	case HDMI_CSC_YUV601_TO_RGB_LR:
		us_csc_coeff[0] = 0x23;
		us_csc_coeff[1] = 256;
		us_csc_coeff[2] = 938;
		us_csc_coeff[3] = 846;
		us_csc_coeff[4] = 256;
		us_csc_coeff[5] = 443;
		us_csc_coeff[6] = 0;
		us_csc_coeff[7] = 256;
		us_csc_coeff[8] = 0;
		us_csc_coeff[9] = 350;
		break;
	case HDMI_CSC_YUV601_TO_RGB_FR:
		us_csc_coeff[0] = 0x03;
		us_csc_coeff[1] = 298;
		us_csc_coeff[2] = 924;
		us_csc_coeff[3] = 816;
		us_csc_coeff[4] = 298;
		us_csc_coeff[5] = 516;
		us_csc_coeff[6] = 0;
		us_csc_coeff[7] = 298;
		us_csc_coeff[8] = 0;
		us_csc_coeff[9] = 408;
		break;
	case HDMI_CSC_YUV709_TO_RGB_LR:
		us_csc_coeff[0] = 0x23;
		us_csc_coeff[1] = 256;
		us_csc_coeff[2] = 978;
		us_csc_coeff[3] = 907;
		us_csc_coeff[4] = 256;
		us_csc_coeff[5] = 464;
		us_csc_coeff[6] = 0;
		us_csc_coeff[7] = 256;
		us_csc_coeff[8] = 0;
		us_csc_coeff[9] = 394;
		break;
	case HDMI_CSC_YUV709_TO_RGB_FR:
		us_csc_coeff[0] = 0x03;
		us_csc_coeff[1] = 298;
		us_csc_coeff[2] = 970;
		us_csc_coeff[3] = 888;
		us_csc_coeff[4] = 298;
		us_csc_coeff[5] = 540;
		us_csc_coeff[6] = 0;
		us_csc_coeff[7] = 298;
		us_csc_coeff[8] = 0;
		us_csc_coeff[9] = 458;
		break;
	case HDMI_CSC_YUV601_TO_YUV709:
		us_csc_coeff[0] = 0x33;
		us_csc_coeff[1] = 256;
		us_csc_coeff[2] = 995;
		us_csc_coeff[3] = 971;
		us_csc_coeff[4] = 0;
		us_csc_coeff[5] = 260;
		us_csc_coeff[6] = 29;
		us_csc_coeff[7] = 0;
		us_csc_coeff[8] = 19;
		us_csc_coeff[9] = 262;
		break;
	case HDMI_CSC_RGB_FR_TO_RGB_LR:
		us_csc_coeff[0] = 0x20;
		us_csc_coeff[1] = 220;
		us_csc_coeff[2] = 0;
		us_csc_coeff[3] = 0;
		us_csc_coeff[4] = 0;
		us_csc_coeff[5] = 220;
		us_csc_coeff[6] = 0;
		us_csc_coeff[7] = 0;
		us_csc_coeff[8] = 0;
		us_csc_coeff[9] = 220;
		break;
	case HDMI_CSC_RGB_FR_TO_YUV601:
		us_csc_coeff[0] = 0x30;
		us_csc_coeff[1] = 129;
		us_csc_coeff[2] = 25;
		us_csc_coeff[3] = 65;
		us_csc_coeff[4] = 950;
		us_csc_coeff[5] = 112;
		us_csc_coeff[6] = 986;
		us_csc_coeff[7] = 930;
		us_csc_coeff[8] = 1006;
		us_csc_coeff[9] = 112;
		break;
	case HDMI_CSC_RGB_FR_TO_YUV709:
		us_csc_coeff[0] = 0x30;
		us_csc_coeff[1] = 157;
		us_csc_coeff[2] = 16;
		us_csc_coeff[3] = 47;
		us_csc_coeff[4] = 937;
		us_csc_coeff[5] = 112;
		us_csc_coeff[6] = 999;
		us_csc_coeff[7] = 922;
		us_csc_coeff[8] = 1014;
		us_csc_coeff[9] = 112;
		break;
	case HDMI_BYPASS:
		us_csc_coeff[0] = 0x33;
		us_csc_coeff[1] = 256;
		us_csc_coeff[2] = 0;
		us_csc_coeff[3] = 0;
		us_csc_coeff[4] = 0;
		us_csc_coeff[5] = 256;
		us_csc_coeff[6] = 0;
		us_csc_coeff[7] = 0;
		us_csc_coeff[8] = 0;
		us_csc_coeff[9] = 256;
		break;
	default:
		pr_err("%s::invalid out_mode parameter(%d)\n",
			__func__, csc_type);
		return S5P_TV_HDMI_ERR_INVALID_PARAM;
		break;
	}

/*	writel(us_csc_coeff[0], g_hdmi_base + S5P_HDMI_CSC_CON);;

	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[1]),
		g_hdmi_base + S5P_HDMI_Y_G_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[1]),
		g_hdmi_base + S5P_HDMI_Y_G_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[2]),
		g_hdmi_base + S5P_HDMI_Y_B_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[2]),
		g_hdmi_base + S5P_HDMI_Y_B_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[3]),
		g_hdmi_base + S5P_HDMI_Y_R_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[3]),
		g_hdmi_base + S5P_HDMI_Y_R_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[4]),
		g_hdmi_base + S5P_HDMI_CB_G_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[4]),
		g_hdmi_base + S5P_HDMI_CB_G_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[5]),
		g_hdmi_base + S5P_HDMI_CB_B_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[5]),
		g_hdmi_base + S5P_HDMI_CB_B_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[6]),
		g_hdmi_base + S5P_HDMI_CB_R_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[6]),
		g_hdmi_base + S5P_HDMI_CB_R_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[7]),
		g_hdmi_base + S5P_HDMI_CR_G_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[7]),
		g_hdmi_base + S5P_HDMI_CR_G_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[8]),
		g_hdmi_base + S5P_HDMI_CR_B_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[8]),
		g_hdmi_base + S5P_HDMI_CR_B_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[9]),
		g_hdmi_base + S5P_HDMI_CR_R_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[9]),
		g_hdmi_base + S5P_HDMI_CR_R_COEF_H);

	HDMIPRINTK("HDMI_CSC_CON = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_CSC_CON));
	HDMIPRINTK("HDMI_Y_G_COEF_L = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_Y_G_COEF_L));
	HDMIPRINTK("HDMI_Y_G_COEF_H = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_Y_G_COEF_H));
	HDMIPRINTK("HDMI_Y_B_COEF_L = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_Y_B_COEF_L));
	HDMIPRINTK("HDMI_Y_B_COEF_H = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_Y_B_COEF_H));
	HDMIPRINTK("HDMI_Y_R_COEF_L = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_Y_R_COEF_L));
	HDMIPRINTK("HDMI_Y_R_COEF_H = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_Y_R_COEF_H));
	HDMIPRINTK("HDMI_CB_G_COEF_L = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_CB_G_COEF_L));
	HDMIPRINTK("HDMI_CB_G_COEF_H = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_CB_G_COEF_H));
	HDMIPRINTK("HDMI_CB_B_COEF_L = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_CB_B_COEF_L));
	HDMIPRINTK("HDMI_CB_B_COEF_H = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_CB_B_COEF_H));
	HDMIPRINTK("HDMI_CB_R_COEF_L = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_CB_R_COEF_L));
	HDMIPRINTK("HDMI_CB_R_COEF_H = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_CB_R_COEF_H));
	HDMIPRINTK("HDMI_CR_G_COEF_L = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_CR_G_COEF_L));
	HDMIPRINTK("HDMI_CR_G_COEF_H = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_CR_G_COEF_H));
	HDMIPRINTK("HDMI_CR_B_COEF_L = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_CR_B_COEF_L));
	HDMIPRINTK("HDMI_CR_B_COEF_H = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_CR_B_COEF_H));
	HDMIPRINTK("HDMI_CR_R_COEF_L = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_CR_R_COEF_L));
	HDMIPRINTK("HDMI_CR_R_COEF_H = 0x%08x\n",
		readl(g_hdmi_base + S5P_HDMI_CR_R_COEF_H));
*/
	return HDMI_NO_ERROR;
}

enum s5p_tv_hdmi_err s5p_hdmi_video_init_avi_infoframe(
	enum s5p_hdmi_transmit trans_type, u8 check_sum, u8 *avi_data)
{
	HDMIPRINTK("%d, %d, %d\n", (u32)trans_type, (u32)check_sum,
		(u32)avi_data);

	switch (trans_type) {
	case HDMI_DO_NOT_TANS:
		writel(AVI_TX_CON_NO_TRANS, g_hdmi_base + S5P_AVI_CON);
		break;
	case HDMI_TRANS_ONCE:
		writel(AVI_TX_CON_TRANS_ONCE, g_hdmi_base + S5P_AVI_CON);
		break;
	case HDMI_TRANS_EVERY_SYNC:
		writel(AVI_TX_CON_TRANS_EVERY_VSYNC, g_hdmi_base + S5P_AVI_CON);
		break;
	default:
		pr_err("%s::invalid out_mode parameter(%d)\n",
			__func__, trans_type);
		return S5P_TV_HDMI_ERR_INVALID_PARAM;
		break;
	}

	writel(SET_AVI_CHECK_SUM(check_sum), g_hdmi_base + S5P_AVI_CHECK_SUM);

	writel(SET_AVI_BYTE(*(avi_data)), g_hdmi_base + S5P_AVI_BYTE1);
	writel(SET_AVI_BYTE(*(avi_data + 1)), g_hdmi_base + S5P_AVI_BYTE2);
	writel(SET_AVI_BYTE(*(avi_data + 2)), g_hdmi_base + S5P_AVI_BYTE3);
	writel(SET_AVI_BYTE(*(avi_data + 3)), g_hdmi_base + S5P_AVI_BYTE4);
	writel(SET_AVI_BYTE(*(avi_data + 4)), g_hdmi_base + S5P_AVI_BYTE5);
	writel(SET_AVI_BYTE(*(avi_data + 5)), g_hdmi_base + S5P_AVI_BYTE6);
	writel(SET_AVI_BYTE(*(avi_data + 6)), g_hdmi_base + S5P_AVI_BYTE7);
	writel(SET_AVI_BYTE(*(avi_data + 7)), g_hdmi_base + S5P_AVI_BYTE8);
	writel(SET_AVI_BYTE(*(avi_data + 8)), g_hdmi_base + S5P_AVI_BYTE9);
	writel(SET_AVI_BYTE(*(avi_data + 9)), g_hdmi_base + S5P_AVI_BYTE10);
	writel(SET_AVI_BYTE(*(avi_data + 10)), g_hdmi_base + S5P_AVI_BYTE11);
	writel(SET_AVI_BYTE(*(avi_data + 11)), g_hdmi_base + S5P_AVI_BYTE12);
	writel(SET_AVI_BYTE(*(avi_data + 12)), g_hdmi_base + S5P_AVI_BYTE13);

	HDMIPRINTK("AVI_CON = 0x%08x\n", readl(g_hdmi_base + S5P_AVI_CON));
	HDMIPRINTK("AVI_CHECK_SUM = 0x%08x\n",
		readl(g_hdmi_base + S5P_AVI_CHECK_SUM));
	HDMIPRINTK("AVI_BYTE1  = 0x%08x\n",
		readl(g_hdmi_base + S5P_AVI_BYTE1));
	HDMIPRINTK("AVI_BYTE2  = 0x%08x\n",
		readl(g_hdmi_base + S5P_AVI_BYTE2));
	HDMIPRINTK("AVI_BYTE3  = 0x%08x\n",
		readl(g_hdmi_base + S5P_AVI_BYTE3));
	HDMIPRINTK("AVI_BYTE4  = 0x%08x\n",
		readl(g_hdmi_base + S5P_AVI_BYTE4));
	HDMIPRINTK("AVI_BYTE5  = 0x%08x\n",
		readl(g_hdmi_base + S5P_AVI_BYTE5));
	HDMIPRINTK("AVI_BYTE6  = 0x%08x\n",
		readl(g_hdmi_base + S5P_AVI_BYTE6));
	HDMIPRINTK("AVI_BYTE7  = 0x%08x\n",
		readl(g_hdmi_base + S5P_AVI_BYTE7));
	HDMIPRINTK("AVI_BYTE8  = 0x%08x\n",
		readl(g_hdmi_base + S5P_AVI_BYTE8));
	HDMIPRINTK("AVI_BYTE9  = 0x%08x\n",
		readl(g_hdmi_base + S5P_AVI_BYTE9));
	HDMIPRINTK("AVI_BYTE10 = 0x%08x\n",
		readl(g_hdmi_base + S5P_AVI_BYTE10));
	HDMIPRINTK("AVI_BYTE11 = 0x%08x\n",
		readl(g_hdmi_base + S5P_AVI_BYTE11));
	HDMIPRINTK("AVI_BYTE12 = 0x%08x\n",
		readl(g_hdmi_base + S5P_AVI_BYTE12));
	HDMIPRINTK("AVI_BYTE13 = 0x%08x\n",
		readl(g_hdmi_base + S5P_AVI_BYTE13));

	return HDMI_NO_ERROR;
}

enum s5p_tv_hdmi_err s5p_hdmi_video_init_mpg_infoframe(
	enum s5p_hdmi_transmit trans_type, u8 check_sum, u8 *mpg_data)
{
	HDMIPRINTK("trans_type : %d, %d, %d\n", (u32)trans_type,
			(u32)check_sum, (u32)mpg_data);

	switch (trans_type) {
	case HDMI_DO_NOT_TANS:
		writel(MPG_TX_CON_NO_TRANS,
		       g_hdmi_base + S5P_MPG_CON);
		break;
	case HDMI_TRANS_ONCE:
		writel(MPG_TX_CON_TRANS_ONCE,
		       g_hdmi_base + S5P_MPG_CON);
		break;
	case HDMI_TRANS_EVERY_SYNC:
		writel(MPG_TX_CON_TRANS_EVERY_VSYNC,
		       g_hdmi_base + S5P_MPG_CON);
		break;
	default:
		pr_err("%s::invalid out_mode parameter(%d)\n",
			__func__, trans_type);
		return S5P_TV_HDMI_ERR_INVALID_PARAM;
		break;
	}

	writel(SET_MPG_CHECK_SUM(check_sum),
	       g_hdmi_base + S5P_MPG_CHECK_SUM);

	writel(SET_MPG_BYTE(*(mpg_data)),
	       g_hdmi_base + S5P_MPEG_BYTE1);
	writel(SET_MPG_BYTE(*(mpg_data + 1)),
	       g_hdmi_base + S5P_MPEG_BYTE2);
	writel(SET_MPG_BYTE(*(mpg_data + 2)),
	       g_hdmi_base + S5P_MPEG_BYTE3);
	writel(SET_MPG_BYTE(*(mpg_data + 3)),
	       g_hdmi_base + S5P_MPEG_BYTE4);
	writel(SET_MPG_BYTE(*(mpg_data + 4)),
	       g_hdmi_base + S5P_MPEG_BYTE5);

	HDMIPRINTK("MPG_CON = 0x%08x\n",
		   readl(g_hdmi_base + S5P_MPG_CON));
	HDMIPRINTK("MPG_CHECK_SUM = 0x%08x\n",
		   readl(g_hdmi_base + S5P_MPG_CHECK_SUM));
	HDMIPRINTK("MPEG_BYTE1 = 0x%08x\n",
		   readl(g_hdmi_base + S5P_MPEG_BYTE1));
	HDMIPRINTK("MPEG_BYTE2 = 0x%08x\n",
		   readl(g_hdmi_base + S5P_MPEG_BYTE2));
	HDMIPRINTK("MPEG_BYTE3 = 0x%08x\n",
		   readl(g_hdmi_base + S5P_MPEG_BYTE3));
	HDMIPRINTK("MPEG_BYTE4 = 0x%08x\n",
		   readl(g_hdmi_base + S5P_MPEG_BYTE4));
	HDMIPRINTK("MPEG_BYTE5 = 0x%08x\n",
		   readl(g_hdmi_base + S5P_MPEG_BYTE5));

	return HDMI_NO_ERROR;
}

void s5p_hdmi_video_init_tg_cmd(bool time_c_e,
				bool bt656_sync_en,
				bool tg_en)
{
	u32 temp_reg = 0;

	temp_reg = readl(g_hdmi_base + S5P_TG_CMD);

	if (time_c_e)
		temp_reg |= GETSYNC_TYPE_EN;
	else
		temp_reg &= GETSYNC_TYPE_DIS;

	if (bt656_sync_en)
		temp_reg |= GETSYNC_EN;
	else
		temp_reg &= GETSYNC_DIS;

	if (tg_en)
		temp_reg |= TG_EN;
	else
		temp_reg &= TG_DIS;

	writel(temp_reg, g_hdmi_base + S5P_TG_CMD);

	HDMIPRINTK("TG_CMD = 0x%08x\n", readl(g_hdmi_base + S5P_TG_CMD));
}


bool s5p_hdmi_start(enum s5p_hdmi_audio_type hdmi_audio_type,
				bool hdcp_en,
				struct i2c_client *ddc_port)
{
	u32 temp_reg = HDMI_EN;

	HDMIPRINTK("aud type : %d, hdcp enable : %d\n",
		   hdmi_audio_type, hdcp_en);

	switch (hdmi_audio_type) {
	case HDMI_AUDIO_PCM:
		temp_reg |= ASP_EN;
		break;
	case HDMI_AUDIO_NO:
		break;
	default:
		pr_err("%s::invalid hdmi_audio_type(%d)\n",
			__func__, hdmi_audio_type);
		return false;
		break;
	}

	s5p_hpd_set_hdmiint();

	if (hdcp_en) {
		writel(HDCP_ENC_DISABLE, g_hdmi_base + S5P_ENC_EN);
		s5p_hdmi_mute_en(true);
	}

	writel(readl(g_hdmi_base + S5P_HDMI_CON_0) | temp_reg,
	       g_hdmi_base + S5P_HDMI_CON_0);

#if 0
	if (hdcp_en) {


		if (!s5p_start_hdcp())
			pr_err("%s::HDCP start failed\n", __func__);

	}

#endif
	HDMIPRINTK("HPD : 0x%08x, HDMI_CON_0 : 0x%08x\n",
		   readl(g_hdmi_base + S5P_HPD),
		   readl(g_hdmi_base + S5P_HDMI_CON_0));

	return true;
}

/*
* stop  - stop functions are only called under running HDMI
*/
void s5p_hdmi_stop(void)
{
	u32 temp = 0, result = 0;
	HDMIPRINTK("\n");

	s5p_stop_hdcp();

/* TODO: Check Manual
	writel(readl(g_hdmi_base + S5P_HDMI_CON_0) &
	       ~(PWDN_ENB_NORMAL | HDMI_EN | ASP_EN),
	       ~(HDMI_EN | ASP_EN),
	       g_hdmi_base + S5P_HDMI_CON_0);

*/
	/*
	 * Before stopping hdmi, stop the hdcp first. However,
	 * if there's no delay between hdcp stop & hdmi stop,
	 * re-opening would be failed.
	 */
	msleep(100);

	temp = readl(g_hdmi_base + S5P_HDMI_CON_0);
	result = temp & HDMI_EN;

	if (result)
		writel(temp &  ~(HDMI_EN | ASP_EN),
			g_hdmi_base + S5P_HDMI_CON_0);

	do {
		temp = readl(g_hdmi_base + S5P_HDMI_CON_0);
	} while (temp & HDMI_EN);

	s5p_hpd_set_eint();

	HDMIPRINTK("HPD 0x%08x, HDMI_CON_0 0x%08x\n",
		   readl(g_hdmi_base + S5P_HPD),
		   readl(g_hdmi_base + S5P_HDMI_CON_0));
}

int __init s5p_hdmi_probe(struct platform_device *pdev,
	u32 res_num, u32 res_num2)
{
	struct resource *res;
	size_t	size;
	u32	reg;

	spin_lock_init(&g_lock_hdmi);

	res = platform_get_resource(pdev, IORESOURCE_MEM, res_num);
	if (res == NULL) {
		dev_err(&pdev->dev,
			"failed to get memory region resource\n");
		goto error;
	}
	size = (res->end - res->start) + 1;

	g_hdmi_mem = request_mem_region(res->start, size, pdev->name);
	if (g_hdmi_mem == NULL) {
		dev_err(&pdev->dev,
			"failed to get memory region\n");
		goto error;
	}
	g_hdmi_base = ioremap(res->start, size);
	if (g_hdmi_base == NULL) {
		dev_err(&pdev->dev,
			"failed to ioremap address region\n");
		goto error;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, res_num2);
	if (res == NULL) {
		dev_err(&pdev->dev,
			"failed to get memory region resource\n");
		goto error;
	}

	size = (res->end - res->start) + 1;

	g_i2c_hdmi_phy_mem = request_mem_region(res->start, size, pdev->name);
	if (g_i2c_hdmi_phy_mem == NULL) {
		dev_err(&pdev->dev,
			"failed to get memory region\n");
		goto error;
	}

	g_i2c_hdmi_phy_base = ioremap(res->start, size);
	if (g_i2c_hdmi_phy_base == NULL) {
		dev_err(&pdev->dev,
			"failed to ioremap address region\n");
		goto error;
	}

	/* PMU Block : HDMI PHY Enable */
	reg = readl(S3C_VA_SYS+0xE804);
	reg |= (1<<0);
	writel(reg, S3C_VA_SYS+0xE804);

	/* i2c_hdmi init - set i2c filtering */
	writeb(0x5, g_i2c_hdmi_phy_base + I2C_HDMI_LC);

	/* temp for test - hdmi intr. global enable */
	reg = readb(g_hdmi_base+S5P_HDMI_CTRL_INTC_CON);
	writeb(reg | (1<<HDMI_IRQ_GLOBAL), g_hdmi_base+S5P_HDMI_CTRL_INTC_CON);

	return 0;
error:
	return -ENOENT;

}

int __init s5p_hdmi_release(struct platform_device *pdev)
{
	if (g_hdmi_base) {
		iounmap(g_hdmi_base);
		g_hdmi_base = NULL;
	}

	if (g_hdmi_mem) {
		if (release_resource(g_hdmi_mem))
			dev_err(&pdev->dev,
				"Can't remove tvout drv !!\n");

		kfree(g_hdmi_mem);
		g_hdmi_mem = NULL;
	}

	return 0;
}

/*
 * HDCP ISR.
 * If HDCP IRQ occurs, set hdcp_event and wake up the waitqueue.
 */

#define HDMI_IRQ_TOTAL_NUM	6

hdmi_isr g_hdmi_isr_ftn[HDMI_IRQ_TOTAL_NUM];

int s5p_hdmi_register_isr(hdmi_isr isr, u8 irq_num)
{
	HDMIPRINTK("Try to register ISR for IRQ number (%d)\n", irq_num);

	if (isr == NULL) {
		pr_err("%s::Invaild ISR\n", __func__);
		return -EINVAL;
	}

	/* check IRQ number */
	if (irq_num > HDMI_IRQ_TOTAL_NUM) {
		pr_err("%s::irq_num exceeds allowed IRQ number(%d)\n",
			__func__, HDMI_IRQ_TOTAL_NUM);
		return -EINVAL;
	}

	/* check if is the number already registered? */
	if (g_hdmi_isr_ftn[irq_num]) {
		HDMIPRINTK("the %d th ISR is already registered\n",
			irq_num);
	}

	g_hdmi_isr_ftn[irq_num] = isr;

	HDMIPRINTK("Success to register ISR for IRQ number (%d)\n",
			irq_num);

	return 0;
}

irqreturn_t s5p_hdmi_irq(int irq, void *dev_id)
{
	u8 irq_state, irq_num;

	spin_lock_irq(&g_lock_hdmi);

	irq_state = readb(g_hdmi_base+S5P_HDMI_CTRL_INTC_FLAG);

	HDMIPRINTK("S5P_HDMI_CTRL_INTC_FLAG = 0x%02x\n", irq_state);

	/* Check interrupt happened */
	/* Priority of Interrupt  HDCP> I2C > Audio > CEC (Not implemented) */

	if (irq_state) {
		/* HDCP IRQ*/
		irq_num = 0;
		/* check if ISR is null or not */
		while (irq_num < HDMI_IRQ_TOTAL_NUM) {
			if (irq_state & (1 << irq_num)) {
				if (g_hdmi_isr_ftn[irq_num] != NULL)
					(g_hdmi_isr_ftn[irq_num])(irq_num);
				else
					HDMIPRINTK(
					"No registered ISR for IRQ[%d]\n",
					irq_num);

			}
			++irq_num;
		}

	} else {
		HDMIPRINTK("Undefined IRQ happened[%x]\n", irq_state);
	}

	spin_unlock_irq(&g_lock_hdmi);

	return IRQ_HANDLED;
}
u8 s5p_hdmi_get_enabled_interrupt(void)
{
	u8 reg;
	reg = readb(g_hdmi_base+S5P_HDMI_CTRL_INTC_CON);
	return reg;
}

void s5p_hdmi_enable_interrupts(enum s5p_tv_hdmi_interrrupt intr)
{
	u8 reg;
	reg = readb(g_hdmi_base+S5P_HDMI_CTRL_INTC_CON);
	writeb(reg | (1<<intr) | (1<<HDMI_IRQ_GLOBAL),
		g_hdmi_base+S5P_HDMI_CTRL_INTC_CON);
}

void s5p_hdmi_disable_interrupts(enum s5p_tv_hdmi_interrrupt intr)
{
	u8 reg;
	reg = readb(g_hdmi_base+S5P_HDMI_CTRL_INTC_CON);
	writeb(reg & ~(1<<intr), g_hdmi_base+S5P_HDMI_CTRL_INTC_CON);
}

void s5p_hdmi_clear_pending(enum s5p_tv_hdmi_interrrupt intr)
{
	u8 reg;
	reg = readb(g_hdmi_base+S5P_HDMI_CTRL_INTC_FLAG);
	writeb(reg | (1<<intr), g_hdmi_base+S5P_HDMI_CTRL_INTC_FLAG);
}

u8 s5p_hdmi_get_interrupts(void)
{
	u8 reg;
	reg = readb(g_hdmi_base+S5P_HDMI_CTRL_INTC_FLAG);
	return reg;
}

u8 s5p_hdmi_get_swhpd_status(void)
{
	u8 reg;
	reg = readb(g_hdmi_base+S5P_HPD) & HPD_SW_ENABLE;
	return reg;
}

u8 s5p_hdmi_get_hpd_status(void)
{
	u8 reg;
	reg = readb(g_hdmi_base+S5P_HDMI_CTRL_HPD);
	return reg;
}

void s5p_hdmi_swhpd_disable(void)
{
	writeb(HPD_SW_DISABLE, g_hdmi_base+S5P_HPD);
}

void s5p_hdmi_hpd_gen(void)
{
	writeb(0xFF, g_hdmi_base+S5P_HDMI_HPD_GEN);
}

