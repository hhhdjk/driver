/*
 * Copyright (c) 2007 Ben Dooks
 * Copyright (c) 2008 Simtec Electronics
 *     Ben Dooks <ben@simtec.co.uk>, <ben-linux@fluff.org>
 * Copyright (c) 2013 Tomasz Figa <tomasz.figa@gmail.com>
 *
 * PWM driver for Samsung SoCs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/export.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/time.h>
#include <linux/reset.h>

/* For struct samsung_timer_variant and samsung_pwm_lock. */
#include <clocksource/samsung_pwm.h>

#define REG_TCFG0			0x00
#define REG_TCFG1			0x04
#define REG_TCON			0x08

#define REG_TCNTB(chan)			(0x0c + ((chan) * 0xc))
#define REG_TCMPB(chan)			(0x10 + ((chan) * 0xc))

#define TCFG0_PRESCALER_MASK		0xff
#define TCFG0_PRESCALER1_SHIFT		8

#define TCFG1_MUX_MASK			0xf
#define TCFG1_SHIFT(chan)		(4 * (chan))

#define TCON_START(chan)		    BIT(4 * (chan) + 0)
#define TCON_MANUALUPDATE(chan)		BIT(4 * (chan) + 1)
#define TCON_INVERT(chan)		    BIT(4 * (chan) + 2)
#define _TCON_AUTORELOAD(chan)		BIT(4 * (chan) + 3)
#define _TCON_AUTORELOAD4(chan)		BIT(4 * (chan) + 2)
#define TCON_AUTORELOAD(chan)		((chan < 5) ? _TCON_AUTORELOAD(chan) : _TCON_AUTORELOAD4(chan))

/**
 * struct samsung_pwm_channel - private data of PWM channel
 * @period_ns:	current period in nanoseconds programmed to the hardware
 * @duty_ns:	current duty time in nanoseconds programmed to the hardware
 * @tin_ns:	time of one timer tick in nanoseconds with current timer rate
 */
struct samsung_pwm_channel {
	u32 period_ns;
	u32 duty_ns;
	u32 tin_ns;
};

/**
 * struct samsung_pwm_chip - private data of PWM chip
 * @chip:		generic PWM chip
 * @variant:		local copy of hardware variant data
 * @inverter_mask:	inverter status for all channels - one bit per channel
 * @base:		base address of mapped PWM registers
 * @base_clk:		base clock used to drive the timers
 * @tclk0:		external clock 0 (can be ERR_PTR if not present)
 * @tclk1:		external clock 1 (can be ERR_PTR if not present)
 * add for s5p4418
 * @tclk2:		external clock 0 (can be ERR_PTR if not present)
 * @tclk3:		external clock 1 (can be ERR_PTR if not present)

 */
struct samsung_pwm_chip {
	struct pwm_chip chip;
	struct samsung_pwm_variant variant;
	u8 inverter_mask;                                   // 15

	void __iomem *base;                                 // chip->base 映射后的pwm基地址
	struct clk *base_clk;

	/* for suspend/resume of s5p4418/s5p6818 */
	u32 tcfg0;
	u32 tcfg1;
	u32 tcon;
	u32 tcntb[SAMSUNG_PWM_NUM];
	u32 tcmpb[SAMSUNG_PWM_NUM];
	u32 is_enabled;
};

#ifndef CONFIG_CLKSRC_SAMSUNG_PWM
static DEFINE_SPINLOCK(samsung_pwm_lock);
#endif

static inline struct samsung_pwm_chip *to_samsung_pwm_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct samsung_pwm_chip, chip);
}

static inline unsigned int to_tcon_channel(unsigned int channel)
{
	return (channel == 0) ? 0 : (channel + 1);
}

