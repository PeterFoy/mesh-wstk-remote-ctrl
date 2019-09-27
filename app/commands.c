/* Board headers */
#include "init_mcu.h"
#include "init_board.h"
#include "init_app.h"
#include "ble-configuration.h"
#include "board_features.h"

/* Bluetooth stack headers */
#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"
#include <gecko_configuration.h>
#include <mesh_sizes.h>

/* Libraries containing default Gecko configuration values */
#include "em_emu.h"
#include "em_cmu.h"
#include <em_gpio.h>

/* Device initialization header */
#include "hal-config.h"

#if defined(HAL_CONFIG)
#include "bsphalconfig.h"
#else
#include "bspconfig.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>

#include "main.h"
#include "commands.h"
#include "net/mesh.h"


static void print_array(const char* name, const uint8 *data, size_t len) {
	uint8_t i;
	if (name)
		ucli_printf("%s: ", name);
	for (i = 0; i < (len - 1); i++) {
		ucli_printf("%02x", data[i]);
	}
	ucli_printf("%02x\r\n", data[i]);
}

static int cmd_prov_info(int argc, char *argv[]) {
	struct gecko_msg_mesh_node_get_uuid_rsp_t* uuid_resp;
	struct gecko_msg_mesh_node_get_element_address_rsp_t* element_resp;

	ucli_printf("provisioned: %s\r\n", net_mesh_provisioned() ? "yes" : "no");

	uuid_resp = gecko_cmd_mesh_node_get_uuid();
	if (uuid_resp->result) {
		ucli_printf("error: 0x%04x\r\n", uuid_resp->result);
		return -1;
	}
	print_array("uuid", uuid_resp->uuid.data, uuid_resp->uuid.len);

	element_resp = gecko_cmd_mesh_node_get_element_address(0);
	if (element_resp->result) {
		ucli_printf("error: 0x%04x\r\n", uuid_resp->result);
		return -1;
	}
	ucli_printf("unicast address: 0x%04x\r\n", element_resp->address);
	return 0;
}

static int cmd_device_reboot(int argc, char *argv[]) {
	ucli_printf("rebooting...\r\n");
	app_reboot(0);
	return 0;
}

static struct {
	aes_key_128 devkey;
	aes_key_128 netkey;
	uint16 netkey_index;
	uint16 address;
	uint32 iv_index;
	uint8 kr_in_progress;
} prov_data;

static void prov_print(void)
{
	print_array("devkey", prov_data.devkey.data, 16);
	print_array("netkey", prov_data.netkey.data, 16);
	ucli_printf("netkey_index: 0x%04x\r\n", prov_data.netkey_index);
//	ucli_printf("iv_index: 0x%08x\r\n", prov_data.iv_index);
	ucli_printf("address: 0x%04x\r\n", prov_data.address);
//	ucli_printf("kr_in_progress: %s\r\n", prov_data.kr_in_progress ? "yes" : "no");
}

static int parse_key(char *arg, uint8_t *key)
{
	int i;
	if (!arg || strlen(arg) != 32)
		return -1;

	for(i = 0; i < 16; i++)
	{
		char byte[2];
		*(uint16_t*)byte = *(uint16_t*)arg;
		*key = strtol(byte, NULL, 16);
		arg += 2;
		key++;
	}
	return 0;
}

static int cmd_prov_reset(int argc, char *argv[]) {
	bool factory = false;
	if (argc > 2 ) {
		ucli_printf("usage: %s [force]\r\n",  argv[0]);
		ucli_printf("%s\r\n", argv[0]);
		return -1;
	}
	factory = net_mesh_provisioned() | (argc == 2 && strcmp("force", argv[1]) == 0);
	if (net_mesh_provisioned()) {
		ucli_printf("info: node already provisioned\r\n");
	}
	if (factory) {
		ucli_printf("performing factory reset\r\n");
		app_factory_reset();
	} else {
		memset(&prov_data, 0, sizeof(prov_data));
		prov_print();
	}
	return 0;
}

