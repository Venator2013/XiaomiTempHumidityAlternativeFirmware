/* main.c - Application main entry point */

#include <stdio.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include "battery.h"
#include "bluetooth.h"
#include "display.h"
#include "sensor.h"

static bool allow_bonding = false;

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
                                                              {0});
static struct gpio_callback button_cb_data;

void button_pressed(struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    LOG_INF("Button pressed at %" PRIu32, k_cycle_get_32());
    allow_bonding = true;
}

void main(void)
{
    int ret;
    bool bluetooth_enabled = false;
    uint32_t loop_count = 0;

    LOG_INF("Hello world!");

    if (!gpio_is_ready_dt(&button))
    {
        LOG_ERR("Didn't find %s device", button.port->name);
        return;
    }
    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret != 0)
    {
        LOG_ERR("Failed to configure pin %d '%s' (Error %d)", button.pin, button.port->name, ret);
        return;
    }

    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0)
    {
        LOG_ERR("Failed to configure interrupt on pin %d '%s' (Error %d)", button.pin, button.port->name, ret);
        return;
    }

    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    ret = bt_enable(NULL);
    if (ret != 0)
    {
        LOG_ERR("Bluetooth init failed (Error %d)", ret);
    }
    else
    {
        bluetooth_ready();
        bluetooth_enabled = true;
    }

    LOG_INF("Press %s on the board", button.port->name);

    struct sensor_value temp, hum;

    display_set_symbols(DISPLAY_SYMBOL_HORIZONTAL_RULE);

    while (1)
    {
        if (bluetooth_enabled)
        {
            if (allow_bonding)
            {
                bluetooth_set_bonding(true);
                allow_bonding = false;
            }

            if (bluetooth_get_bonding() && (loop_count % 2 == 0))
            {
                display_clear_symbols(DISPLAY_SYMBOL_BLUETOOTH);
            }
            else
            {
                display_set_symbols(DISPLAY_SYMBOL_BLUETOOTH);
            }
        }
        int batt_mV = battery_sample();

        if (batt_mV >= 0)
        {
            int batt_pptt = battery_level_pptt(batt_mV, alkaline_level_point) / 100;
            LOG_INF("Battery: %d%% (%d.%03dV)", batt_pptt, batt_mV / 1000, batt_mV % 1000);

            display_set_battery(batt_pptt);
            bluetooth_update_battery(batt_pptt);
        }
        else
        {
            LOG_ERR("Failed to read battery voltage: %d", batt_mV);
        }

        if (update_sensor(&temp, &hum) == 0)
        {
            LOG_INF("Sensor: %d.%d°C, %d.%d%%RH",
                    temp.val1, temp.val2 / 100000,
                    hum.val1, hum.val2 / 100000);

            display_set_symbols(DISPLAY_SYMBOL_CELSIUS | DISPLAY_SYMBOL_HUMIDITY);
            display_set_temperature(&temp);
            display_set_humidity(&hum);

            bluetooth_update_temperature((temp.val1 * 100) + (temp.val2 / 10000));
            bluetooth_update_humidity((hum.val1 * 100) + (hum.val2 / 10000));
        }

        k_msleep(1000);
        loop_count++;
    }
}