static void pwm_samsung_set_divisor(struct samsung_pwm_chip *pwm, unsigned int channel, u8 divisor)
{
	u8 shift = TCFG1_SHIFT(channel);
	unsigned long flags;
	u32 reg;
	u8 bits;

	bits = (fls(divisor) - 1) - pwm->variant.div_base;

	spin_lock_irqsave(&samsung_pwm_lock, flags);

	reg = readl(pwm->base + REG_TCFG1);
	reg &= ~(TCFG1_MUX_MASK << shift);
	reg |= bits << shift;
	writel(reg, pwm->base + REG_TCFG1);

	spin_unlock_irqrestore(&samsung_pwm_lock, flags);
}

static void pwm_samsung_set_tclk(struct samsung_pwm_chip *pwm, unsigned int chan)
{
	u32 tclk = 5;
	unsigned long flags;
	u32 reg;

	spin_lock_irqsave(&samsung_pwm_lock, flags);

	reg = readl(pwm->base + REG_TCFG1);
	reg |= tclk << TCFG1_SHIFT(chan);
	writel(reg, pwm->base + REG_TCFG1);

	spin_unlock_irqrestore(&samsung_pwm_lock, flags);
}



static unsigned long pwm_samsung_get_tin_rate(struct samsung_pwm_chip *chip, unsigned int chan)
{
	unsigned long rate;
	u32 reg;
    unsigned int div;
	unsigned int pre_div;

	rate = clk_get_rate(chip->base_clk);

    writel(0, chip->base + REG_TCFG0);
	reg = readl(chip->base + REG_TCFG0);
	if (chan >= 2)
		reg >>= TCFG0_PRESCALER1_SHIFT;
	reg &= TCFG0_PRESCALER_MASK;
    div = reg+1;

    writel(0, chip->base + REG_TCFG1);
	reg = readl(chip->base + REG_TCFG1);
    reg >>= TCFG1_SHIFT(chan);
    reg &= TCFG1_MUX_MASK;
    pre_div = (1UL << reg);

	return rate / div / pre_div;
}


static unsigned long pwm_samsung_calc_tin(struct samsung_pwm_chip *chip, unsigned int chan, unsigned long freq)
{
	struct samsung_pwm_variant *variant = &chip->variant;
	struct clk *clk;
	unsigned long rate;
	u8 div;

    rate = pwm_samsung_get_tin_rate(chip , chan);

    return rate;
}

static int pwm_samsung_request(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct samsung_pwm_chip *our_chip = to_samsung_pwm_chip(chip);
	struct samsung_pwm_channel *our_chan;

	if (!(our_chip->variant.output_mask & BIT(pwm->hwpwm))) {
		dev_warn(chip->dev,
			"tried to request PWM channel %d without output\n",
			pwm->hwpwm);
		return -EINVAL;
	}

	our_chan = devm_kzalloc(chip->dev, sizeof(*our_chan), GFP_KERNEL);
	if (!our_chan)
		return -ENOMEM;

	pwm_set_chip_data(pwm, our_chan);

	return 0;
}

static void pwm_samsung_free(struct pwm_chip *chip, struct pwm_device *pwm)
{
	devm_kfree(chip->dev, pwm_get_chip_data(pwm));
	pwm_set_chip_data(pwm, NULL);
}

static int pwm_samsung_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct samsung_pwm_chip *our_chip = to_samsung_pwm_chip(chip);
	unsigned int tcon_chan = to_tcon_channel(pwm->hwpwm);
	unsigned long flags;
	u32 tcon;

	spin_lock_irqsave(&samsung_pwm_lock, flags);

	tcon = readl(our_chip->base + REG_TCON);

	tcon &= ~TCON_START(tcon_chan);
	tcon |= TCON_MANUALUPDATE(tcon_chan);
	writel(tcon, our_chip->base + REG_TCON);

	tcon &= ~TCON_MANUALUPDATE(tcon_chan);
	tcon |= TCON_START(tcon_chan) | TCON_AUTORELOAD(tcon_chan);
	writel(tcon, our_chip->base + REG_TCON);

	spin_unlock_irqrestore(&samsung_pwm_lock, flags);

	return 0;
}

