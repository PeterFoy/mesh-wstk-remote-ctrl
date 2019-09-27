#include <stdint.h>
#include <stdio.h>
#include "mesh.h"
#include <mesh_lib.h>

#if NET_MESH_LIGHT_LIGHTNESS_CLIENT

static struct {
	uint16_t value;
	uint32_t timestamp;
} lightness;

void mesh_light_lightness_client_register(void) {
	errorcode_t result = mesh_lib_generic_client_register_handler(
							MESH_LIGHTING_LIGHTNESS_CLIENT_MODEL_ID,
							0,
							mesh_light_lightness_client_server_response_cb);
	if (result != bg_err_success) {
		printf("ERROR: mesh_lib_generic_client_register_handler returns %d(%x)\r\n", result, result);
	}
}

void mesh_light_lightness_client_server_response_cb(uint16_t model_id,
                                              uint16_t element_index,
                                              uint16_t client_addr,
                                              uint16_t server_addr,
                                              const struct mesh_generic_state *current,
                                              const struct mesh_generic_state *target,
                                              uint32_t remaining_ms,
                                              uint8_t response_flags)
{
	net_mesh_transaction_close(MESH_LIGHTING_LIGHTNESS_CLIENT_MODEL_ID);
	printf("Lightness of 0x%04x is %uh\r\n", server_addr, current->lightness.level);
	lightness.value = current->lightness.level;
}


void mesh_light_lightness_client_set(uint16_t server_addr, uint16_t value, bool ack)
{
	struct mesh_generic_request *request;
	request = net_mesh_transaction_new(MESH_LIGHTING_LIGHTNESS_CLIENT_MODEL_ID);
	if (request) {
		request->kind = mesh_lighting_request_lightness_actual;
		request->lightness = value;
		lightness.value = value;
		net_mesh_transaction_set(server_addr, ack);
	}
}

void mesh_light_lightness_client_publish(uint16_t value)
{
	struct mesh_generic_request *request;
	request = net_mesh_transaction_new(MESH_LIGHTING_LIGHTNESS_CLIENT_MODEL_ID);
	if (request) {
		request->kind = mesh_lighting_request_lightness_actual;
		request->lightness = value;
		lightness.value = value;
		net_mesh_transaction_publish();
	} else {
		printf("SKIP transaction\r\n");
	}
}

void mesh_light_lightness_client_get(uint16_t server_addr)
{
	errorcode_t result;
	result = mesh_lib_generic_client_get(MESH_LIGHTING_LIGHTNESS_CLIENT_MODEL_ID,
			0, server_addr, 0, mesh_lighting_request_lightness_actual);
	if (result != bg_err_success) {
		printf("ERROR: %s %d(%x)\r\n", __FUNCTION__, result, result);
	}
}

uint16_t mesh_light_lightness_value(void)
{
	return lightness.value;
}

#endif /* NET_MESH_LIGHT_LIGHTNESS_CLIENT */
