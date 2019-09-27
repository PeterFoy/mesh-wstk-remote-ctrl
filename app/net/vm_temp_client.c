#include <stdint.h>
#include <stdio.h>
#include "mesh.h"

#if NET_MESH_VENDOR_CLIENT

void mesh_vm_temp_client_register(void) {
	uint16 result;
	result = gecko_cmd_mesh_vendor_model_init(0,
			MESH_VM_VENDOR_ID,
			MESH_VM_TEMP_CLIENT_MODEL_ID,
			true,
			sizeof(vm_opcodes),
			vm_opcodes)->result;
	if (result != bg_err_success) {
		printf("temp: mesh_vendor_model_init failed: 0x%04x\r\n", result);
	}
}

#endif /* NET_MESH_VENDOR_CLIENT */
