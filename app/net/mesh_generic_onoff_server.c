#include <stdint.h>
#include <stdio.h>
#include "mesh.h"
#include <mesh_generic_model_capi_types.h>
#include <mesh_lib.h>

#if NET_MESH_GENERIC_ONOFF_SERVER

static bool g_current_onoff = 0;

static inline const char* stronoff(int state) {
	return (state ? "ON" : "OFF");
}

void mesh_generic_onoff_server_register(void) {
	errorcode_t result = mesh_lib_generic_server_register_handler(
							MESH_GENERIC_ON_OFF_SERVER_MODEL_ID,
							0,
							mesh_generic_onoff_server_client_request_cb,
							mesh_generic_onoff_server_change_cb);
	if (result != bg_err_success) {
		printf("ERROR: mesh_lib_generic_server_register_handler returns %d(%x)\r\n", result, result);
	}
}

void mesh_generic_onoff_server_client_request_cb(uint16_t model_id,
                                             uint16_t element_index,
                                             uint16_t client_addr,
                                             uint16_t server_addr,
                                             uint16_t appkey_index,
                                             const struct mesh_generic_request *req,
                                             uint32_t transition_ms,
                                             uint16_t delay_ms,
                                             uint8_t request_flags)
{
	if (req->kind != mesh_generic_request_on_off) {
		printf("ignored request\r\n");
		return;
	}

	printf("\r\n0x%04x requests (0x%02x): %s -> %s\r\n", request_flags, client_addr, stronoff(g_current_onoff), stronoff(req->on_off));

	struct mesh_generic_state target;
	target.kind = mesh_generic_state_on_off;
	target.on_off.on = req->on_off;


	if (request_flags & MESH_REQUEST_FLAG_RESPONSE_REQUIRED) {
		errorcode_t result = mesh_lib_generic_server_response(MESH_GENERIC_ON_OFF_SERVER_MODEL_ID,
				  element_index,
				  client_addr,
				  appkey_index,
				  &target, /* target became our new current */
				  NULL,
				  0,
				  0x00);
		if (result != bg_err_success) {
			printf("ack send error: %d\n", result);
		}
	} else {
		mesh_generic_onoff_server_set(target.on_off.on);
	}
}

void mesh_generic_onoff_server_change_cb(uint16_t model_id,
                                     uint16_t element_index,
                                     const struct mesh_generic_state *current,
                                     const struct mesh_generic_state *target,
                                     uint32_t remaining_ms)
{
	/* actual change */
	printf("On/Off value is: %s\r\n", stronoff(current->on_off.on));
	g_current_onoff = current->on_off.on;

	// TODO: create abstraction and move it to app logic
	BOARD_rgbledPowerEnable(current->on_off.on);
	BOARD_rgbledEnable(current->on_off.on, 0x0F);
	BOARD_rgbledSetColor(255, 255, 255);

	errorcode_t result = mesh_lib_generic_server_publish(MESH_GENERIC_ON_OFF_SERVER_MODEL_ID, 0, mesh_generic_state_on_off);
	if (result != bg_err_success) {
		printf("ERROR: %s %d(%x)\r\n", __FUNCTION__, result, result);
	}
}

void mesh_generic_onoff_server_set(bool new_value)
{
	struct mesh_generic_state target;

	target.kind = mesh_generic_state_on_off;
	target.on_off.on = new_value;

	errorcode_t result = mesh_lib_generic_server_update(MESH_GENERIC_ON_OFF_SERVER_MODEL_ID,
			0, &target, NULL, 0);
	if (result != bg_err_success) {
		printf("ERROR: %s %d(%x)\r\n", __FUNCTION__, result, result);
	}
}

#endif /* NET_MESH_GENERIC_ONOFF_SERVER */
