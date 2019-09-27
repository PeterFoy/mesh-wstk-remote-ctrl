#include <stdint.h>
#include <stdio.h>
#include "mesh.h"
#include <mesh_generic_model_capi_types.h>
#include <mesh_lib.h>

#if NET_MESH_GENERIC_ONOFF_CLIENT

static inline const char* stronoff(int state) {
	return (state ? "ON" : "OFF");
}

void mesh_generic_onoff_client_register(void)
{
	errorcode_t result = mesh_lib_generic_client_register_handler(
							MESH_GENERIC_ON_OFF_CLIENT_MODEL_ID,
							0,
							mesh_generic_onoff_client_server_response_cb);
	if (result != bg_err_success) {
		printf("ERROR: mesh_lib_generic_client_register_handler returns %d(%x)\r\n", result, result);
	}
}

void mesh_generic_onoff_client_server_response_cb(uint16_t model_id,
                                              uint16_t element_index,
                                              uint16_t client_addr,
                                              uint16_t server_addr,
                                              const struct mesh_generic_state *current,
                                              const struct mesh_generic_state *target,
                                              uint32_t remaining_ms,
                                              uint8_t response_flags)
{
	net_mesh_transaction_close(MESH_GENERIC_ON_OFF_CLIENT_MODEL_ID);
	printf("onoff: 0x%04x is %s\r\n", server_addr, stronoff(current->on_off.on));
}


void mesh_generic_onoff_client_set(uint16_t server_addr, bool value, bool ack)
{
	struct mesh_generic_request *request;
	request = net_mesh_transaction_new(MESH_GENERIC_ON_OFF_CLIENT_MODEL_ID);
	if (request) {
		request->kind = mesh_generic_request_on_off;
		request->on_off = (uint8_t)value;
		net_mesh_transaction_set(server_addr, ack);
	}
}

void mesh_generic_onoff_client_publish(bool value)
{
	struct mesh_generic_request *request;
	request = net_mesh_transaction_new(MESH_GENERIC_ON_OFF_CLIENT_MODEL_ID);
	if (request) {
		request->kind = mesh_generic_request_on_off;
		request->on_off = (uint8_t)value;
		net_mesh_transaction_publish();
	}
}

void mesh_generic_onoff_client_get(uint16_t server_addr)
{
	errorcode_t result = mesh_lib_generic_client_get(MESH_GENERIC_ON_OFF_CLIENT_MODEL_ID,
			0, server_addr, 0, mesh_generic_state_on_off);
	if (result != bg_err_success) {
		printf("ERROR: %s %d(%x)\r\n", __FUNCTION__, result, result);
	}
}

#endif /* NET_MESH_GENERIC_ONOFF_CLIENT */