static int cmd_prov_devkey(int argc, char *argv[]) {
	if (argc != 2) {
		ucli_printf("usage: %s <key>\r\n",  argv[0]);
		ucli_printf("%s 444a18bc4026e36c4e6e98af8b45d920\r\n", argv[0]);
		return -1;
	}
	if (net_mesh_provisioned()) {
		ucli_printf("error: node already provisioned\r\n");
		return -1;
	}
	if (parse_key(argv[1], prov_data.devkey.data)) {
		ucli_printf("parsing error: '%s'\r\n", argv[1]);
		return -1;
	}
	prov_print();
	return 0;
}

static int cmd_prov_netkey(int argc, char *argv[]) {
	if (argc != 3) {
		ucli_printf("usage: %s <id> <key>\r\n",  argv[0]);
		ucli_printf("%s 0 444a18bc4026e36c4e6e98af8b45d920\r\n", argv[0]);
		return -1;
	}
	if (net_mesh_provisioned()) {
		ucli_printf("error: node already provisioned\r\n");
		return -1;
	}
	if (parse_key(argv[2], prov_data.netkey.data)) {
		ucli_printf("parsing error: '%s'\r\n", argv[2]);
		return -1;
	}

	prov_data.netkey_index = (uint16)strtol(argv[1], NULL, 0);

	prov_print();
	return 0;
}

static int cmd_prov_iv_index(int argc, char *argv[]) {
	if (argc != 2) {
		ucli_printf("usage: %s <iv_index>\r\n",  argv[0]);
		ucli_printf("%s 5\r\n", argv[0]);
		return -1;
	}
	if (net_mesh_provisioned()) {
		ucli_printf("error: node already provisioned\r\n");
		return -1;
	}
	prov_data.iv_index = (uint32)strtol(argv[1], NULL, 0);
	prov_print();
	return 0;
}

static int cmd_prov_address(int argc, char *argv[]) {
	if (argc != 2) {
		ucli_printf("usage: %s <unicast address>\r\n",  argv[0]);
		ucli_printf("%s 0x0100\r\n", argv[0]);
		return -1;
	}
	if (net_mesh_provisioned()) {
		ucli_printf("error: node already provisioned\r\n");
		return -1;
	}
	prov_data.address = (uint16)strtol(argv[1], NULL, 0);
	prov_print();
	return 0;
}

static int cmd_prov_commit(int argc, char *argv[]) {
	struct gecko_msg_mesh_node_set_provisioning_data_rsp_t* rsp;
	if (net_mesh_provisioned()) {
		ucli_printf("error: node already provisioned\r\n");
		return -1;
	}
	if (net_mesh_advertising()) {
		ucli_printf("error: device is unprov beaconing\r\n");
		return -1;
	}
	rsp = gecko_cmd_mesh_node_set_provisioning_data(
			prov_data.devkey,
			prov_data.netkey,
			prov_data.netkey_index,
			prov_data.iv_index,
			prov_data.address,
			prov_data.kr_in_progress
			);
	if (!rsp->result) {
		printf("device is going to reset\r\n");
		app_reboot(1500);
	} else {
		printf("error: 0x%04x\r\n", rsp->result);
		return -1;

	}
	return 0;
}

static int cmd_prov_beacon(int argc, char *argv[]) {
	if (argc != 2) {
		ucli_printf("usage: %s bearers\r\n",  argv[0]);
		return -1;
	}
	if (net_mesh_provisioned()) {
		ucli_printf("error: node already provisioned\r\n");
		return -1;
	}
	if (net_mesh_advertising()) {
		ucli_printf("error: device is already beaconing\r\n");
		return -1;
	}
	uint8_t bearers = (uint8_t)strtol(argv[1], NULL, 0);
	uint16_t rv = net_mesh_advertise(bearers);
	if (rv) {
		ucli_printf("error: 0x%04x\r\n", rv);
		return -1;
	}

	return 0;
}

