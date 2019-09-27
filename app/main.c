/* Board headers */
#include "init_mcu.h"
#include "init_board.h"
#include "init_app.h"
#include "ble-configuration.h"
#include "board_features.h"

/* Bluetooth stack headers */
#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"
#include <gecko_configuration.h>
#include <mesh_sizes.h>

/* Libraries containing default Gecko configuration values */
#include "em_emu.h"
#include "em_cmu.h"
#include <em_gpio.h>

/* Device initialization header */
#include "hal-config.h"

#if defined(HAL_CONFIG)
#include "bsphalconfig.h"
#else
#include "bspconfig.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sleep.h>

#include "net/mesh.h"
#include "ucli/ucli.h"
#include "main.h"
#include "worker.h"
#include "commands.h"
#include "led.h"

#include "bsp.h"
#include "display_interface.h"

//#define UI 1


#define TIMER_CLK_FREQ ((uint32)32768)
#define TIMER_MS_2_TIMERTICK(ms) ((TIMER_CLK_FREQ * ms) / 1000)

#define call(api) ({ \
	uint16_t result = api->result; \
	printf("%s: "#api" (0x%04x)\r\n", result ? "FAIL" : "OK  ", result); \
	result;})

#define UCLI_RECV_SIGNAL 	 (1<<0)
#define BTN0_CHANGED_SIGNAL  (1<<2)
#define BTN1_CHANGED_SIGNAL  (1<<3)

enum {
	TIMER_ID_FACTORY_RESET   = 77,
	TIMER_ID_NET_MESH_TIMEOUT,
	TIMER_ID_NET_MESH_LPN_TIMEOUT,
	TIMER_ID_LED_RED,
	TIMER_ID_LED_GREEN,
	TIMER_ID_WORKER_ = 90
};

void stdin_irq_hook(void) {
	gecko_external_signal(UCLI_RECV_SIGNAL);
}

void btn0_irq_hook(void) {
	gecko_external_signal(BTN0_CHANGED_SIGNAL);
}

void btn1_irq_hook(void) {
	gecko_external_signal(BTN1_CHANGED_SIGNAL);
}

void app_handle_timer_evt(struct gecko_msg_hardware_soft_timer_evt_t* evt)
{
	switch (evt->handle) {
	case TIMER_ID_FACTORY_RESET:
		app_reboot(0);
		break;
	case TIMER_ID_NET_MESH_TIMEOUT:
		net_mesh_transaction_timer_expired();
		break;
	case TIMER_ID_NET_MESH_LPN_TIMEOUT:
		net_mesh_lpn_timer_expired();
		break;
	case TIMER_ID_LED_RED:
		led_timer_expired(LED_RED);
		break;
	case TIMER_ID_LED_GREEN:
		led_timer_expired(LED_GREEN);
		break;
	default:
		if (evt->handle >= TIMER_ID_WORKER_ && evt->handle < TIMER_ID_WORKER_ + NUM_WORKERS) {
			uint32_t wid = evt->handle - TIMER_ID_WORKER_;
			workers_check(wid);
		} else {
			printf("ERROR: unhandled timer\r\n");
		}
	}
}

static void handle_external_signal(uint32_t signal) {

	if (signal & UCLI_RECV_SIGNAL) {
		uint8_t buf[8];
		size_t len = read(0, buf, sizeof(buf));
		if (len)
			ucli_feed_buffered(buf, len);
		signal &= ~(UCLI_RECV_SIGNAL);
	}
	if (signal & BTN0_CHANGED_SIGNAL) {
		uint16_t cur_val = mesh_light_lightness_value();
		if (cur_val >= 6553) {
			cur_val -= 6553;
		}
		mesh_light_lightness_client_publish(cur_val);
		signal &= ~(BTN0_CHANGED_SIGNAL);
	}
	if (signal & BTN1_CHANGED_SIGNAL) {
		uint16_t cur_val = mesh_light_lightness_value();
		if (cur_val <= (65535 - 6553)) {
			cur_val += 6553;
		}
		mesh_light_lightness_client_publish(cur_val);
		signal &= ~(BTN1_CHANGED_SIGNAL);
	}
	if (signal) {
		printf("unhandled external signal: %lu\r\n", signal);
	}
}

static void setup_mesh_timer(uint32_t delay_ms) {
	gecko_cmd_hardware_set_soft_timer(TIMER_MS_2_TIMERTICK(delay_ms), TIMER_ID_NET_MESH_TIMEOUT, 1);
}

