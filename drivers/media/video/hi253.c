/* linux/drivers/media/video/hi253.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * 		http://www.samsung.com/
 *
 * Driver for hi253 (SXGA camera) from Samsung Electronics
 * 1/5" 2Mp CMOS Image Sensor SoC with an Embedded Image Processor
 * supporting MIPI CSI-2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-i2c-drv.h>
#include <media/hi253_platform.h>

#ifdef CONFIG_VIDEO_SAMSUNG_V4L2
#include <linux/videodev2_samsung.h>
#endif

#include "hi253.h"

#define HI253_DRIVER_NAME	"hi253"

/* Default resolution & pixelformat. plz ref hi253_platform.h */
#define DEFAULT_RES		SVGA	/* Index of resoultion */
#define DEFAUT_FPS_INDEX	HI253_15FPS
#define DEFAULT_FMT		V4L2_PIX_FMT_UYVY	/* YUV422 */

/*
 * Specification
 * Parallel : ITU-R. 656/601 YUV422, RGB565, RGB888 (Up to VGA), RAW10
 * Serial : MIPI CSI2 (single lane) YUV422, RGB565, RGB888 (Up to VGA), RAW10
 * Resolution : 1280 (H) x 1024 (V)
 * Image control : Brightness, Contrast, Saturation, Sharpness, Glamour
 * Effect : Mono, Negative, Sepia, Aqua, Sketch
 * FPS : 15fps @full resolution, 30fps @VGA, 24fps @720p
 * Max. pixel clock frequency : 48MHz(upto)
 * Internal PLL (6MHz to 27MHz input frequency)
*/

/* Camera functional setting values configured by user concept */
struct hi253_userset {
	signed int exposure_bias;	/* V4L2_CID_EXPOSURE */
	unsigned int ae_lock;
	unsigned int awb_lock;
	unsigned int auto_wb;	/* V4L2_CID_CAMERA_WHITE_BALANCE */
	unsigned int manual_wb;	/* V4L2_CID_WHITE_BALANCE_PRESET */
	unsigned int wb_temp;	/* V4L2_CID_WHITE_BALANCE_TEMPERATURE */
	unsigned int effect;	/* Color FX (AKA Color tone) */
	unsigned int contrast;	/* V4L2_CID_CAMERA_CONTRAST */
	unsigned int saturation;	/* V4L2_CID_CAMERA_SATURATION */
	unsigned int sharpness;		/* V4L2_CID_CAMERA_SHARPNESS */
	unsigned int glamour;
	unsigned int banding;
};

struct hi253_state {
	struct hi253_platform_data *pdata;
	struct v4l2_subdev sd;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
	struct hi253_userset userset;
	int freq;	/* MCLK in KHz */
	int is_mipi;
	int isize;
	int ver;
	int fps;

	int mode;
	int mode_index;
	int initialized;
};

static inline struct hi253_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct hi253_state, sd);
}

/*
 * hi253 register structure : 2bytes address, 2bytes value
 * retry on write failure up-to 5 times
 */
static inline int hi253_write(struct v4l2_subdev *sd, u16 addr, u16 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg[1];
	unsigned char reg[4];
	int err = 0;
	int retry = 0;


	if (!client->adapter)
		return -ENODEV;

again:
	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 2;
	msg->buf = reg;

	reg[1] = addr & 0xff;
	reg[3] = val & 0xff;

	err = i2c_transfer(client->adapter, msg, 1);
	if (err >= 0)
		return err;	/* Returns here on success */

	/* abnormal case: retry 5 times */
	if (retry < 5) {
		dev_err(&client->dev, "%s: address: 0x%02x, " "value: 0x%02x\n", __func__, reg[0], reg[1]);
		retry++;
		goto again;
	}

	return err;
}

static int hi253_i2c_write(struct v4l2_subdev *sd, unsigned char i2c_data[],
				unsigned char length)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[length], i;
	struct i2c_msg msg = {client->addr, 0, length, buf};

	for (i = 0; i < length; i++)
		buf[i] = i2c_data[i];

	return i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
}

static inline int hi253_write_regs(struct v4l2_subdev *sd, const struct hi253_reg regs[])
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int i, err = 0;

	for (i = 0; i < REGSET_MAX; i++) {
		if (regs[i].addr == REGSET_END)
		{
			break;
		}

		err = hi253_write(sd, regs[i].addr, regs[i].val);
		if (err < 0)
		{
			v4l_info(client, "%s: register set failed\n", __func__);
			break;
		}
	}

	return err;
}