static int cmd_conf_appkey_add(int argc, char *argv[]) {
	if (argc != 3) {
		ucli_printf("usage: %s <id> <key>\r\n",  argv[0]);
		ucli_printf("%s 0 444a18bc4026e36c4e6e98af8b45d920\r\n", argv[0]);
		return -1;
	}

	aes_key_128 appkey;
	if (parse_key(argv[2], appkey.data)) {
		ucli_printf("parsing error: '%s'\r\n", argv[2]);
		return -1;
	}

	uint16 appkey_id = (uint16)strtol(argv[1], NULL, 0);

	uint16 result = gecko_cmd_mesh_test_add_local_key(1, appkey, appkey_id, 0)->result;
	if (result) {
		printf("error: 0x%04x\r\n", result);
		return -1;
	}
	return 0;
}

static int cmd_conf_appkey_del(int argc, char *argv[]) {
	if (argc != 2) {
		ucli_printf("usage: %s <id>\r\n",  argv[0]);
		ucli_printf("%s 0\r\n", argv[0]);
		return -1;
	}

	uint16 appkey_id = (uint16)strtol(argv[1], NULL, 0);

	uint16 result = gecko_cmd_mesh_test_del_local_key(1, appkey_id)->result;
	if (result) {
		printf("error: 0x%04x\r\n", result);
		return -1;
	}
	return 0;
}

static int cmd_conf_bind(int argc, char *argv[]) {
	if (argc < 3 || argc > 4) {
		ucli_printf("usage: %s <appkey_id> [vendor_id] <model_id>\r\n",  argv[0]);
		ucli_printf("%s 0 0x1003\r\n", argv[0]);
		return -1;
	}
	uint16 appkey_index =(uint16)strtol(argv[1], NULL, 0);
	uint16 model_id = (uint16)strtol(argv[(argc == 3) ? 2 : 3], NULL, 0);
	uint16 vendor_id = (argc == 4) ? (uint16)strtol(argv[2], NULL, 0) : 0xffff;
	uint16 result = gecko_cmd_mesh_test_bind_local_model_app(0, appkey_index, vendor_id, model_id)->result;
	if (result) {
		printf("error: 0x%04x\r\n", result);
		return -1;
	}
	return 0;
}


static int cmd_conf_unbind(int argc, char *argv[]) {
	if (argc < 3 || argc > 4) {
		ucli_printf("usage: %s <appkey_id> [vendor_id] <model_id>\r\n",  argv[0]);
		ucli_printf("%s 0 0x1003\r\n", argv[0]);
		return -1;
	}
	uint16 appkey_index =(uint16)strtol(argv[1], NULL, 0);
	uint16 model_id = (uint16)strtol(argv[(argc == 3) ? 2 : 3], NULL, 0);
	uint16 vendor_id = (argc == 4) ? (uint16)strtol(argv[2], NULL, 0) : 0xffff;
	uint16 result = gecko_cmd_mesh_test_unbind_local_model_app(0, appkey_index, vendor_id, model_id)->result;
	if (result) {
		printf("error: 0x%04x\r\n", result);
		return -1;
	}
	return 0;
}

static int cmd_conf_netkey_list(int argc, char *argv[]) {
	int i;

	/* uint32 count = gecko_cmd_mesh_test_get_key_count(0)->count; */
	struct gecko_msg_mesh_test_get_key_rsp_t *rsp;
	ucli_printf("id\tnetkey\r\n");
	for(i = 0; i < 10; i++) {
		rsp = gecko_cmd_mesh_test_get_key(0, i, 1);
		if (rsp->result) {
			break;
		}
		ucli_printf("%ld\t", rsp->id);
		print_array(NULL, rsp->key.data, 16);
	}
	return 0;
}

static int cmd_conf_appkey_list(int argc, char *argv[]) {
	int i;

	struct gecko_msg_mesh_test_get_key_rsp_t *rsp;
	ucli_printf("id\tnetkey\tappkey\r\n");
	for(i = 0; i < 10; i++) {
		rsp = gecko_cmd_mesh_test_get_key(1, i, 1);
		if (rsp->result) {
			break;
		}
		ucli_printf("%ld\t%ld\t", rsp->id, rsp->network);
		print_array(NULL, rsp->key.data, 16);
	}
	return 0;
}