static void setup_mesh_lpn_timer(uint32_t delay_ms) {
	gecko_cmd_hardware_set_soft_timer(TIMER_MS_2_TIMERTICK(delay_ms), TIMER_ID_NET_MESH_LPN_TIMEOUT, 1);
	if (delay_ms)
		led_blink(LED_GREEN, &SLOW_STROBE);
	else {
		led_clr(LED_GREEN);
	}
}

static void setup_worker_timer(uint8_t wid, uint32_t delay_ms) {
		gecko_cmd_hardware_set_soft_timer(TIMER_MS_2_TIMERTICK(delay_ms),
				TIMER_ID_WORKER_ + wid, 1);
}

static void setup_led_timer(enum led id, uint32_t ms) {
	gecko_cmd_hardware_set_soft_timer(TIMER_MS_2_TIMERTICK(ms),
			TIMER_ID_LED_RED + id, 1);
}

void app_event_handler(uint32_t evt_id, struct gecko_cmd_packet *evt) {

	switch (evt_id) {

	case gecko_evt_system_boot_id: {
		if (call(gecko_cmd_mesh_node_init())) {
			printf("Feature config changed. Factory reset\r\n");
			app_factory_reset();
		}
		//int16 set_power = gecko_cmd_system_set_tx_power((int16)-100)->set_power;
		//printf("tx power set to: %2.1f dBm\r\n", (float) set_power / 10);
		led_init(setup_led_timer);
		workers_init(setup_worker_timer);
		break;
	}

	case gecko_evt_mesh_node_initialized_id: {
		net_mesh_init(evt->data.evt_mesh_node_initialized.provisioned, setup_mesh_timer);
		if (net_mesh_provisioned()) {
			DI_Print("provisioned", LCD_ROW_STATUS);
			DI_Print("LPN disabled", LCD_ROW_LPN);
			/* debug: do not go to em2 sleep*/
			if (GPIO_PinInGet(BSP_BUTTON0_PORT, BSP_BUTTON0_PIN) == 0) {
				printf("lpn feature disabled due to debug run\r\n");
				DI_Print("LPN disabled", LCD_ROW_LPN);
				SLEEP_SleepBlockBegin(sleepEM2);
				ucli_init(ucli_commands, ucli_commands_num);
				led_blink(LED_RED, &FAST);
			} else {
				net_mesh_lpn_init(setup_mesh_lpn_timer);
				DI_Print("LPN enabled", LCD_ROW_LPN);
			}
		} else {
			SLEEP_SleepBlockBegin(sleepEM2);
			ucli_init(ucli_commands, ucli_commands_num);
			led_blink(LED_RED, &FAST);
			DI_Print("un-provisioned", LCD_ROW_STATUS);
			DI_Print("LPN disabled", 3);
		}
		break;
	}

	case gecko_evt_mesh_generic_client_server_status_id:
	case gecko_evt_mesh_generic_server_client_request_id:
	case gecko_evt_mesh_node_model_config_changed_id:
	case gecko_evt_mesh_generic_server_state_changed_id:
	case gecko_evt_mesh_vendor_model_receive_id:
	case gecko_evt_mesh_lpn_friendship_established_id:
	case gecko_evt_mesh_lpn_friendship_failed_id:
	case gecko_evt_mesh_lpn_friendship_terminated_id: {
		net_mesh_event_handler(evt);
		break;
	}

	case gecko_evt_mesh_node_reset_id: {
		app_factory_reset();
		break;
	}

	case gecko_evt_system_external_signal_id: {
		handle_external_signal(evt->data.evt_system_external_signal.extsignals);
		break;
	}

	case gecko_evt_hardware_soft_timer_id:
	{
		app_handle_timer_evt(&evt->data.evt_hardware_soft_timer);
		break;
	}

	case gecko_evt_le_gap_adv_timeout_id:
	{
		/* ignored. mesh packet sent. */
		break;
	}
	default:
		printf("unhandled event: 0x%08lx\r\n", evt_id);
	}
}

void app_factory_reset(void)
{
	/* TODO: close all ble connections. */
	gecko_cmd_flash_ps_erase_all();
	app_reboot(10);
}

void app_reboot(uint32 delay_ms)
{
	if (delay_ms)
		gecko_cmd_hardware_set_soft_timer(TIMER_MS_2_TIMERTICK(delay_ms), TIMER_ID_FACTORY_RESET, 1);
	else
		gecko_cmd_system_reset(0);
}
