#include <stdint.h>
#include <stdio.h>
#include "mesh.h"

#if NET_MESH_VENDOR_SERVER

static void dump(struct gecko_msg_mesh_vendor_model_receive_evt_t *event) {
	printf("from: 0x%04x to 0x%04x: ", event->source_address, event->destination_address);
	printf("vendor_id: 0x%04x, ", event->vendor_id);
	printf("model_id: 0x%04x, ", event->model_id);
	printf("opcode: 0x%02x\r\n", event->opcode);

}

void mesh_vendor_server_register(void) {
	mesh_vm_temp_server_register();
}

void mesh_vendor_client_register(void) {
	mesh_vm_temp_client_register();
}

void mesh_vendor_server_event_handler(struct gecko_cmd_packet *evt) {
	switch(BGLIB_MSG_ID(evt->header)) {
	case gecko_evt_mesh_vendor_model_receive_id: {
		struct gecko_msg_mesh_vendor_model_receive_evt_t *event;
		event = &evt->data.evt_mesh_vendor_model_receive;
		dump(event);
		if (event->model_id == MESH_VM_TEMP_SERVER_MODEL_ID) {
			// handle receive.
		}
		break;
	}
	}
}

#endif /* NET_MESH_VENDOR_SERVER */