static const char *hi253_querymenu_wb_preset[] = {
	"WB sunny",
	"WB cloudy",
	"WB Tungsten",
	"WB Fluorescent",
	NULL
};

static const char *hi253_querymenu_effect_mode[] = {
        "Effect Sepia", "Effect Aqua", "Effect Monochrome",
        "Effect Negative", "Effect Sketch", NULL
};

static const char *hi253_querymenu_ev_bias_mode[] = {
	"-3EV",	"-2,1/2EV", "-2EV", "-1,1/2EV",
	"-1EV", "-1/2EV", "0", "1/2EV",
	"1EV", "1,1/2EV", "2EV", "2,1/2EV",
	"3EV", NULL
};

static struct v4l2_queryctrl hi253_controls[] = {
	{
		/*
		 * For now, we just support in preset type
		 * to be close to generic WB system,
		 * we define color temp range for each preset
		 */
		.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "White balance temperature",
		.minimum = 0,
		.maximum = 10000,
		.step = 1,
		.default_value = 0,	/* FIXME */
		.flags = V4L2_CTRL_FLAG_DISABLED,
	}, 
	{
		.id = V4L2_CID_WHITE_BALANCE_PRESET,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "White balance preset",
		.minimum = 0,
		.maximum = ARRAY_SIZE(hi253_querymenu_wb_preset) - 2,
		.step = 1,
		.default_value = 0,
		.flags = V4L2_CTRL_FLAG_DISABLED,
	},
	{
		.id = V4L2_CID_CAMERA_WHITE_BALANCE,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Auto white balance",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
		.flags = V4L2_CTRL_FLAG_DISABLED,
	},
	{ 
		.id = V4L2_CID_EXPOSURE,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Exposure bias",
		.minimum = 0,
		.maximum = ARRAY_SIZE(hi253_querymenu_ev_bias_mode) - 2,
		.step = 1,
		.default_value = \
		(ARRAY_SIZE(hi253_querymenu_ev_bias_mode) - 2) / 2,/* 0 EV */
		.flags = V4L2_CTRL_FLAG_DISABLED,
	},
	{
		.id = V4L2_CID_CAMERA_EFFECT,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Image Effect",
		.minimum = 0,
		.maximum = ARRAY_SIZE(hi253_querymenu_effect_mode) - 2,
		.step = 1,
		.default_value = 0,
		.flags = V4L2_CTRL_FLAG_DISABLED,
	},
	{
		.id = V4L2_CID_CAMERA_CONTRAST,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Contrast",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
		.flags = V4L2_CTRL_FLAG_DISABLED,
	},
	{
		.id = V4L2_CID_CAMERA_SATURATION,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Saturation",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
		.flags = V4L2_CTRL_FLAG_DISABLED,
	},
	{
		.id = V4L2_CID_CAMERA_SHARPNESS,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Sharpness",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
		.flags = V4L2_CTRL_FLAG_DISABLED,
	},
	{
                .id = V4L2_CID_CAMERA_ANTI_BANDING,
                .type = V4L2_CTRL_TYPE_INTEGER,
                .name = "Camera Antibanding",
                .minimum = 0,
                .maximum = 3,
                .step = 1,
                .default_value = 1,
                .flags = V4L2_CTRL_FLAG_DISABLED,
        },
};

static const char **hi253_ctrl_get_menu(u32 id)
{
	switch (id) {
	case V4L2_CID_WHITE_BALANCE_PRESET:
		return hi253_querymenu_wb_preset;

	case V4L2_CID_CAMERA_EFFECT:
		return hi253_querymenu_effect_mode;

	case V4L2_CID_EXPOSURE:
	case V4L2_CID_BRIGHTNESS:
		return hi253_querymenu_ev_bias_mode;

	default:
		return v4l2_ctrl_get_menu(id);
	}
}

static inline struct v4l2_queryctrl const *hi253_find_qctrl(int id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(hi253_controls); i++)
		if (hi253_controls[i].id == id)
			return &hi253_controls[i];

	return NULL;
}

static int hi253_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	int i;

	//printk("%s (%d)\n", __func__, qc->id);
	for (i = 0; i < ARRAY_SIZE(hi253_controls); i++) {
		if (hi253_controls[i].id == qc->id) {
			memcpy(qc, &hi253_controls[i], \
				sizeof(struct v4l2_queryctrl));
			return 0;
		}
	}

	return -EINVAL;
}

