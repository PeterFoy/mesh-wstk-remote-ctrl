#include <stdint.h>
#include <stdio.h>
#include "mesh.h"
#include <mesh_generic_model_capi_types.h>
#include <mesh_lib.h>

#if NET_MESH_GENERIC_LEVEL_SERVER

static int16_t g_current_level = 0;

void mesh_generic_level_server_register(void) {
	errorcode_t result = mesh_lib_generic_server_register_handler(
							MESH_GENERIC_LEVEL_SERVER_MODEL_ID,
							0,
							mesh_generic_level_server_client_request_cb,
							mesh_generic_level_server_change_cb);
	if (result != bg_err_success) {
		printf("ERROR: mesh_lib_generic_server_register_handler returns %d(%x)\r\n", result, result);
	}
}

void mesh_generic_level_server_client_request_cb(uint16_t model_id,
                                             uint16_t element_index,
                                             uint16_t client_addr,
                                             uint16_t server_addr,
                                             uint16_t appkey_index,
                                             const struct mesh_generic_request *req,
                                             uint32_t transition_ms,
                                             uint16_t delay_ms,
                                             uint8_t request_flags)
{
	if (req->kind != mesh_generic_request_level) {
		printf("ignored request\r\n");
		return;
	}

	printf("\r\n0x%04x requests: %d -> %d\r\n", client_addr, g_current_level, req->level);

	struct mesh_generic_state target;
	target.kind = mesh_generic_state_level;
	target.level.level = req->level;


	if (request_flags & MESH_REQUEST_FLAG_RESPONSE_REQUIRED) {
		printf("Send ACK\r\n");
		mesh_lib_generic_server_response(MESH_GENERIC_LEVEL_SERVER_MODEL_ID,
											  element_index,
											  client_addr,
											  appkey_index,
											  &target, /* target became our new current */
											  NULL,
											  0,
											  0x00);
	} else {
		printf("ACK Not needed\r\n");
		mesh_generic_level_server_set(target.level.level);
	}
}

void mesh_generic_level_server_change_cb(uint16_t model_id,
                                     uint16_t element_index,
                                     const struct mesh_generic_state *current,
                                     const struct mesh_generic_state *target,
                                     uint32_t remaining_ms)
{
	/* actual change */
	printf("Measured value is: %d\r\n", current->level.level);
	g_current_level = current->level.level;
	errorcode_t result = mesh_lib_generic_server_publish(MESH_GENERIC_LEVEL_SERVER_MODEL_ID, 0, mesh_generic_state_level);
	if (result != bg_err_success) {
		printf("ERROR: %s %d(%x)\r\n", __FUNCTION__, result, result);
	}
}

void mesh_generic_level_server_set(int16_t new_value)
{
	struct mesh_generic_state target;

	target.kind = mesh_generic_state_level;
	target.level.level = new_value;

	errorcode_t result = mesh_lib_generic_server_update(MESH_GENERIC_LEVEL_SERVER_MODEL_ID,
			0, &target, NULL, 0);
	if (result != bg_err_success) {
		printf("ERROR: %s %d(%x)\r\n", __FUNCTION__, result, result);
	}
}

#endif /* NET_MESH_GENERIC_LEVEL_SERVER */
