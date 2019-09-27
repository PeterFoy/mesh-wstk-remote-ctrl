#include <stdint.h>
#include <stdio.h>
#include "mesh.h"

#if NET_MESH_VENDOR_SERVER

void mesh_vm_temp_server_register(void) {
	uint16 result;
	result = gecko_cmd_mesh_vendor_model_init(0,
			MESH_VM_VENDOR_ID,
			MESH_VM_TEMP_SERVER_MODEL_ID,
			true,
			sizeof(vm_opcodes),
			vm_opcodes)->result;
	if (result != bg_err_success) {
		printf("temp: mesh_vendor_model_init failed: 0x%04x\r\n", result);
	}
}

void mesh_vm_temp_server_set(float temp) {
	uint16 result;
	result = gecko_cmd_mesh_vendor_model_set_publication(0,
			MESH_VM_VENDOR_ID,
			MESH_VM_TEMP_SERVER_MODEL_ID,
			temperature_status,
			true, 4, (uint8*)&temp)->result;
	if (result != bg_err_success) {
		printf("temp: set_publication failed: 0x%04x\r\n", result);
		return;
	}

	result = gecko_cmd_mesh_vendor_model_publish(0,
			MESH_VM_VENDOR_ID,
			MESH_VM_TEMP_SERVER_MODEL_ID)->result;
	if (result != bg_err_success) {
		printf("temp: publish failed: 0x%04x\r\n", result);
		return;
	}

}

#endif /* NET_MESH_VENDOR_SERVER */
