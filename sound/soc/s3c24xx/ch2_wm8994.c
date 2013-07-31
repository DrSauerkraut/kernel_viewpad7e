/*
 *  ch2_wm8994.c
 *
 *  Copyright (c) 2009 Samsung Electronics Co. Ltd
 *  Author: Jaswinder Singh <jassi.brar@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <linux/io.h>
#include <mach/map.h>
#include <mach/regs-clock.h>

#include <mach/regs-gpio.h>
#include <mach/gpio-bank.h>

#include <plat/regs-iis.h>
#include <linux/mfd/wm8994/core.h>
#include <linux/mfd/wm8994/registers.h>
#include "../codecs/wm8994.h"
#include "s3c-dma.h"
#include "s3c64xx-i2s-v4.h"

static struct snd_soc_card ch2;
static struct platform_device *ch2_snd_device;

extern struct snd_soc_dai i2s_sec_fifo_dai;
extern struct snd_soc_platform s3c_dma_wrapper;
extern const struct snd_kcontrol_new s5p_idma_control;

static int set_epll_rate(unsigned long rate);

static int ch2_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->dai->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	unsigned int rclk, psr=1;
	int blc, bfs, rfs, ret;
	unsigned long epll_out_rate;
	u32 val;
	
	/* Configure the GPI0~GPI4 pins in I2S and Pull-Up mode */
	val = __raw_readl(S5PV210_GPICON);
	val &= ~((0xF<<0) | (0xF<<4) | (0xF<<8) | (0xF<<12) | (0xF<<16));
	val |= (2<<0) | (2<<4) | (2<<8) | (2<<12) | (2<<16);
	__raw_writel(val, S5PV210_GPICON);

	val = __raw_readl(S5PV210_GPIPUD);
	val &= ~((2<<0) | (2<<2) | (2<<4) | (2<<6) | (2<<8));
	val |= (3<<0) | (3<<4) | (3<<8) | (3<<12) | (3<<16);
	__raw_writel(val, S5PV210_GPIPUD); 

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		blc = 16;
		bfs = 32;		
		rfs = 256;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		blc = 24;
		bfs = 48;
		rfs = 384;
		break;
	default:
		return -EINVAL;

	}

	switch (params_rate(params)) 
	{

	case 8000:
		psr = 24;
		break;
	case 11025:
		psr = 16;
		break;
	case 12000:
		psr = 16;
		break;
	case 16000:
		psr = 12;
		break;
	case 22050:
		psr = 8;
		break;
	case 24000:
		psr = 8;
		break;
	case 32000:
		psr = 6;
		break;
	case 44100:
		psr = 4;
		break;
	case 48000:
		psr = 4;
		break;
	case 64000:
		psr = 3;
		break;
	case 88200:
		psr = 2;
		break;
	case 96000:
		psr = 2;
		break;
	default:
		psr = 1;
		break;
	}	

	printk("IIS Audio: sample_rate=%d, format=%d, psr=%d, rfs=%d\n",
		params_rate(params), blc, psr, rfs);


/*
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_CBS_CFS
						| SND_SOC_DAIFMT_I2S
						| SND_SOC_DAIFMT_NB_NF);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_CBM_CFM
						| SND_SOC_DAIFMT_I2S
						| SND_SOC_DAIFMT_NB_NF);
	if (ret < 0)
		return ret;
*/
	rclk = params_rate(params)*rfs;

	epll_out_rate = rclk * psr;

	/* Set EPLL clock rate */
	ret = set_epll_rate(epll_out_rate);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C64XX_CLKSRC_CDCLK,
					0, SND_SOC_CLOCK_OUT);
	if (ret < 0)
		return ret;

	/* We use MUX for basic ops in SoC-Master mode */
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C64XX_CLKSRC_MUX,	//set the I2S clock as I2SCLK (RCLKSRC:bit 10)
					0, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_I2SV2_DIV_PRESCALER, psr-1);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_I2SV2_DIV_BCLK, bfs);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_I2SV2_DIV_RCLK, rfs);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_MCLK1, rclk,	SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;