static void pwm_samsung_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct samsung_pwm_chip *our_chip = to_samsung_pwm_chip(chip);
	unsigned int tcon_chan = to_tcon_channel(pwm->hwpwm);
	unsigned long flags;
	u32 tcon;

	spin_lock_irqsave(&samsung_pwm_lock, flags);

	tcon = readl(our_chip->base + REG_TCON);
	tcon &= ~TCON_AUTORELOAD(tcon_chan);
	writel(tcon, our_chip->base + REG_TCON);

	spin_unlock_irqrestore(&samsung_pwm_lock, flags);
}

static void pwm_samsung_manual_update(struct samsung_pwm_chip *chip,
				      struct pwm_device *pwm)
{
	unsigned int tcon_chan = to_tcon_channel(pwm->hwpwm);
	u32 tcon;
	unsigned long flags;

	spin_lock_irqsave(&samsung_pwm_lock, flags);

	tcon = readl(chip->base + REG_TCON);
	tcon |= TCON_MANUALUPDATE(tcon_chan);
	writel(tcon, chip->base + REG_TCON);

	tcon &= ~TCON_MANUALUPDATE(tcon_chan);
	writel(tcon, chip->base + REG_TCON);

	spin_unlock_irqrestore(&samsung_pwm_lock, flags);
}

static int pwm_samsung_config(struct pwm_chip *chip, struct pwm_device *pwm, int duty_ns, int period_ns)
{
	struct samsung_pwm_chip *our_chip = to_samsung_pwm_chip(chip);
	struct samsung_pwm_channel *chan = pwm_get_chip_data(pwm);
	u32 tin_ns = chan->tin_ns , tcnt, tcmp, oldtcmp;
	unsigned long tin_rate;
	u64 correction;

	/*
	 * We currently avoid using 64bit arithmetic by using the
	 * fact that anything faster than 1Hz is easily representable
	 * by 32bits.
	 */
	if (period_ns > NSEC_PER_SEC || duty_ns > NSEC_PER_SEC || period_ns == 0)
		return -ERANGE;

	if (period_ns == chan->period_ns && duty_ns == chan->duty_ns)
		return 0;

	tcnt = readl(our_chip->base + REG_TCNTB(pwm->hwpwm));
	oldtcmp = readl(our_chip->base + REG_TCMPB(pwm->hwpwm));

	/* We need tick count for calculation, not last tick. */
	++tcnt;

	tin_rate = pwm_samsung_calc_tin(our_chip, pwm->hwpwm, NSEC_PER_SEC / period_ns);

	/* Check to see if we are changing the clock rate of the PWM. */
	if (chan->period_ns != period_ns) {

        tin_ns = NSEC_PER_SEC / tin_rate;       // 计数一次所需时间

        tcnt = (period_ns / tin_ns) - 1;
		printk("duty_ns=%d, period_ns=%d (%u)\n",duty_ns, period_ns, tin_ns);
		printk("tin_rate=%lu\n", tin_rate);

	}

	/* Period is too short. */
	if (tcnt <= 1)
    {
        printk("Period is too short. (%d)\r\n" , tcnt);
        return -ERANGE;
    }
		

	/* Note that counters count down. */
    tcmp = duty_ns / tin_ns;

	/* 0% duty is not available */
	if (!tcmp)
		++tcmp;

	tcmp = tcnt - tcmp;

	/* Decrement to get tick numbers, instead of tick counts. */
	--tcnt;
	/* -1UL will give 100% duty. */
	--tcmp;

	printk("tin_ns=%u, tcmp=%u/%u, pwm->hwpwm=%d\n", tin_ns, tcmp, tcnt , pwm->hwpwm );

	/* Update PWM registers. */
	writel(tcnt, our_chip->base + REG_TCNTB(pwm->hwpwm));
	writel(tcmp, our_chip->base + REG_TCMPB(pwm->hwpwm));

	/*
	 * In case the PWM is currently at 100% duty cycle, force a manual
	 * update to prevent the signal staying high if the PWM is disabled
	 * shortly afer this update (before it autoreloaded the new values).
	 */
	if (oldtcmp == (u32) -1) {
		dev_dbg(our_chip->chip.dev, "Forcing manual update");
		pwm_samsung_manual_update(our_chip, pwm);
	}

	chan->period_ns = period_ns;
	chan->tin_ns = tin_ns;
	chan->duty_ns = duty_ns;

	return 0;
}

