#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/uuid.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bluetooth, LOG_LEVEL_INF);

/* ESS error definitions */
#define ESS_ERR_WRITE_REJECT 0x80
#define ESS_ERR_COND_NOT_SUPP 0x81

/* ESS Trigger Setting conditions */
#define ESS_TRIGGER_INACTIVE 0x00
#define ESS_FIXED_TIME_INTERVAL 0x01
#define ESS_NO_LESS_THAN_SPECIFIED_TIME 0x02
#define ESS_VALUE_CHANGED 0x03
#define ESS_LESS_THAN_REF_VALUE 0x04
#define ESS_LESS_OR_EQUAL_TO_REF_VALUE 0x05
#define ESS_GREATER_THAN_REF_VALUE 0x06
#define ESS_GREATER_OR_EQUAL_TO_REF_VALUE 0x07
#define ESS_EQUAL_TO_REF_VALUE 0x08
#define ESS_NOT_EQUAL_TO_REF_VALUE 0x09

#if CONFIG_BT_SHELL
extern
#endif
	struct bt_conn *default_conn;

static bool allow_bonding = false;

static const struct bt_data bluettoth_advertise_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
				  0x1a, 0x18,  /* Environmental Sensing Service */
				  0x0a, 0x18,  /* Device Information Service */
				  0x0f, 0x18), /* Battery Service */
};

static void bluetooth_connected(struct bt_conn *conn, uint8_t err)
{
	if (err)
	{
		LOG_WRN("Connection failed (err 0x%02x)", err);
	}
	else
	{
		default_conn = bt_conn_ref(conn);
		LOG_INF("Bluetooth connected");
	}

	if (bt_conn_set_security(conn, BT_SECURITY_L3))
	{
		printk("Failed to set security\n");
	}
}

static void bluetooth_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason 0x%02x)", reason);

	if (default_conn)
	{
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}

static struct bt_conn_cb bluetooth_connection_callbacks = {
	.connected = bluetooth_connected,
	.disconnected = bluetooth_disconnected,
};

static void auth_confirm(struct bt_conn *conn)
{
	if (allow_bonding)
	{
		bt_conn_auth_pairing_confirm(conn);
	}
	else
	{
		bt_conn_auth_cancel(conn);
	}
	allow_bonding = false;
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_WRN("Pairing cancelled: %s", addr);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	LOG_WRN("Pairing Failed (%d)", reason);
}

static struct bt_conn_auth_cb bluetooth_auth_cb_display = {
	.cancel = auth_cancel,
	.pairing_confirm = auth_confirm,
	.pairing_failed = pairing_failed,
	.passkey_display = NULL,
	.passkey_entry = NULL,
	.passkey_confirm = NULL,
};

void bluetooth_ready()
{
	int ret;

	ret = bt_le_adv_start(BT_LE_ADV_CONN_NAME, bluettoth_advertise_data, ARRAY_SIZE(bluettoth_advertise_data), NULL, 0);
	if (ret < 0)
	{
		LOG_ERR("Advertising failed to start (%d)", ret);
		return;
	}

	LOG_DBG("Advertising successfully started");

	bt_conn_cb_register(&bluetooth_connection_callbacks);
	bt_conn_auth_cb_register(&bluetooth_auth_cb_display);

	LOG_DBG("Initialized");
}

void bluetooth_update_battery(uint8_t level)
{
#if CONFIG_BT_GATT_BAS
	bt_gatt_bas_set_battery_level(level);
#endif
}

void bluetooth_set_bonding(bool allow)
{
	allow_bonding = allow;
}

bool bluetooth_get_bonding()
{
	return allow_bonding;
}