static int hi253_querymenu(struct v4l2_subdev *sd, struct v4l2_querymenu *qm)
{
	struct v4l2_queryctrl qctrl;

	qctrl.id = qm->id;
	hi253_queryctrl(sd, &qctrl);

	return v4l2_ctrl_query_menu(qm, &qctrl, hi253_ctrl_get_menu(qm->id));
}

/*
 * Clock configuration
 * Configure expected MCLK from host and return EINVAL if not supported clock
 * frequency is expected
 * 	freq : in Hz
 * 	flag : not supported for now
 */
static int hi253_s_crystal_freq(struct v4l2_subdev *sd, u32  freq, u32 flags)
{
	int err = -EINVAL;

	return err;
}

static int hi253_g_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;

	return err;
}

static int hi253_goto_mode(struct v4l2_subdev *sd, int camera_mode, int mode_index)
{
	struct hi253_state *state = to_state(sd);

	if (camera_mode == state->mode
		&& mode_index == state->mode_index)
	{
		return 0;
	}

	switch (camera_mode)
	{
	case CAMERA_PREVIEW:
		hi253_write_regs(sd, hi253_regs_preview[mode_index]);
		break;

	case CAMERA_CAPTURE:
		hi253_write_regs(sd, hi253_regs_capture[mode_index]);
		break;

	case CAMERA_RECORD:
		hi253_write_regs(sd, hi253_regs_record[mode_index]);
		break;
	}

	return 0;
}

static int hi253_set_framesize(struct v4l2_subdev *sd, const struct hi253_framesize *frmsize, int frmsize_count, int camera_mode)
{
	const struct hi253_framesize *last_frmsize = &frmsize[frmsize_count - 1];
	struct hi253_state *state = to_state(sd);
	
	do {
		if (frmsize->width >= state->pix.width && frmsize->height >= state->pix.height)
		{
			break;
		}
		frmsize++;
	} while (frmsize <= last_frmsize);
	
	if (frmsize > last_frmsize)
		frmsize = last_frmsize;


	state->pix.width = frmsize->width;
	state->pix.height = frmsize->height;


	hi253_goto_mode(sd, camera_mode, frmsize->index);

	return 0;
}

static int hi253_s_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;

	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct hi253_state *state = to_state(sd);

	dev_info(&client->dev, "hi253_s_fmt\n");

	state->pix.width = fmt->fmt.pix.width;
	state->pix.height = fmt->fmt.pix.height;
	state->pix.pixelformat = fmt->fmt.pix.pixelformat;

	if (!state->initialized)
	{
		return 0;
	}

	switch (fmt->fmt.pix.priv)
	{
	case CAMERA_PREVIEW:
		hi253_set_framesize(sd, hi253_preview_framesize_list, ARRAY_SIZE(hi253_preview_framesize_list), CAMERA_PREVIEW);
		break;

        case CAMERA_CAPTURE:
		hi253_set_framesize(sd, hi253_capture_framesize_list, ARRAY_SIZE(hi253_capture_framesize_list), CAMERA_CAPTURE);
		break;

        case CAMERA_RECORD:
		hi253_set_framesize(sd, hi253_record_framesize_list, ARRAY_SIZE(hi253_record_framesize_list), CAMERA_RECORD);
		break;
	}

	return err;
}

static int hi253_enum_framesizes(struct v4l2_subdev *sd,
					struct v4l2_frmsizeenum *fsize)
{
	int err = 0;
	struct hi253_state *state = to_state(sd);

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = state->pix.width;
	fsize->discrete.height = state->pix.height;
	
	return err;
}

static int hi253_enum_frameintervals(struct v4l2_subdev *sd,
					struct v4l2_frmivalenum *fival)
{
	int err = 0;

	return err;
}

static int hi253_enum_fmt(struct v4l2_subdev *sd,
				struct v4l2_fmtdesc *fmtdesc)
{
	int err = 0;

	return err;
}

static int hi253_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;

	return err;
}

static int hi253_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	int err = 0;

	return err;
}

static int hi253_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	int err = 0;

	return err;
}