static void pwm_samsung_set_invert(struct samsung_pwm_chip *chip,
				   unsigned int channel, bool invert)
{
	unsigned int tcon_chan = to_tcon_channel(channel);
	unsigned long flags;
	u32 tcon;

	spin_lock_irqsave(&samsung_pwm_lock, flags);

	tcon = readl(chip->base + REG_TCON);

	if (invert) {
		chip->inverter_mask |= BIT(channel);
		tcon |= TCON_INVERT(tcon_chan);
	} else {
		chip->inverter_mask &= ~BIT(channel);
		tcon &= ~TCON_INVERT(tcon_chan);
	}

	writel(tcon, chip->base + REG_TCON);

	spin_unlock_irqrestore(&samsung_pwm_lock, flags);
}

static int pwm_samsung_set_polarity(struct pwm_chip *chip,struct pwm_device *pwm,enum pwm_polarity polarity)
{
	struct samsung_pwm_chip *our_chip = to_samsung_pwm_chip(chip);
    bool invert = (polarity == PWM_POLARITY_NORMAL);
	/* Inverted means normal in the hardware. */
	pwm_samsung_set_invert(our_chip, pwm->hwpwm, invert);

	return 0;
}

static const struct pwm_ops pwm_samsung_ops = {
	.request	= pwm_samsung_request,
	.free		= pwm_samsung_free,
	.enable		= pwm_samsung_enable,
	.disable	= pwm_samsung_disable,
	.config		= pwm_samsung_config,
	.set_polarity	= pwm_samsung_set_polarity,
	.owner		= THIS_MODULE,
};

#ifdef CONFIG_OF
static const struct samsung_pwm_variant s5pxx18_variant = {
	.bits		= 32,
	.div_base	= 0,
	.has_tint_cstat	= true,
	.tclk_mask	= BIT(SAMSUNG_PWM_NUM),
};

static const struct of_device_id samsung_pwm_matches[] = {
	{ .compatible = "nexell,s5p4418-pwm", .data = &s5pxx18_variant },
	{ .compatible = "nexell,s5p6818-pwm", .data = &s5pxx18_variant },
	{},
};
MODULE_DEVICE_TABLE(of, samsung_pwm_matches);

static int pwm_samsung_parse_dt(struct samsung_pwm_chip *chip)
{
	struct device_node *np = chip->chip.dev->of_node;
	const struct of_device_id *match;
	struct property *prop;
	const __be32 *cur;
	u32 val;

	match = of_match_node(samsung_pwm_matches, np);
	if (!match)
		return -ENODEV;

	memcpy(&chip->variant, match->data, sizeof(chip->variant));

	of_property_for_each_u32(np, "samsung,pwm-outputs", prop, cur, val) {
		if (val >= SAMSUNG_PWM_NUM) {
			dev_err(chip->chip.dev,
				"%s: invalid channel index in samsung,pwm-outputs property\n",
								__func__);
			continue;
		}
		chip->variant.output_mask |= BIT(val);
	}

	return 0;
}
#else
static int pwm_samsung_parse_dt(struct samsung_pwm_chip *chip)
{
	return -ENODEV;
}
#endif

