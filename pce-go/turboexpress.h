// TurboExpress (PC Engine GT) hardware profile.
// Header-only; the single instance `te_cfg` is defined in pce.c.
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "pce-go.h"

#ifdef __cplusplus
extern "C" {
#endif

// Compile-time switch: 1 = TurboExpress build (no CD, 1 pad, 2 buttons).
#ifndef TURBOEXPRESS
#define TURBOEXPRESS 1
#endif

// Pad bits that exist on the handheld: D-Pad, I, II, Select, Run.
#define TE_PAD_MASK (JOY_UP | JOY_DOWN | JOY_LEFT | JOY_RIGHT | JOY_A | JOY_B | JOY_SELECT | JOY_RUN)

typedef enum {
	TE_TURBO_OFF = 0,
	TE_TURBO_SLOW,   // ~10 Hz: state flips every 3 frames @60fps
	TE_TURBO_FAST,   // ~20 Hz: 3-frame period (2 frames on, 1 off)
} te_turbo_t;

typedef struct {
	uint8_t turbo_switch_I;   // te_turbo_t, acts on button I  (JOY_A)
	uint8_t turbo_switch_II;  // te_turbo_t, acts on button II (JOY_B)
	bool    headphones;       // false = mono speaker, true = stereo
	uint8_t lcd_sat;          // LCD saturation level 0..4 (index into a table in main.cpp)
	uint8_t lcd_contrast;     // LCD contrast level 0..4
	uint8_t lcd_grid;         // pixel grid: 0 = off, 1 = light, 2 = medium, 3 = strong
	bool    lcd;              // LCD look: desaturated palette, pixel grid, 256->320 stretch
	uint32_t frame;           // free-running frame counter (turbo clock)
} te_config_t;

extern te_config_t te_cfg;

// Returns true while the oscillator holds the button "pressed" for this frame.
static inline bool te_turbo_phase(te_turbo_t mode, uint32_t frame)
{
	switch (mode) {
	case TE_TURBO_SLOW: return (frame % 6) < 3;  // 3 on, 3 off  -> 10 Hz
	case TE_TURBO_FAST: return (frame % 3) != 2; // 2 on, 1 off -> 20 Hz
	default:            return true;
	}
}

// Filter the raw P1 state before $1000 can be read by the CPU.
// Called once per frame. Always returns only TE_PAD_MASK bits.
static inline uint8_t te_filter_pad(uint8_t raw)
{
	uint8_t pad = raw & TE_PAD_MASK;
	uint32_t f = te_cfg.frame;

	if ((pad & JOY_A) && !te_turbo_phase((te_turbo_t)te_cfg.turbo_switch_I, f))
		pad &= ~JOY_A;
	if ((pad & JOY_B) && !te_turbo_phase((te_turbo_t)te_cfg.turbo_switch_II, f))
		pad &= ~JOY_B;

	te_cfg.frame++;
	return pad;
}

// (L+R)/2 downmix of an interleaved stereo buffer, in place; both channels
// receive the same value. No-op when headphones are enabled.
static inline void te_audio_downmix(int16_t *buf, int frames)
{
	if (te_cfg.headphones)
		return;
	for (int i = 0; i < frames; i++) {
		int16_t m = (int16_t)(((int32_t)buf[2 * i] + (int32_t)buf[2 * i + 1]) >> 1);
		buf[2 * i] = m;
		buf[2 * i + 1] = m;
	}
}

#ifdef __cplusplus
}
#endif
