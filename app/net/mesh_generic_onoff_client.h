#ifndef APP_MESH_GENERIC_ONOFF_CLIENT_H_
#define APP_MESH_GENERIC_ONOFF_CLIENT_H_

void mesh_generic_onoff_client_register(void);

void mesh_generic_onoff_client_server_response_cb(uint16_t model_id,
                                              uint16_t element_index,
                                              uint16_t client_addr,
                                              uint16_t server_addr,
                                              const struct mesh_generic_state *current,
                                              const struct mesh_generic_state *target,
                                              uint32_t remaining_ms,
                                              uint8_t response_flags);

void mesh_generic_onoff_client_set(uint16_t server_addr, bool value, bool ack);
void mesh_generic_onoff_client_publish(bool value);
void mesh_generic_onoff_client_get(uint16_t server_addr);

#endif /* APP_MESH_GENERIC_ONOFF_CLIENT_H_ */