static int pwm_samsung_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct samsung_pwm_chip *chip;
	struct resource *res;
	unsigned int chan;
	int ret;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;

	chip->chip.dev = &pdev->dev;
	chip->chip.ops = &pwm_samsung_ops;
	chip->chip.base = -1;
	chip->chip.npwm = SAMSUNG_PWM_NUM;
	chip->inverter_mask = BIT(SAMSUNG_PWM_NUM) - 1;

	if (IS_ENABLED(CONFIG_OF) && pdev->dev.of_node) {
		ret = pwm_samsung_parse_dt(chip);
		if (ret)
			return ret;

		chip->chip.of_xlate = of_pwm_xlate_with_flags;
		chip->chip.of_pwm_n_cells = 3;
	} else {
		if (!pdev->dev.platform_data) {
			dev_err(&pdev->dev, "no platform data specified\n");
			return -EINVAL;
		}
		memcpy(&chip->variant, pdev->dev.platform_data,
							sizeof(chip->variant));
	}
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    printk("pwm addr :0x%p , size : %d\r\n" , res->start , resource_size(res));

	chip->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(chip->base)){
         printk("pwm addr ioremap failed\r\n");
		return PTR_ERR(chip->base);
    }

	chip->base_clk = devm_clk_get(&pdev->dev, "timers");
	if (!IS_ERR(chip->base_clk)) {
		ret = clk_prepare_enable(chip->base_clk);
		if (ret < 0) {
			dev_err(dev, "failed to enable base clock\n");
			return ret;
		}
	}
    printk("base_clk at %lu\r\n" ,clk_get_rate(chip->base_clk));

#ifdef CONFIG_RESET_CONTROLLER
	if (of_device_is_compatible(pdev->dev.of_node, "nexell,s5p4418-pwm") ||
        of_device_is_compatible(pdev->dev.of_node, "nexell,s5p6818-pwm")) 
    {
		struct reset_control *rst = devm_reset_control_get(&pdev->dev, "pwm-reset");
		if (IS_ERR(rst)) {
			dev_err(&pdev->dev, "PWM failed to get reset_control\n");
			return -EINVAL;
		}
		if (reset_control_status(rst))
			reset_control_reset(rst);
	}
#endif
	/*
     *  开启invert  0 off : 1 on
	 */
	for (chan = 0; chan < SAMSUNG_PWM_NUM; ++chan)
		if (chip->variant.output_mask & BIT(chan))
			pwm_samsung_set_invert(chip, chan, true);

	platform_set_drvdata(pdev, chip);
    /*
     *  注册pwm
	 */
	ret = pwmchip_add(&chip->chip);
	if (ret < 0) {
		dev_err(dev, "failed to register PWM chip\n");
		clk_disable_unprepare(chip->base_clk);
		return ret;
	}

	return 0;
}