static int cmd_conf_netkey_update(int argc, char *argv[]) {
	if (argc != 3) {
		ucli_printf("usage: %s <id> <key>\r\n",  argv[0]);
		ucli_printf("%s 0 444a18bc4026e36c4e6e98af8b45d920\r\n", argv[0]);
		return -1;
	}

	aes_key_128 netkey;
	uint16 netkey_index =(uint16)strtol(argv[1], NULL, 0);
	if (parse_key(argv[2], netkey.data)) {
		ucli_printf("parsing error: '%s'\r\n", argv[2]);
		return -1;
	}

	gecko_cmd_mesh_test_update_local_key(0, netkey, netkey_index);

	return 0;
}

static int cmd_conf_appkey_update(int argc, char *argv[]) {
	if (argc != 3) {
		ucli_printf("usage: %s <id> <key>\r\n",  argv[0]);
		ucli_printf("%s 0 444a18bc4026e36c4e6e98af8b45d920\r\n", argv[0]);
		return -1;
	}

	aes_key_128 appkey;
	uint16 appkey_index =(uint16)strtol(argv[1], NULL, 0);
	if (parse_key(argv[2], appkey.data)) {
		ucli_printf("parsing error: '%s'\r\n", argv[2]);
		return -1;
	}

	gecko_cmd_mesh_test_update_local_key(1, appkey, appkey_index);

	return 0;
}

static int cmd_conf_relay_get(int argc, char *argv[]) {
	struct gecko_msg_mesh_test_get_relay_rsp_t *rsp;
	rsp = gecko_cmd_mesh_test_get_relay();
	if (rsp->result) {
		printf("error: 0x%04x\r\n", rsp->result);
		return -1;
	} else {
		ucli_printf("state: %u, retran count: %u, retran interval: %u\r\n",
					rsp->enabled,
					rsp->count,
					rsp->interval);
	}
	return 0;
}

static int cmd_conf_relay_set(int argc, char *argv[]) {
	uint16 count = 0;
	uint16 interval = 0;
	uint8 enabled = 0;
	uint16 result;

	if (argc == 2) {
		if (!strcmp("off", argv[1])) {
			enabled = 0;
			goto cmd_conf_relay_set_execute;
		}
	} else if (argc == 4 && !strcmp("on", argv[1])) {
		enabled = 1;
		count =(uint16)strtol(argv[2], NULL, 0);
		interval =(uint16)strtol(argv[3], NULL, 0);
		goto cmd_conf_relay_set_execute;
	}
	ucli_printf("usage: %s on|off [count interval_steps]\r\n",  argv[0]);
	ucli_printf("%s off\r\n", argv[0]);
	ucli_printf("%s on 3 10\r\n", argv[0]);
	return -1;
cmd_conf_relay_set_execute:
	result = gecko_cmd_mesh_test_set_relay(enabled, count, interval)->result;
	if (result) {
		printf("error: 0x%04x\r\n", result);
		return -1;
	}
	return 0;
}

static int cmd_conf_pub_set(int argc, char *argv[]) {
	if (argc < 4 || argc > 5) {
		ucli_printf("usage: %s <appkey_id> [vendor_id] <model_id> <pub_address>\r\n",  argv[0]);
		ucli_printf("%s 0 0x1003 0xc000\r\n", argv[0]);
		return -1;
	}

	uint16 appkey_index =(uint16)strtol(argv[1], NULL, 0);
	uint16 model_id = (uint16)strtol(argv[argc == 4 ? 2 : 3], NULL, 0);
	uint16 pub_address = (uint16)strtol(argv[argc == 4 ? 3 : 4], NULL, 0);
	uint16 vendor_id = (argc == 5) ? (uint16)strtol(argv[2], NULL, 0) : 0xffff;
	uint16 result = gecko_cmd_mesh_test_set_local_model_pub(0, appkey_index, vendor_id, model_id, pub_address, 16, 0, 0, 0)->result;
	if (result) {
		printf("error: 0x%04x\r\n", result);
		return -1;
	}
	return 0;
}