#if 0	
	ret = snd_soc_dai_set_pll(codec_dai, WM8994_SYSCLK_FLL1,
						rclk, SND_SOC_CLOCK_IN);
	if (ret < 0)
			return ret;
#endif
	return 0;
}

/*
 * SMDK64XX WM8580 DAI operations.
 */
static struct snd_soc_ops ch2_ops = {
	.hw_params = ch2_hw_params,
};

static int s5pv2xx_wm8994_init(struct snd_soc_codec *codec)
{
	int ret;
	
#if 0
	for (i = 0; i < ARRAY_SIZE(ssm2602_controls); i++) {
		err = snd_ctl_add(codec->card,
			snd_soc_cnew(&ssm2602_controls[i],
			codec, NULL));
		if (err < 0) {
			return err;
		}
	}
#endif
	snd_soc_update_bits(codec, WM8994_GPIO_1, 0x1F, 0x0);

	snd_soc_dapm_enable_pin(codec, "HPOUT1R");
	snd_soc_dapm_enable_pin(codec, "HPOUT1L");
	snd_soc_dapm_sync(codec);

	/* Set the Codec DAI configuration */
	ret = snd_soc_dai_set_fmt(&wm8994_dai[0], SND_SOC_DAIFMT_I2S
					 | SND_SOC_DAIFMT_NB_NF
					 | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	/* Set the AP DAI configuration */
//	ret = snd_soc_dai_set_fmt(&s3c64xx_i2s_dai[I2S_NUM], SND_SOC_DAIFMT_I2S
	ret = snd_soc_dai_set_fmt(&s3c64xx_i2s_v4_dai[S3C24XX_DAI_I2S], SND_SOC_DAIFMT_I2S
					 | SND_SOC_DAIFMT_NB_NF
					 | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	return 0;
}

static struct snd_soc_dai_link ch2_dai[] = {
	{ 
		.name = "WM8994 AIF1 Playback",
		.stream_name = "WM8994 Playback Stream",
		.cpu_dai = &s3c64xx_i2s_v4_dai[S3C24XX_DAI_I2S],
//		.cpu_dai = &s3c64xx_i2s_dai[I2S_NUM],
		.codec_dai = &wm8994_dai[0],
		.init = s5pv2xx_wm8994_init,
		.ops = &ch2_ops,

	},
	{ 
		.name = "WM8994 AIF1 Capture",
		.stream_name = "WM8994 Capture Stream",
		.cpu_dai = &s3c64xx_i2s_v4_dai[S3C24XX_DAI_I2S],
//		.cpu_dai = &s3c64xx_i2s_dai[I2S_NUM],
		.codec_dai = &wm8994_dai[0],
		.init = s5pv2xx_wm8994_init,
		.ops = &ch2_ops,
	
	},
	{ 
		.name = "WM8994 AIF2",
		.stream_name = "WM8994 AIF2 Hi-Fi",
		.cpu_dai = &s3c64xx_i2s_v4_dai[S3C24XX_DAI_I2S],
//		.cpu_dai = &s3c64xx_i2s_dai[I2S_NUM],
		.codec_dai = &wm8994_dai[1],
		.init = s5pv2xx_wm8994_init,
		.ops = &ch2_ops,
	
	},
	{ 
		.name = "WM8994 AIF3",
		.stream_name = "WM8994 AIF3 Hi-Fi",
		.cpu_dai = &s3c64xx_i2s_v4_dai[S3C24XX_DAI_I2S],
//		.cpu_dai = &s3c64xx_i2s_dai[I2S_NUM],
		.codec_dai = &wm8994_dai[2],
		.init = s5pv2xx_wm8994_init,
		.ops = &ch2_ops,

	},
};

static struct snd_soc_card ch2 = {
	.name = "s5p",
	.platform = &s3c_dma_wrapper,
	.dai_link = ch2_dai,
	.num_links = ARRAY_SIZE(ch2_dai),
};

static struct snd_soc_device ch2_snd_devdata = {
	.card = &ch2,
	.codec_dev = &soc_codec_dev_wm8994,
};

static int set_epll_rate(unsigned long rate)
{
	struct clk *fout_epll;

	fout_epll = clk_get(NULL, "fout_epll");
	if (IS_ERR(fout_epll)) {
		printk(KERN_ERR "%s: failed to get fout_epll\n", __func__);
		return -ENOENT;
	}

	if (rate == clk_get_rate(fout_epll))
		goto out;

	clk_disable(fout_epll);
	clk_set_rate(fout_epll, rate);
	clk_enable(fout_epll);

out:
	clk_put(fout_epll);

	return 0;
}

static int __init ch2_audio_init(void)
{
	int ret;
	u32 val;
	u32 reg;

#include <mach/map.h>
#define S3C_VA_AUDSS	S3C_ADDR(0x01600000)	/* Audio SubSystem */
#include <mach/regs-audss.h>

	/* MUXepll : FOUTepll , config clock path*/
	writel(readl(S5P_CLK_SRC0)|S5P_CLKSRC0_EPLL_MASK, S5P_CLK_SRC0);
	/* AUDIO0_SEL : FOUTepll */
	writel((readl(S5P_CLK_SRC6)&~(0xF<<0))|(7<<0), S5P_CLK_SRC6);
	/* AUDIO0 CLK_DIV6 setting */
	writel(readl(S5P_CLK_DIV6)&~(0xF<<0),S5P_CLK_DIV6);

	/* We use I2SCLK for rate generation, so set EPLLout as
	 * the parent of I2SCLK.
	 */
	val = readl(S5P_CLKSRC_AUDSS);
	val &= ~(0x3 << 2);		//	val &= ~(0x3<<2);
	val |= (0x2 << 2);
	val |= (1<<0);
	writel(val, S5P_CLKSRC_AUDSS);

	val = readl(S5P_CLKGATE_AUDSS);
	val |= (0x7f<<0);
	writel(val, S5P_CLKGATE_AUDSS);

#define CONFIG_S5P_LPAUDIO

#ifdef CONFIG_S5P_LPAUDIO
	/* yman.seo CLKOUT is prior to CLK_OUT of SYSCON. XXTI & XUSBXTI work in sleep mode */
	reg = __raw_readl(S5P_OTHERS);
	reg &= ~(0x0003 << 8);
	reg |= 0x0003 << 8;	/* XUSBXTI */
	__raw_writel(reg, S5P_OTHERS);
#else
	/* yman.seo Set XCLK_OUT as 24MHz (XUSBXTI -> 24MHz) */
	reg = __raw_readl(S5P_CLK_OUT);
	reg &= ~S5P_CLKOUT_CLKSEL_MASK;
	reg |= S5P_CLKOUT_CLKSEL_XUSBXTI;
	reg &= ~S5P_CLKOUT_DIV_MASK;
	reg |= 0x0001 << S5P_CLKOUT_DIV_SHIFT;	/* DIVVAL = 1, Ratio = 2 = DIVVAL + 1 */
	__raw_writel(reg, S5P_CLK_OUT);

	reg = __raw_readl(S5P_OTHERS);
	reg &= ~(0x0003 << 8);
	reg |= 0x0000 << 8;	/* Clock from SYSCON */
	__raw_writel(reg, S5P_OTHERS);
#endif

	ch2_snd_device = platform_device_alloc("soc-audio", 0);
	if (!ch2_snd_device)
		return -ENOMEM;

	platform_set_drvdata(ch2_snd_device, &ch2_snd_devdata);
	ch2_snd_devdata.dev = &ch2_snd_device->dev;
	ret = platform_device_add(ch2_snd_device);

	if (ret)
		platform_device_put(ch2_snd_device);

	return ret;
}
static void __exit ch2_audio_exit(void)
{
	platform_device_unregister(ch2_snd_device);
}

module_init(ch2_audio_init);
module_exit(ch2_audio_exit);

MODULE_AUTHOR("Jaswinder Singh, jassi.brar@samsung.com");
MODULE_DESCRIPTION("ALSA SoC SMDK64XX WM8580");
MODULE_LICENSE("GPL");

