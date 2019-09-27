#include <stddef.h>
#include "led.h"
#include "bsp.h"

struct id_map_entry {
  GPIO_Port_TypeDef port;
  unsigned int pin;
};

static struct {
	uint8_t state;
	led_timer_f timer;
	const struct blink* bspec[2];
} led;

const struct blink FAST = { 50, 50 };
const struct blink SLOW = { 500, 500 };
const struct blink SLOW_STROBE = { 50, 4950 };
static const struct id_map_entry ID_MAP[] = {
    {BSP_LED0_PORT, BSP_LED0_PIN },
    {BSP_LED1_PORT, BSP_LED1_PIN },
};

void led_init(led_timer_f timer) {
	led.timer = timer;
	led.bspec[LED_GREEN] = led.bspec[LED_RED] = NULL;
	GPIO_PinModeSet(ID_MAP[LED_GREEN].port, ID_MAP[LED_GREEN].pin, gpioModePushPull, 0);
	GPIO_PinModeSet(ID_MAP[LED_RED].port, ID_MAP[LED_RED].pin, gpioModePushPull, 0);
}

static void _led_set(enum led id) {
	led.state |= (1<<id);
	GPIO_PinOutSet(ID_MAP[id].port, ID_MAP[id].pin);
}

void led_set(enum led id) {
	led.bspec[id] = NULL;
	led.timer(id, 0);
	_led_set(id);
}

static void _led_clr(enum led id) {
	led.state &= ~(1<<id);
	GPIO_PinOutClear(ID_MAP[id].port, ID_MAP[id].pin);
}

void led_clr(enum led id) {
	led.bspec[id] = NULL;
	led.timer(id, 0);
	_led_clr(id);
}

void led_blink(enum led id, const struct blink* spec) {
	led.bspec[id] = spec;
	_led_set(id);
	led.timer(id, led.bspec[id]->on_time);
}

void led_timer_expired(enum led id) {
	if (led.state & (1<<id)) {
		_led_clr(id);
		led.timer(id, led.bspec[id]->off_time);
	} else {
		_led_set(id);
		led.timer(id, led.bspec[id]->on_time);
	}
}
