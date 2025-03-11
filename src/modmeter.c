/* MOD level meter
 *
 * Copyright (C) 2016 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE

#define MODMETER_URI "http://gareus.org/oss/lv2/modmeter"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#if defined(__has_include) && __has_include(<lv2/core/lv2.h>)
#include <lv2/core/lv2.h>
#else
#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#endif

#define THROTTLE // MOD output port event throttle

enum {
	P_AIN = 0,
	P_RESET,
	P_LEVEL,
	P_PEAK,
	P_RMS,
	P_LAST
};

typedef struct {
	/* ports */
	float* ports[P_LAST];
	bool c_reset;

	/* internal state */
	float meter_level;
	float peak_level;
	float rms_level;

#ifdef THROTTLE
	float db_lvl;
	float db_rms;
#endif

	/* config */
	float    rate;
	float    rms_omega;
	float    falloff;
	uint32_t spp;
} ModMeter;


/* *****************************************************************************
 * LV2 Plugin
 */

static LV2_Handle
instantiate (const LV2_Descriptor*     descriptor,
             double                    rate,
             const char*               bundle_path,
             const LV2_Feature* const* features)
{
	ModMeter* self = (ModMeter*)calloc (1, sizeof (ModMeter));

	self->rate = rate;
  self->rms_omega = 9.72 / rate;
	return (LV2_Handle)self;
}

static void
connect_port (LV2_Handle instance,
              uint32_t   port,
              void*      data)
{
	ModMeter* self = (ModMeter*)instance;
	if (port < P_LAST) {
		self->ports[port] = (float*)data;
	}
}

static void
run (LV2_Handle instance, uint32_t n_samples)
{
	ModMeter* self = (ModMeter*)instance;

	if (*self->ports[P_RESET] > 0 && !self->c_reset) {
		self->peak_level = 0;
	}
	self->c_reset = *self->ports[P_RESET] > 0;

	if (self->spp != n_samples) {
		const float fall = 15.0f;
		const float tme  = (float) n_samples / self->rate;
		self->falloff    = powf (10.0f, -0.05f * fall * tme);
		self->spp = n_samples;
	}

	float l = self->meter_level + 1e-20f;
	float p = self->peak_level;
	float r = self->rms_level + 1e-20f;

	const float omega = self->rms_omega;
	const float* in = self->ports[P_AIN];

	l *= self->falloff;

	for (uint32_t i = 0; i < n_samples; ++i) {
		const float s = in[i];
		const float a = fabsf (s);
		const float s2 = s * s;

		if (a > p) { p = a; }
		if (a > l) { l = a; }
		r += omega * (s2 - r);
	}

	if (!isfinite (l)) l = 0;
	if (!isfinite (r)) r = 0;
	if (!isfinite (p)) p = 0;

	self->meter_level = l;
	self->peak_level  = p;
	self->rms_level   = r;

#ifdef THROTTLE
	float db_lvl = l > 1e-6f  ? 20.f * log10f (l) : -120;
	float db_rms = r > 1e-12f ? 10.f * log10f (r) : -120;

	if (db_lvl == -120 && self->db_lvl != -120) {
		*self->ports[P_LEVEL] = 0;
		self->db_lvl = db_lvl;
	}
	else if (fabsf (db_lvl - self->db_lvl) > .2) {
		*self->ports[P_LEVEL] = l;
		self->db_lvl = db_lvl;
	}

	if (db_rms == -120 && self->db_rms != -120) {
		*self->ports[P_RMS] = 0;
		self->db_rms = db_rms;
	}
	else if (fabsf (db_rms - self->db_rms) > .2) {
		*self->ports[P_RMS] = sqrtf (r);
		self->db_rms = db_rms;
	}
#else
	*self->ports[P_LEVEL] = l;
	*self->ports[P_RMS]   = sqrtf (r);
#endif
	*self->ports[P_PEAK]  = p;
}

static void
cleanup (LV2_Handle instance)
{
	free (instance);
}

static const void*
extension_data (const char* uri)
{
	return NULL;
}

static const LV2_Descriptor descriptor = {
	MODMETER_URI,
	instantiate,
	connect_port,
	NULL,
	run,
	NULL,
	cleanup,
	extension_data
};

#undef LV2_SYMBOL_EXPORT
#ifdef _WIN32
#    define LV2_SYMBOL_EXPORT __declspec(dllexport)
#else
#    define LV2_SYMBOL_EXPORT  __attribute__ ((visibility ("default")))
#endif
LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor (uint32_t index)
{
	switch (index) {
	case 0:
		return &descriptor;
	default:
		return NULL;
	}
}
