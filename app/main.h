#ifndef APP_MAIN_H_
#define APP_MAIN_H_

#include <stdint.h>

void app_event_handler(uint32_t evt_id, struct gecko_cmd_packet *evt);

void app_factory_reset(void);
void app_reboot(uint32_t delay_ms);
void app_measure(void);
void app_sensor_period(uint16_t sec);
#endif /* APP_MAIN_H_ */
