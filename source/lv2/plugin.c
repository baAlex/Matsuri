/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include <stdlib.h>

#include "lv2/atom/atom.h"
#include "lv2/atom/util.h"
#include "lv2/core/lv2.h"
#include "lv2/core/lv2_util.h"
#include "lv2/log/log.h"
#include "lv2/log/logger.h"
#include "lv2/midi/midi.h"
#include "lv2/urid/urid.h"

#include "../misc.h"
#include "../version.h"
#include "../voice-allocator.h"


struct Matsuri
{
	// Ports buffers
	const LV2_Atom_Sequence* control;
	float* out[2];

	// Features
	LV2_URID_Map* map;
	LV2_Log_Logger logger;

	// Uris
	LV2_URID midi_type_thingie;

	// ####

	struct VoiceAllocator allocator;
};


static LV2_Handle sInitialize(const LV2_Descriptor* descriptor, double rate, const char* bundle_path,
                              const LV2_Feature* const* features)
{
	(void)descriptor;
	(void)bundle_path;

	// Initialize stuff
	struct Matsuri* plugin = (struct Matsuri*)(realloc(NULL, sizeof(struct Matsuri)));
	if (plugin == NULL)
		return NULL;

	memset(plugin, 0, sizeof(struct Matsuri));
	VoiceAllocatorSet(&plugin->allocator, (float)(rate), MAX_MAX_ITEMS);

	// Scan host features for URID map
	const char* missing =
	    lv2_features_query(features, LV2_LOG__log, &plugin->logger.log, false, LV2_URID__map, &plugin->map, true, NULL);

	lv2_log_logger_set_map(&plugin->logger, plugin->map);
	if (missing != NULL)
	{
		lv2_log_error(&plugin->logger, "Missing feature <%s>\n", missing);
		free(plugin);
		return NULL;
	}

	plugin->midi_type_thingie = plugin->map->map(plugin->map->handle, LV2_MIDI__MidiEvent);

	// Bye!
	return (LV2_Handle*)(plugin);
}


static void sDeinitialize(LV2_Handle instance)
{ (void)instance; }


static void sClean(LV2_Handle instance)
{
	struct Matsuri* plugin = (struct Matsuri*)(instance);

	if (plugin != NULL)
		free(instance);
}


static void sConnectPort(LV2_Handle instance, uint32_t port, void* data)
{
	struct Matsuri* plugin = (struct Matsuri*)(instance);

	switch (port)
	{
	case 0: plugin->control = (const LV2_Atom_Sequence*)(data); break;
	case 1: plugin->out[0] = (float*)(data); break;
	case 2: plugin->out[1] = (float*)(data); break;
	default: break;
	}
}


static void sActivate(LV2_Handle instance)
{ (void)instance; }


static void sRun(LV2_Handle instance, uint32_t frames)
{
	struct Matsuri* plugin = (struct Matsuri*)(instance);

	// Handle input
	LV2_ATOM_SEQUENCE_FOREACH(plugin->control, ev) // Nasty, nasty, DEFINE garbage
	{
		if (ev->body.type == plugin->midi_type_thingie)
		{
			const uint8_t* msg = (const uint8_t*)(ev + 1);
			VoiceAllocatorMidi(&plugin->allocator, *(msg + 0), *(msg + 1), *(msg + 2));
		}
	}

	// Render
	VoiceAllocatorRender(&plugin->allocator, frames, plugin->out[0]);
	memcpy(plugin->out[1], plugin->out[0], (size_t)(frames) * sizeof(float));
}


static const void* sExtensionData(const char* uri)
{
	(void)uri;
	return NULL;
}


static const LV2_Descriptor descriptor = {"https://github.com/baAlex/Matsuri",
                                          sInitialize,
                                          sConnectPort,
                                          sActivate,
                                          sRun,
                                          sDeinitialize,
                                          sClean,
                                          sExtensionData};

LV2_SYMBOL_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{ return index == 0 ? &descriptor : NULL; }
