#ifndef APP_MESH_GENERIC_LEVEL_SERVER_H_
#define APP_MESH_GENERIC_LEVEL_SERVER_H_


void mesh_generic_level_server_register(void);

void mesh_generic_level_server_client_request_cb(uint16_t model_id,
                                             uint16_t element_index,
                                             uint16_t client_addr,
                                             uint16_t server_addr,
                                             uint16_t appkey_index,
                                             const struct mesh_generic_request *req,
                                             uint32_t transition_ms,
                                             uint16_t delay_ms,
                                             uint8_t request_flags);

void mesh_generic_level_server_change_cb(uint16_t model_id,
                                     uint16_t element_index,
                                     const struct mesh_generic_state *current,
                                     const struct mesh_generic_state *target,
                                     uint32_t remaining_ms);

void mesh_generic_level_server_set(int16_t new_value);

#endif /* APP_MESH_GENERIC_LEVEL_SERVER_H_ */