static int cmd_conf_pub_list(int argc, char *argv[]) {
	if (argc < 2 || argc > 3) {
		ucli_printf("usage: %s [vendor_id] <model_id>\r\n",  argv[0]);
		ucli_printf("%s 0x1003\r\n", argv[0]);
		return -1;
	}

	uint16 model_id = (uint16)strtol(argv[argc == 2 ? 1 : 2], NULL, 0);
	uint16 vendor_id = (argc == 3) ? (uint16)strtol(argv[1], NULL, 0) : 0xffff;

	struct gecko_msg_mesh_test_get_local_model_pub_rsp_t *rsp;
	rsp = gecko_cmd_mesh_test_get_local_model_pub(0, vendor_id, model_id);
	if (rsp->result) {
		printf("error: 0x%04x\r\n", rsp->result);
		return -1;
	}
	ucli_printf("%ld\t0x%04x\r\n", rsp->appkey_index, rsp->pub_address);
	return 0;
}

static int cmd_conf_sub_add(int argc, char *argv[]) {
	if (argc < 3 || argc > 4) {
		ucli_printf("usage: %s [vendor_id] <model_id> <sub_address>\r\n",  argv[0]);
		ucli_printf("%s 0x1003 0xc000\r\n", argv[0]);
		return -1;
	}
	uint16 vendor_id = (argc == 4) ? (uint16)strtol(argv[1], NULL, 0) : 0xffff;
	uint16 model_id = (uint16)strtol(argv[argc == 3 ? 1 : 2], NULL, 0);
	uint16 sub_address = (uint16)strtol(argv[argc == 3 ? 2 : 3], NULL, 0);
	uint16 result = gecko_cmd_mesh_test_add_local_model_sub(0, vendor_id, model_id, sub_address)->result;
	if (result) {
		printf("error: 0x%04x\r\n", result);
		return -1;
	}
	return 0;
}

static int cmd_conf_sub_del(int argc, char *argv[]) {
	if (argc < 3 || argc > 4) {
		ucli_printf("usage: %s [vendor_id] <model_id> <sub_address>\r\n",  argv[0]);
		ucli_printf("%s 0x1003 0xc000\r\n", argv[0]);
		return -1;
	}
	uint16 vendor_id = (argc == 4) ? (uint16)strtol(argv[1], NULL, 0) : 0xffff;
	uint16 model_id = (uint16)strtol(argv[argc == 3 ? 1 : 2], NULL, 0);
	uint16 sub_address = (uint16)strtol(argv[argc == 3 ? 2 : 3], NULL, 0);
	uint16 result = gecko_cmd_mesh_test_del_local_model_sub(0, vendor_id, model_id, sub_address)->result;
	if (result) {
		printf("error: 0x%04x\r\n", result);
		return -1;
	}
	return 0;
}

static int cmd_conf_sub_list(int argc, char *argv[]) {
	if (argc < 2 || argc > 3) {
		ucli_printf("usage: %s [vendor_id] <model_id>\r\n",  argv[0]);
		ucli_printf("%s 0x1003\r\n", argv[0]);
		return -1;
	}
	uint16 vendor_id = (argc == 3) ? (uint16)strtol(argv[1], NULL, 0) : 0xffff;
	uint16 model_id = (uint16)strtol(argv[argc == 2 ? 1 : 2], NULL, 0);
	struct gecko_msg_mesh_test_get_local_model_sub_rsp_t *rsp;
	rsp = gecko_cmd_mesh_test_get_local_model_sub(0, vendor_id, model_id);
	if (rsp->result) {
		printf("error: 0x%04x\r\n", rsp->result);
		return -1;
	}
	int i;
	for(i = 0; i < rsp->addresses.len; i += 2) {
		uint16_t address = rsp->addresses.data[i] | (rsp->addresses.data[i + 1] << 8);
		ucli_printf("[%d] 0x%04x\r\n", (i >> 1), address);
	}
	return 0;
}

