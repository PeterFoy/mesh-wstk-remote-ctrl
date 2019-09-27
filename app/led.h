#ifndef APP_LED_H_
#define APP_LED_H_

#include <stdint.h>


enum led {
	LED_RED,
	LED_GREEN
};

struct blink {
	uint32_t on_time;
	uint32_t off_time;
};

extern const struct blink FAST;
extern const struct blink SLOW;
extern const struct blink SLOW_STROBE;

typedef void (*led_timer_f)(enum led id, uint32_t ms);

void led_init(led_timer_f timer);
void led_set(enum led);
void led_clr(enum led);
void led_blink(enum led, const struct blink*);
void led_timer_expired(enum led id);

#endif /* APP_LED_H_ */
