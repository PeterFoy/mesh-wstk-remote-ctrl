#ifndef APP_MESH_GENERIC_LEVEL_CLIENT_H_
#define APP_MESH_GENERIC_LEVEL_CLIENT_H_

void mesh_generic_level_client_register(void);

void mesh_generic_level_client_server_response_cb(uint16_t model_id,
                                              uint16_t element_index,
                                              uint16_t client_addr,
                                              uint16_t server_addr,
                                              const struct mesh_generic_state *current,
                                              const struct mesh_generic_state *target,
                                              uint32_t remaining_ms,
                                              uint8_t response_flags);

void mesh_generic_level_client_set(uint16_t server_addr, int16_t value, bool ack);
void mesh_generic_level_client_publish(int16_t value);
void mesh_generic_level_client_get(uint16_t server_addr);
void mesh_generic_level_timeout(void);

#endif /* APP_MESH_GENERIC_LEVEL_CLIENT_H_ */