#if NET_MESH_GENERIC_ONOFF_CLIENT
static int cmd_client_onoff_set(int argc, char *argv[]) {
	if (argc != 3) {
		goto cmd_client_onoff_set_error;
	}

	uint16 target_addr = (uint16)strtol(argv[1], NULL, 0);
	bool value;
	if (!strcmp("on", argv[2])) {
		value = true;
	} else if (!strcmp("off", argv[2])) {
		value = false;
	} else {
		goto cmd_client_onoff_set_error;
	}

	mesh_generic_onoff_client_set(target_addr, value, false);
	return 0;

cmd_client_onoff_set_error:
	ucli_printf("usage: %s <target> on|off\r\n",  argv[0]);
	ucli_printf("%s 0x0001 on\r\n", argv[0]);
	return -1;
}

static int cmd_client_onoff_get(int argc, char *argv[]) {
	if (argc != 2) {
		ucli_printf("usage: %s <target>\r\n",  argv[0]);
		ucli_printf("%s 0x0001\r\n", argv[0]);
		return -1;
	}

	uint16 target_addr = (uint16)strtol(argv[1], NULL, 0);
	mesh_generic_onoff_client_get(target_addr);

	return 0;
}

static int cmd_client_onoff_publish(int argc, char *argv[]) {
	if (argc != 2) {
		goto cmd_client_onoff_publish_error;
	}

	bool value;
	if (!strcmp("on", argv[1])) {
		value = true;
	} else if (!strcmp("off", argv[1])) {
		value = false;
	} else {
		goto cmd_client_onoff_publish_error;
	}
	mesh_generic_onoff_client_publish(value);
	return 0;

cmd_client_onoff_publish_error:
	ucli_printf("usage: %s on|off\r\n",  argv[0]);
	ucli_printf("%s on\r\n", argv[0]);
	return -1;
}
#endif /* NET_MESH_GENERIC_ONOFF_CLIENT */

#if NET_MESH_GENERIC_ONOFF_SERVER
static int cmd_server_onoff_set(int argc, char *argv[]) {
	if (argc != 2) {
		goto cmd_server_onoff_set_error;
	}
	bool value;
	if (!strcmp("on", argv[1])) {
		value = true;
	} else if (!strcmp("off", argv[1])) {
		value = false;
	} else {
		goto cmd_server_onoff_set_error;
	}
	mesh_generic_onoff_server_set(value);

	return 0;
cmd_server_onoff_set_error:
	ucli_printf("usage: %s on|off\r\n",  argv[0]);
	ucli_printf("%s on\r\n", argv[0]);
	return -1;
}
#endif /* NET_MESH_GENERIC_ONOFF_SERVER */

#if NET_MESH_GENERIC_LEVEL_CLIENT
static int cmd_client_level_set(int argc, char *argv[]) {
	if (argc != 3) {
		ucli_printf("usage: %s <target> <value>\r\n",  argv[0]);
		ucli_printf("%s 0x0001 55\r\n", argv[0]);
		return -1;
	}

	uint16 target_addr = (uint16)strtol(argv[1], NULL, 0);
	int16 value = (int16)strtol(argv[2], NULL, 0);

	mesh_generic_level_client_set(target_addr, value, false);

	return 0;
}

static int cmd_client_level_get(int argc, char *argv[]) {
	if (argc != 2) {
		ucli_printf("usage: %s <target>\r\n",  argv[0]);
		ucli_printf("%s 0x0001\r\n", argv[0]);
		return -1;
	}

	uint16 target_addr = (uint16)strtol(argv[1], NULL, 0);
	mesh_generic_level_client_get(target_addr);

	return 0;
}

static int cmd_client_level_publish(int argc, char *argv[]) {
	if (argc != 2) {
		ucli_printf("usage: %s <value>\r\n",  argv[0]);
		ucli_printf("%s 0x0001\r\n", argv[0]);
		return -1;
	}

	uint16 target_addr = (uint16)strtol(argv[1], NULL, 0);
	mesh_generic_level_client_publish(target_addr, false);
	return 0;
}
#endif /* NET_MESH_GENERIC_LEVEL_CLIENT */

