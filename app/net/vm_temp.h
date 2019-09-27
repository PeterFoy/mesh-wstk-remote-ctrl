#ifndef APP_NET_VM_TEMP_H_
#define APP_NET_VM_TEMP_H_

#define MESH_VM_TEMP_SERVER_MODEL_ID 0x0001
#define MESH_VM_TEMP_CLIENT_MODEL_ID 0x0002

typedef enum {
	temperature_get = 0x1,
	temperature_status,
	VM_TEMP_OPCODES_NUM
} vm_temp_opcodes_t;

static const uint8_t vm_opcodes[] = {
		temperature_get,
		temperature_status
};

#endif /* APP_NET_VM_TEMP_H_ */
