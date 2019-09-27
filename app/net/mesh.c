#include <stdlib.h>
#include <stdio.h>
#include "mesh.h"
#include <mesh_lib.h>

static struct {
	bool provisioned;
	bool advertising;
	bool has_friend;
	set_timer_f set_timer;
#if NET_MESH_LPN
	set_timer_f set_lpn_timer;
#endif
	struct {
		uint8_t id;
		uint8_t retransmits;
		uint16_t model;
		struct mesh_generic_request request;
		bool ack;
		uint16_t dest_addr;
	} transaction;
} net_mesh;

void net_mesh_init(bool provisioned, set_timer_f setter) {
	mesh_lib_init(malloc, free, MESH_CFG_MAX_MODELS);
	net_mesh.transaction.id = 1;
	net_mesh.provisioned = provisioned;
	net_mesh.set_timer = setter;


#if	NET_MESH_GENERIC_ONOFF_SERVER || NET_MESH_GENERIC_LEVEL_SERVER
	gecko_cmd_mesh_generic_server_init();
#endif
#if NET_MESH_GENERIC_ONOFF_SERVER
	mesh_generic_onoff_server_register();
#endif
#if NET_MESH_GENERIC_LEVEL_SERVER
	mesh_generic_level_server_register();
#endif
#if NET_MESH_SENSOR_SERVER
	mesh_sensor_server_register();
#endif

#if	NET_MESH_GENERIC_ONOFF_CLIENT || NET_MESH_GENERIC_LEVEL_CLIENT
	gecko_cmd_mesh_generic_client_init();
#endif
#if NET_MESH_GENERIC_ONOFF_CLIENT
	mesh_generic_onoff_client_register();
#endif
#if NET_MESH_GENERIC_LEVEL_CLIENT
	mesh_generic_level_client_register();
#endif
#if NET_MESH_LIGHT_LIGHTNESS_CLIENT
	mesh_light_lightness_client_register();
#endif
}

int16_t net_mesh_advertise(uint8_t bearers) {
	net_mesh.advertising = true;
	return gecko_cmd_mesh_node_start_unprov_beaconing(bearers)->result;
}

bool net_mesh_advertising(void) {
	return net_mesh.advertising;
}

bool net_mesh_provisioned(void) {
	return net_mesh.provisioned;
}

void net_mesh_reset(void) {
	gecko_cmd_flash_ps_erase_all();
}

struct mesh_generic_request *net_mesh_transaction_new(uint16_t model) {
	if (net_mesh.transaction.model) {
		return NULL;
	}
	net_mesh.transaction.id++;
	/* move forward towards 0 */
	if (!net_mesh.transaction.id) {
		net_mesh.transaction.id++;
	}
	net_mesh.transaction.retransmits = 0;
	net_mesh.transaction.model = model;
	net_mesh.transaction.ack = false;
	net_mesh.transaction.dest_addr = 0;

	printf(">T(%d) - new 0x%04x\r\n", net_mesh.transaction.id, model);
	return &net_mesh.transaction.request;
}

static bool set(bool ack) {
	errorcode_t result = mesh_lib_generic_client_set(
			net_mesh.transaction.model,
			0,
			net_mesh.transaction.dest_addr,
			0,
			net_mesh.transaction.id,
			&net_mesh.transaction.request,
			0,
			0,
			ack ? 0x01 : 0x00);

	if (result != bg_err_success) {
		printf("ERROR: %s %d(%x)\r\n", __FUNCTION__, result, result);
	} else {
		printf(">T(%d) - set\r\n", net_mesh.transaction.id);
	}
	return (result == bg_err_success);
}

static bool publish() {
	errorcode_t result = mesh_lib_generic_client_publish(
			net_mesh.transaction.model,
			0,
			net_mesh.transaction.id,
			&net_mesh.transaction.request,
			0,
			0,
			0);
	if (result != bg_err_success) {
		printf("ERROR: %s %d(%x)\r\n", __FUNCTION__, result, result);
	} else {
		printf(">T(%d) - publish\r\n", net_mesh.transaction.id);
	}
	return (result == bg_err_success);
}

void net_mesh_transaction_set(uint16_t server_addr, bool ack) {
	net_mesh.transaction.ack = ack;
	net_mesh.transaction.dest_addr = server_addr;
	if (set(ack)) {
		net_mesh.set_timer(NET_MESH_TRANSACTION_TIMEOUT_MS);
	} else {
		net_mesh_transaction_close(net_mesh.transaction.model);
	}
}

void net_mesh_transaction_publish() {
	if (publish()) {
		net_mesh.set_timer(NET_MESH_TRANSACTION_TIMEOUT_MS);
	} else {
		net_mesh_transaction_close(net_mesh.transaction.model);
	}
}