#if NET_MESH_GENERIC_LEVEL_SERVER
static int cmd_server_level_set(int argc, char *argv[]) {
	if (argc != 2) {
		ucli_printf("usage: %s <value>\r\n",  argv[0]);
		ucli_printf("%s 0x0001\r\n", argv[0]);
		return -1;
	}
	int16 server_value = (int16)strtol(argv[1], NULL, 0);
	mesh_generic_level_server_set(server_value);
	return 0;
}
#endif

#if NET_MESH_VENDOR_SERVER
static int cmd_server_vm_temp_set(int argc, char *argv[]) {
	if (argc != 2) {
		ucli_printf("usage: %s <temp>\r\n",  argv[0]);
		ucli_printf("%s 23.41\r\n", argv[0]);
		return -1;
	}
	float temp = strtof(argv[1], NULL);
	mesh_vm_temp_set(temp);
	return 0;
}
#endif

#if 0
static int cmd_conf_period(int argc, char *argv[]) {
	if (argc != 2) {
		ucli_printf("usage: %s <seconds>\r\n",  argv[0]);
		ucli_printf("%s 5\r\n", argv[0]);
		return -1;
	}
	uint16_t sec = strtol(argv[1], NULL, 0);
	app_sensor_period(sec);
	return 0;
}
#endif

#if CONFIG_MAX_CMD_LENGTH < 24
#error "confiugre ucli max cmd length"
#endif

const ucli_cmd_t ucli_commands[] = {
		{ "reboot", cmd_device_reboot },

		{ "prov-info", cmd_prov_info },
		{ "prov-reset", cmd_prov_reset },
		{ "prov-devkey", cmd_prov_devkey },
		{ "prov-netkey", cmd_prov_netkey },
		{ "prov-iv-index", cmd_prov_iv_index },
		{ "prov-address", cmd_prov_address },
		{ "prov-commit", cmd_prov_commit },
		{ "prov-beacon", cmd_prov_beacon },

		{ "conf-appkey-add", cmd_conf_appkey_add },
		{ "conf-appkey-del", cmd_conf_appkey_del },
		{ "conf-bind", cmd_conf_bind },
		{ "conf-unbind", cmd_conf_unbind },
		{ "conf-netkey-list", cmd_conf_netkey_list },
		{ "conf-appkey-list", cmd_conf_appkey_list },
		{ "conf-netkey-update", cmd_conf_netkey_update },
		{ "conf-appkey-update", cmd_conf_appkey_update },
		{ "conf-relay-get", cmd_conf_relay_get },
		{ "conf-relay-set", cmd_conf_relay_set },

		{ "conf-pub-set", cmd_conf_pub_set },
		{ "conf-pub-list", cmd_conf_pub_list },

		{ "conf-sub-add", cmd_conf_sub_add },
		{ "conf-sub-del", cmd_conf_sub_del },
		{ "conf-sub-list", cmd_conf_sub_list },

#if NET_MESH_GENERIC_ONOFF_CLIENT
		{ "c-onoff-set", 	 cmd_client_onoff_set },
		{ "c-onoff-publish", cmd_client_onoff_publish },
		{ "c-onoff-get", 	 cmd_client_onoff_get },
#endif
#if NET_MESH_GENERIC_ONOFF_SERVER
		{ "s-onoff-set", 	 cmd_server_onoff_set },
#endif
#if NET_MESH_GENERIC_LEVEL_CLIENT
		{ "c-level-set", 	 cmd_client_level_set },
		{ "c-level-publish", cmd_client_level_publish },
		{ "c-level-get", 	 cmd_client_level_get },
#endif
#if NET_MESH_GENERIC_LEVEL_SERVER
		{ "s-level-set", 	 cmd_server_level_set },
#endif
#if NET_MESH_VENDOR_SERVER
		{ "s-vm-temp-set", 	 cmd_server_vm_temp_set},
#endif

//		{ "conf-period", cmd_conf_period },
};

const unsigned int ucli_commands_num = sizeof(ucli_commands)
		/ sizeof(ucli_cmd_t);
