#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display, LOG_LEVEL_DBG);

// TODO: Abstract this API to a generic segmented display
#include <bu9795_driver.h>

#include "display.h"

static struct device *dev_segment = NULL;
static uint32_t set_symbols = 0;

int display_set_temperature(const struct sensor_value *value)
{
    if (dev_segment == NULL)
    {
        return -ENOENT;
    }

    if (value == NULL)
    {
        // TODO: disable decimal point
        bu9795_set_segment(dev_segment, 0, -1);
        bu9795_set_segment(dev_segment, 1, -1);
        bu9795_set_segment(dev_segment, 2, -1);
        display_clear_symbols(DISPLAY_SYMBOL_TEMPERATURE_DECIMAL);
    }
    else
    {
        bu9795_set_segment(dev_segment, 0, value->val1 / 10);
        bu9795_set_segment(dev_segment, 1, value->val1 % 10);
        bu9795_set_segment(dev_segment, 2, value->val2 / 100000);
        display_set_symbols(DISPLAY_SYMBOL_TEMPERATURE_DECIMAL);
    }

    bu9795_flush(dev_segment);
    return 0;
}

int display_set_humidity(const struct sensor_value *value)
{
    if (dev_segment == NULL)
    {
        return -ENOENT;
    }

    if (value == NULL)
    {
        // TODO: disable decimal point
        bu9795_set_segment(dev_segment, 3, -1);
        bu9795_set_segment(dev_segment, 4, -1);
        bu9795_set_segment(dev_segment, 5, -1);
        display_clear_symbols(DISPLAY_SYMBOL_HUMIDITY_DECIMAL);
    }
    else
    {
        bu9795_set_segment(dev_segment, 3, value->val1 / 10);
        bu9795_set_segment(dev_segment, 4, value->val1 % 10);
        bu9795_set_segment(dev_segment, 5, value->val2 / 100000);
        display_set_symbols(DISPLAY_SYMBOL_HUMIDITY_DECIMAL);
    }

    bu9795_flush(dev_segment);
    return 0;
}

int display_set_battery(int percent)
{
    if (dev_segment == NULL)
    {
        return -ENOENT;
    }

    if (percent > 80)
    {
        bu9795_set_segment(dev_segment, 6, 6);
    }
    else if (percent > 60)
    {
        bu9795_set_segment(dev_segment, 6, 5);
    }
    else if (percent > 40)
    {
        bu9795_set_segment(dev_segment, 6, 4);
    }
    else if (percent > 20)
    {
        bu9795_set_segment(dev_segment, 6, 3);
    }
    else if (percent > 0)
    {
        bu9795_set_segment(dev_segment, 6, 2);
    }
    else
    {
        bu9795_set_segment(dev_segment, 6, 1);
    }

    bu9795_flush(dev_segment);
    return 0;
}

int display_set_symbols(uint8_t symbols)
{
    uint32_t old_symbols = set_symbols;
    set_symbols |= symbols;

    if (dev_segment == NULL)
    {
        return -ENOENT;
    }

    if (set_symbols != old_symbols)
    {
        bu9795_set_symbol(dev_segment, set_symbols);
        bu9795_flush(dev_segment);
    }
    return 0;
}

int display_clear_symbols(uint8_t symbols)
{
    uint32_t old_symbols = set_symbols;
    set_symbols &= ~symbols;

    if (dev_segment == NULL)
    {
        return -ENOENT;
    }

    if (set_symbols != old_symbols)
    {
        bu9795_set_symbol(dev_segment, set_symbols);
        bu9795_flush(dev_segment);
    }
    return 0;
}

static int display_setup(struct device *arg)
{
    dev_segment = device_get_binding(DT_ALIAS_SEGMENT0_LABEL);
    if (dev_segment == NULL)
    {
        LOG_ERR("Didn't find %s device", DT_ALIAS_SEGMENT0_LABEL);
        return -ENOENT;
    }
    LOG_DBG("Found display device %s", DT_ALIAS_SEGMENT0_LABEL);

    display_set_temperature(NULL);

    display_clear_symbols(DISPLAY_SYMBOL_ALL);

    // Default the battery logo to empty
    display_set_battery(0);

    return 0;
}

SYS_INIT(display_setup, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
