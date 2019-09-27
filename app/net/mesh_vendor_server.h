#ifndef APP_NET_MESH_VENDOR_SERVER_H_
#define APP_NET_MESH_VENDOR_SERVER_H_

#define MESH_VM_VENDOR_ID 0xbeef

#include "vm_temp_client.h"
#include "vm_temp_server.h"

void mesh_vendor_server_register(void);
void mesh_vendor_client_register(void);
void mesh_vendor_server_event_handler(struct gecko_cmd_packet *evt);

#endif /* APP_NET_MESH_VENDOR_SERVER_H_ */
