#ifndef APP_NET_MESH_LIGHT_LIGHTNESS_CLIENT_H_
#define APP_NET_MESH_LIGHT_LIGHTNESS_CLIENT_H_

void mesh_light_lightness_client_register(void);

void mesh_light_lightness_client_server_response_cb(uint16_t model_id,
                                              uint16_t element_index,
                                              uint16_t client_addr,
                                              uint16_t server_addr,
                                              const struct mesh_generic_state *current,
                                              const struct mesh_generic_state *target,
                                              uint32_t remaining_ms,
                                              uint8_t response_flags);

void mesh_light_lightness_client_set(uint16_t server_addr, uint16_t value, bool ack);
void mesh_light_lightness_client_publish(uint16_t value);
void mesh_light_lightness_client_get(uint16_t server_addr);
uint16_t mesh_light_lightness_value(void);

#endif /* APP_NET_MESH_LIGHT_LIGHTNESS_CLIENT_H_ */
