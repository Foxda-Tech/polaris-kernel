/*
 * Copyright (c) 2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/regmap.h>
#include <linux/soundwire/soundwire.h>
#include <linux/module.h>
#include <linux/init.h>

#include "internal.h"

static int regmap_swr_gather_write(void *context,
				const void *reg, size_t reg_size,
				const void *val, size_t val_size)
{
	struct device *dev = context;
	struct swr_device *swr = to_swr_device(dev);
	struct regmap *map = dev_get_regmap(dev, NULL);
	size_t addr_bytes = map->format.reg_bytes;
	int ret = 0;
	int i;
	u32 reg_addr = 0;

	if (swr == NULL) {
		dev_err(dev, "%s: swr device is NULL\n", __func__);
		return -EINVAL;
	}
	if (reg_size != addr_bytes) {
		dev_err(dev, "%s: reg size %zd bytes not supported\n",
			__func__, reg_size);
		return -EINVAL;
	}
	reg_addr = *(u16 *)reg;
	for (i = 0; i < val_size; i++) {
		ret = swr_write(swr, swr->dev_num, (reg_addr+i),
				(u32 *)(val+i));
		if (ret < 0) {
			dev_err(dev, "%s: write reg 0x%x failed, err %d\n",
				__func__, (reg_addr+i), ret);
			break;
		}
	}
	return ret;
}

static int regmap_swr_write(void *context, const void *data, size_t count)
{
	struct device *dev = context;
	struct regmap *map = dev_get_regmap(dev, NULL);
	size_t addr_bytes = map->format.reg_bytes;

	WARN_ON(count < addr_bytes);

	return regmap_swr_gather_write(context, data, addr_bytes,
					(data + addr_bytes),
					(count - addr_bytes));
}

static int regmap_swr_read(void *context,
			const void *reg, size_t reg_size,
			void *val, size_t val_size)
{
	struct device *dev = context;
	struct swr_device *swr = to_swr_device(dev);
	struct regmap *map = dev_get_regmap(dev, NULL);
	size_t addr_bytes = map->format.reg_bytes;
	int ret = 0;
	u32 reg_addr = 0;

	if (swr == NULL) {
		dev_err(dev, "%s: swr is NULL\n", __func__);
		return -EINVAL;
	}
	if (reg_size != addr_bytes) {
		dev_err(dev, "%s: register size %zd bytes not supported\n",
			__func__, reg_size);
		return -EINVAL;
	}
	reg_addr = *(u32 *)reg;
	ret = swr_read(swr, swr->dev_num, reg_addr, val, val_size);
	if (ret < 0)
		dev_err(dev, "%s: codec reg 0x%x read failed %d\n",
			__func__, reg_addr, ret);
	return ret;
}

static struct regmap_bus regmap_swr = {
	.write = regmap_swr_write,
	.gather_write = regmap_swr_gather_write,
	.read = regmap_swr_read,
	.reg_format_endian_default = REGMAP_ENDIAN_NATIVE,
	.val_format_endian_default = REGMAP_ENDIAN_NATIVE,
};

struct regmap *regmap_init_swr(struct swr_device *swr,
				const struct regmap_config *config)
{
	return regmap_init(&swr->dev, &regmap_swr, &swr->dev, config);
}
EXPORT_SYMBOL(regmap_init_swr);

struct regmap *devm_regmap_init_swr(struct swr_device *swr,
					const struct regmap_config *config)
{
	return devm_regmap_init(&swr->dev, &regmap_swr, &swr->dev, config);
}
EXPORT_SYMBOL(devm_regmap_init_swr);

MODULE_LICENSE("GPL v2");