static int hi253_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	//struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct hi253_state *state = to_state(sd);
	struct hi253_userset userset = state->userset;
	int err = -EINVAL;

	if (!state->initialized)
	{
		return 0;
	}

	printk("%s:0x%x \n", __func__, ctrl->id);
	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_CAMERA_BRIGHTNESS:
		ctrl->value = userset.exposure_bias;
		err = 0;
		break;
	case V4L2_CID_CAMERA_WHITE_BALANCE:
		ctrl->value = userset.auto_wb;
		err = 0;
		break;
	case V4L2_CID_WHITE_BALANCE_PRESET:
		ctrl->value = userset.manual_wb;
		err = 0;
		break;
	case V4L2_CID_CAMERA_EFFECT:
		ctrl->value = userset.effect;
		err = 0;
		break;
	case V4L2_CID_CAMERA_CONTRAST:
		ctrl->value = userset.contrast;
		err = 0;
		break;
	case V4L2_CID_CAMERA_SATURATION:
		ctrl->value = userset.saturation;
		err = 0;
		break;
	case V4L2_CID_CAMERA_SHARPNESS:
		ctrl->value = userset.sharpness;
		err = 0;
		break;

	case V4L2_CID_CAMERA_ANTI_BANDING:
		ctrl->value = userset.banding;
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

static int hi253_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct hi253_state *state = to_state(sd);
	int err = -EINVAL;
	
	if (!state->initialized)
	{
		return 0;
	}

	printk("%s(0x%x)\n", __func__, ctrl->id);
	
	switch (ctrl->id) {
		case V4L2_CID_CAMERA_WHITE_BALANCE:
			dev_dbg(&client->dev, "%s: V4L2_CID_CAMERA_WHITE_BALANCE\n", __func__);
			err = hi253_write_regs(sd, hi253_regs_wb_preset[ctrl->value]);
			break;

		case V4L2_CID_CAMERA_EFFECT:
			dev_dbg(&client->dev, "%s: V4L2_CID_CAMERA_EFFECT\n", __func__);
			err = hi253_write_regs(sd, hi253_regs_color_effect[ctrl->value]);
			break;
                
		case V4L2_CID_EXPOSURE:
		case V4L2_CID_CAMERA_BRIGHTNESS:
			dev_dbg(&client->dev, "%s: V4L2_CID_EXPOSURE\n", __func__);
			err = hi253_write_regs(sd, hi253_regs_ev_bias[ctrl->value]);
			break;
		
		case V4L2_CID_CAMERA_CONTRAST:
			dev_dbg(&client->dev, "%s: V4L2_CID_CAMERA_CONTRAST\n", __func__);
			err = hi253_write_regs(sd, hi253_regs_contrast_bias[ctrl->value]);
			break;
	
		case V4L2_CID_CAMERA_SATURATION:
			dev_dbg(&client->dev, "%s: V4L2_CID_CAMERA_SATURATION\n", __func__);
			err = hi253_write_regs(sd, hi253_regs_saturation_bias[ctrl->value]);
			break;
							
		case V4L2_CID_CAMERA_SHARPNESS:
			dev_dbg(&client->dev, "%s: V4L2_CID_CAMERA_SHARPNESS\n", __func__);
			err = hi253_write_regs(sd, hi253_regs_sharpness_bias[ctrl->value]);
			break;
		
		case V4L2_CID_CAMERA_ANTI_BANDING:
			err = hi253_write_regs(sd, hi253_regs_anti_banding[ctrl->value]);
			break;
			
		case V4L2_CID_CAMERA_CLOSE:
			state->initialized = 0;
			break;

		default:
			return 0;
	}

	if (err < 0)
		goto out;
	else
		return 0;

out:
	dev_dbg(&client->dev, "%s: vidioc_s_ctrl failed\n", __func__);
	return err;
}

static int hi253_init(struct v4l2_subdev *sd, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct hi253_state *state = to_state(sd);
	int err = -EINVAL, i;

	v4l_info(client, "%s: camera initialization start\n", __func__);
	
	for (i = 0; i < HI253_INIT_REGS; i++)
	{
		err = hi253_i2c_write(sd, hi253_init_reg[i],sizeof(hi253_init_reg[i]));
					
		if (err < 0)
			v4l_info(client, "%s: register set failed\n", __func__);
	}

	state->mode = CAMERA_PREVIEW;
	state->initialized = 1;

	hi253_set_framesize(sd, hi253_preview_framesize_list, ARRAY_SIZE(hi253_preview_framesize_list),CAMERA_PREVIEW);
			
	if (err < 0) {
		v4l_err(client, "%s: camera initialization failed\n", __func__);
		return -EIO;	/* FIXME */
	}
	return 0;
}

/*
 * s_config subdev ops
 * With camera device, we need to re-initialize every single opening time
 * therefor, it is not necessary to be initialized on probe time.
 * except for version checking.
 * NOTE: version checking is optional
 */
