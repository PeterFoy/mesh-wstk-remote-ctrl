#ifndef NET_MESH_NET_H_
#define NET_MESH_NET_H_

#include "mesh_config.h"

#if MESH_LIB_HOST
#	include <host_gecko.h>
#else
#	include <native_gecko.h>
#	include "mesh_app_memory_config.h"
#endif

#include <stdint.h>
#include <stdbool.h>
#include <mesh_generic_model_capi_types.h>
#include <mesh_lighting_model_capi_types.h>


#if NET_MESH_GENERIC_ONOFF_SERVER
#	include "mesh_generic_onoff_server.h"
#endif
#if NET_MESH_GENERIC_LEVEL_SERVER
#	include "mesh_generic_level_server.h"
#endif
#if NET_MESH_SENSOR_SERVER
#	include "mesh_sensor_server.h"
#endif
#if NET_MESH_VENDOR_SERVER
#	include "mesh_vendor_server.h"
#endif

#if NET_MESH_GENERIC_ONOFF_CLIENT
#	include "mesh_generic_onoff_client.h"
#endif
#if NET_MESH_GENERIC_LEVEL_CLIENT
#	include "mesh_generic_level_client.h"
#endif
#if NET_MESH_LIGHT_LIGHTNESS_CLIENT
#	include "mesh_light_lightness_client.h"
#endif
#if NET_MESH_SENSOR_CLIENT
#	include "mesh_sensor_client.h"
#endif
#if NET_MESH_VENDOR_CLIENT
#	include "mesh_vendor_client.h"
#endif


typedef void (*set_timer_f)(uint32_t ms);

void net_mesh_init(bool provisioned, set_timer_f setter);
int16_t net_mesh_advertise(uint8_t bearers);
bool net_mesh_advertising(void);
bool net_mesh_provisioned(void);
void net_mesh_reset(void);

struct mesh_generic_request *net_mesh_transaction_new(uint16_t model);
void net_mesh_transaction_set(uint16_t server_addr, bool ack);
void net_mesh_transaction_publish(void);
void net_mesh_transaction_close(uint16_t model);
void net_mesh_transaction_timer_expired(void);

#if NET_MESH_LPN
	void net_mesh_lpn_init(set_timer_f setter);
	void net_mesh_lpn_poll(void);
	void net_mesh_lpn_deinit(void);
	void net_mesh_lpn_timer_expired(void);
#endif

void net_mesh_event_handler(struct gecko_cmd_packet *evt);

#endif /* NET_MESH_NET_H_ */