static int pwm_samsung_remove(struct platform_device *pdev)
{
	struct samsung_pwm_chip *chip = platform_get_drvdata(pdev);
	int ret;

	ret = pwmchip_remove(&chip->chip);
	if (ret < 0)
		return ret;

	if (!IS_ERR(chip->base_clk))
		clk_disable_unprepare(chip->base_clk);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int pwm_samsung_suspend(struct device *dev)
{
	struct samsung_pwm_chip *chip = dev_get_drvdata(dev);
	unsigned int i;

	/*
	 * patch for s5p4418/s5p6818
	 * s5p4418/s5p6818 pwm register backup and tclk disable.
	 */
	if (of_device_is_compatible(dev->of_node, "nexell,s5p4418-pwm") ||
	    of_device_is_compatible(dev->of_node, "nexell,s5p6818-pwm")) {
		chip->tcfg0 = readl(chip->base + REG_TCFG0);
		chip->tcfg1 = readl(chip->base + REG_TCFG1);
		chip->tcon = readl(chip->base + REG_TCON);
		for (i = 0; i < SAMSUNG_PWM_NUM; ++i) {
			struct pwm_device *pwm = &chip->chip.pwms[i];
			unsigned int tcon_chan = to_tcon_channel(pwm->hwpwm);

			chip->tcntb[i] = readl(chip->base + REG_TCNTB(i));
			chip->tcmpb[i] = readl(chip->base + REG_TCMPB(i));
			if ((chip->tcon & TCON_AUTORELOAD(tcon_chan)) &&
			    (chip->variant.output_mask & BIT(i))) {
				chip->is_enabled |= BIT(i);
				pwm_samsung_disable(&chip->chip, pwm);
			} else
				chip->is_enabled &= ~BIT(i);
		}
		chip->tcon = readl(chip->base + REG_TCON);

		if (!IS_ERR(chip->base_clk))
			clk_disable_unprepare(chip->base_clk);
	}

	/*
	 * No one preserves these values during suspend so reset them.
	 * Otherwise driver leaves PWM unconfigured if same values are
	 * passed to pwm_config() next time.
	 */
	for (i = 0; i < SAMSUNG_PWM_NUM; ++i) {
		struct pwm_device *pwm = &chip->chip.pwms[i];
		struct samsung_pwm_channel *chan = pwm_get_chip_data(pwm);
		if (!chan)
			continue;
		chan->period_ns = 0;
		chan->duty_ns = 0;
	}

	return 0;
}

static int pwm_samsung_resume(struct device *dev)
{
	struct samsung_pwm_chip *chip = dev_get_drvdata(dev);
	unsigned int chan;

	/*
	 * patch for s5p4418/s5p6818
	 * s5p4418/s5p6818 pwm must be reset before enabled
	 */
	if (of_device_is_compatible(dev->of_node, "nexell,s5p4418-pwm") ||
	    of_device_is_compatible(dev->of_node, "nexell,s5p6818-pwm")) {
#ifdef CONFIG_RESET_CONTROLLER
		struct reset_control *rst =
			reset_control_get(dev, "pwm-reset");
		if (IS_ERR(rst)) {
			dev_err(dev, "PWM failed to get reset_control\n");
			return -EINVAL;
		}

		if (reset_control_status(rst))
			reset_control_reset(rst);

		reset_control_put(rst);
#endif
		/*
		 * patch for s5p4418/s5p6818
		 * s5p4418/s5p6818 pwm register restore and tclk enable.
		 */
		if (!IS_ERR(chip->base_clk))
			clk_prepare_enable(chip->base_clk);

		}
		writel(chip->tcfg0, chip->base + REG_TCFG0);
		writel(chip->tcfg1, chip->base + REG_TCFG1);
		writel(chip->tcon, chip->base + REG_TCON);
		for (chan = 0; chan < SAMSUNG_PWM_NUM; ++chan) {
			struct pwm_device *pwm = &chip->chip.pwms[chan];

			writel(chip->tcntb[chan], chip->base + REG_TCNTB(chan));
			writel(chip->tcmpb[chan], chip->base + REG_TCMPB(chan));
			if ((chip->is_enabled & BIT(chan)) &&
			    (chip->variant.output_mask & BIT(chan))) {
				pwm_samsung_enable(&chip->chip, pwm);
			}
		}

	/*
	 * Inverter setting must be preserved across suspend/resume
	 * as nobody really seems to configure it more than once.
	 */
	for (chan = 0; chan < SAMSUNG_PWM_NUM; ++chan) {
		if (chip->variant.output_mask & BIT(chan))
			pwm_samsung_set_invert(chip, chan,
					chip->inverter_mask & BIT(chan));
	}

	return 0;
}

#endif

static SIMPLE_DEV_PM_OPS(pwm_samsung_pm_ops, pwm_samsung_suspend,
			 pwm_samsung_resume);

static struct platform_driver pwm_samsung_driver = {
	.driver		= {
		.name	= "samsung-pwm",
		.pm	= &pwm_samsung_pm_ops,
		.of_match_table = of_match_ptr(samsung_pwm_matches),
	},
	.probe		= pwm_samsung_probe,
	.remove		= pwm_samsung_remove,
};
module_platform_driver(pwm_samsung_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tomasz Figa <tomasz.figa@gmail.com>");
MODULE_ALIAS("platform:samsung-pwm");