static int hi253_s_config(struct v4l2_subdev *sd, int irq, void *platform_data)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct hi253_state *state = to_state(sd);
	struct hi253_platform_data *pdata;

	pdata = client->dev.platform_data;

	if (!pdata) {
		dev_err(&client->dev, "%s: no platform data\n", __func__);
		return -ENODEV;
	}

	/*
	 * Assign default format and resolution
	 * Use configured default information in platform data
	 * or without them, use default information in driver
	 */
	if (!(pdata->default_width && pdata->default_height)) {
		/* TODO: assign driver default resolution */
	} else {
		state->pix.width = pdata->default_width;
		state->pix.height = pdata->default_height;
	}

	if (!pdata->pixelformat)
		state->pix.pixelformat = DEFAULT_FMT;
	else
		state->pix.pixelformat = pdata->pixelformat;

	if (!pdata->freq)
		state->freq = 24000000;	/* 24MHz default */
	else
		state->freq = pdata->freq;

	if (!pdata->is_mipi) {
		state->is_mipi = 0;
		dev_info(&client->dev, "parallel mode\n");
	} else
		state->is_mipi = pdata->is_mipi;

	return 0;
}

/*
 * Sleep and Wakeup function
 */

static int hi253_sleep(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = -EINVAL, i;

	v4l_info(client, "%s: sleep mode\n", __func__);

	for (i = 0; i < HI253_SLEEP_REGS; i++) {
		err = hi253_write(sd, hi253_sleep_reg[i][0], hi253_sleep_reg[i][1]);

		if (err < 0)
			v4l_info(client, "%s: register set failed\n", __func__);
			v4l_err(client, "%s: sleep failed\n", __func__);
			return -EIO;
	}
	return 0;
}

static int hi253_wakeup(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = -EINVAL, i;

	v4l_info(client, "%s: wakeup mode\n", __func__);

	for (i = 0; i < HI253_WAKEUP_REGS; i++) {
		err = hi253_write(sd, hi253_wakeup_reg[i][0], hi253_wakeup_reg[i][1]);

		if (err < 0)
			v4l_info(client, "%s: register set failed\n", __func__);
			v4l_err(client, "%s: wake up failed\n", __func__);
			return -EIO;
	}
	return 0;
}

static int hi253_s_stream(struct v4l2_subdev *sd, int enable)
{
	return enable ? hi253_wakeup(sd) : hi253_sleep(sd);
}

static const struct v4l2_subdev_core_ops hi253_core_ops = {
	.init = hi253_init,	/* initializing API */
	.s_config = hi253_s_config,	/* Fetch platform data */
	.queryctrl = hi253_queryctrl,
	.querymenu = hi253_querymenu,
	.g_ctrl = hi253_g_ctrl,
	.s_ctrl = hi253_s_ctrl,
};

static const struct v4l2_subdev_video_ops hi253_video_ops = {
	.s_crystal_freq = hi253_s_crystal_freq,
	.g_fmt = hi253_g_fmt,
	.s_fmt = hi253_s_fmt,
	.enum_framesizes = hi253_enum_framesizes,
	.enum_frameintervals = hi253_enum_frameintervals,
	.enum_fmt = hi253_enum_fmt,
	.try_fmt = hi253_try_fmt,
	.g_parm = hi253_g_parm,
	.s_parm = hi253_s_parm,
	.s_stream = hi253_s_stream,
};

static const struct v4l2_subdev_ops hi253_ops = {
	.core = &hi253_core_ops,
	.video = &hi253_video_ops,
};

/*
 * hi253_probe
 * Fetching platform data is being done with s_config subdev call.
 * In probe routine, we just register subdev device
 */
static int hi253_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct hi253_state *state;
	struct v4l2_subdev *sd;

	state = kzalloc(sizeof(struct hi253_state), GFP_KERNEL);
	if (state == NULL)
		return -ENOMEM;

	sd = &state->sd;
	strcpy(sd->name, HI253_DRIVER_NAME);

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &hi253_ops);
	dev_info(&client->dev, "hi253 has been probed\n");
	return 0;
}


static int hi253_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id hi253_id[] = {
	{ HI253_DRIVER_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, hi253_id);

static struct v4l2_i2c_driver_data v4l2_i2c_data = {
	.name = HI253_DRIVER_NAME,
	.probe = hi253_probe,
	.remove = hi253_remove,
	.id_table = hi253_id,
};

MODULE_DESCRIPTION("1/5'' 2M Pixels CIS with Image Signal Processor [Hi-253]");
MODULE_AUTHOR("<TMSBG-SWRD04/CEN/FOXCONN>");
MODULE_LICENSE("GPL");