void net_mesh_transaction_close(uint16_t model) {
	if (net_mesh.transaction.model == model) {
		net_mesh.transaction.model = 0;
		net_mesh.set_timer(0);
		printf(">T(%d) closed\r\n", net_mesh.transaction.id);
	} else if (net_mesh.transaction.model){
		printf("terminating model 0x%04x transaction by 0x%04x\r\n", net_mesh.transaction.model, model);
	}
}

void net_mesh_transaction_timer_expired(void) {
	if (net_mesh.transaction.model) {
		bool ack = (++net_mesh.transaction.retransmits < NET_MESH_TRANSACTION_RETRIES);
		bool result;
		if (net_mesh.transaction.dest_addr) {
			result = set(ack);
		} else {
			result = publish();
		}
		if (!ack || !result)
			net_mesh_transaction_close(net_mesh.transaction.model);
		else
			net_mesh.set_timer(NET_MESH_TRANSACTION_TIMEOUT_MS);

	} else {
		net_mesh_transaction_close(net_mesh.transaction.model);
	}
}

#if NET_MESH_LPN
void net_mesh_lpn_init(set_timer_f setter) {
	net_mesh.set_lpn_timer = setter;

	uint16_t result = gecko_cmd_mesh_lpn_init()->result;
	if (result != bg_err_success) {
		printf("mesh_lpn_init (0x%x)\r\n", result);
		return;
	}
	result = gecko_cmd_mesh_lpn_configure(NET_MESH_LPN_POLL_QUEUE_LEN, NET_MESH_LPN_POLL_MS)->result;
	if (result != bg_err_success) {
		printf("mesh_lpn_configure (0x%x)\r\n", result);
		return;
	}
	result = gecko_cmd_mesh_lpn_establish_friendship(0)->result;
	if (result != bg_err_success) {
		printf("mesh_lpn_establish_friendship (0x%x)\r\n", result);
		return;
	}
}

void net_mesh_lpn_poll(void) {
	uint16_t result = gecko_cmd_mesh_lpn_poll()->result;
	if (result != bg_err_success) {
		printf("mesh_lpn_poll (0x%x)\r\n", result);
	}
}

void net_mesh_lpn_deinit(void) {
	net_mesh.set_lpn_timer(0);

	// Terminate friendship if exist
	uint16_t result = gecko_cmd_mesh_lpn_terminate_friendship()->result;
	if (result) {
		printf("mesh_lpn_terminate_friendship (0x%x)\r\n", result);
	}
	// turn off lpn feature
	result = gecko_cmd_mesh_lpn_deinit()->result;
	if (result) {
	printf("mesh_lpn_deinit (0x%x)\r\n", result);
	}
}

void net_mesh_lpn_timer_expired(void) {
	uint16_t result = gecko_cmd_mesh_lpn_establish_friendship(0)->result;
	if (result != bg_err_success) {
		printf("mesh_lpn_establish_friendship (0x%x)\r\n", result);
		net_mesh.set_lpn_timer(NET_MESH_LPN_FRIEND_FIND_RETRY_MS);
		return;
	}
}

static void mesh_lpn_event_handler(struct gecko_cmd_packet *evt) {
	switch(BGLIB_MSG_ID(evt->header)) {
		case gecko_evt_mesh_lpn_friendship_established_id:
			net_mesh.has_friend = true;
			net_mesh.set_lpn_timer(0);
			printf("mesh_lpn_friendship_established\r\n");
			break;
		case gecko_evt_mesh_lpn_friendship_failed_id:
			printf("mesh_lpn_friendship_failed (0x%04x)\r\n",
					evt->data.evt_mesh_lpn_friendship_failed.reason);
			net_mesh.set_lpn_timer(NET_MESH_LPN_FRIEND_FIND_RETRY_MS);
			break;
		case gecko_evt_mesh_lpn_friendship_terminated_id:
			printf("mesh_lpn_friendship_terminated (0x%04x)\r\n",
					evt->data.evt_mesh_lpn_friendship_terminated.reason);
			net_mesh.has_friend = false;
			uint16_t result = gecko_cmd_mesh_lpn_establish_friendship(0)->result;
			if (result != bg_err_success) {
				printf("mesh_lpn_establish_friendship (0x%x)\r\n", result);
				return;
			}
			break;
	}
}
#endif

void net_mesh_event_handler(struct gecko_cmd_packet *evt) {
#if NET_MESH_GENERIC_ONOFF_SERVER || NET_MESH_GENERIC_LEVEL_SERVER
	mesh_lib_generic_server_event_handler(evt);
#endif
#if NET_MESH_SENSOR_SERVER
	mesh_sensor_server_event_handler(evt);
#endif
#if NET_MESH_VENDOR_SERVER
	mesh_vendor_server_event_handler(evt);
#endif


#if NET_MESH_GENERIC_ONOFF_CLIENT || NET_MESH_GENERIC_LEVEL_CLIENT
	mesh_lib_generic_client_event_handler(evt);
#endif
#if NET_MESH_LPN
	mesh_lpn_event_handler(evt);
#endif
}
