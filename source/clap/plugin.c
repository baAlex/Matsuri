/*

Copyright (c) 2026 Alexander Brandt

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.

This Source Code Form is "Incompatible With Secondary Licenses", as
defined by the Mozilla Public License, v. 2.0.
*/

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../thirdparty/clap/include/clap/clap.h" // IWYU pragma: keep
#include "../voice-allocator.h"


// PROTIP: On Zrythm is a good idea to remove "cached_plugin_descriptors.yaml" and "plugin-settings.yaml" after updating
// this plugin.


// https://nakst.gitlab.io/tutorial/clap-part-1.html


struct MatsuriPlugin
{
	clap_plugin_t plugin;
	const clap_host_t* host;

	float sampling_frequency;
	struct VoiceAllocator allocator;
};


static bool sInitialisePlugin(const struct clap_plugin* _plugin)
{
	(void)_plugin;
	return true;
}

static void sDestroyPlugin(const struct clap_plugin* _plugin)
{
	struct MatsuriPlugin* plugin = (struct MatsuriPlugin*)(_plugin->plugin_data);
	free(plugin);
}

static bool sActivatePlugin(const struct clap_plugin* _plugin, double sampling_frequency, uint32_t minimum_frames_count,
                            uint32_t maximum_frames_count)
{
	(void)minimum_frames_count;
	(void)maximum_frames_count;

	struct MatsuriPlugin* plugin = (struct MatsuriPlugin*)(_plugin->plugin_data);
	plugin->sampling_frequency = (float)(sampling_frequency);
	VoiceAllocatorSet(&plugin->allocator, plugin->sampling_frequency, MAX_MAX_ITEMS);
	return true;
}

static void sDeactivatePlugin(const struct clap_plugin* _plugin)
{
	(void)_plugin;
}

static bool sStartProcessingPlugin(const struct clap_plugin* _plugin)
{
	(void)_plugin;
	return true;
}

static void sStopProcessingPlugin(const struct clap_plugin* _plugin)
{
	(void)_plugin;
}

static void sResetPlugin(const struct clap_plugin* _plugin)
{
	struct MatsuriPlugin* plugin = (struct MatsuriPlugin*)(_plugin->plugin_data);
	VoiceAllocatorSet(&plugin->allocator, plugin->sampling_frequency, MAX_MAX_ITEMS);
}


static float sMinF(float a, float b)
{
	return (a < b) ? a : b;
}

static float sMaxF(float a, float b)
{
	return (a > b) ? a : b;
}


static clap_process_status sProcessPlugin(const struct clap_plugin* _plugin, const clap_process_t* process)
{
	struct MatsuriPlugin* plugin = (struct MatsuriPlugin*)(_plugin->plugin_data);

	assert(process->audio_outputs_count == 1);
	assert(process->audio_inputs_count == 0);

	const uint32_t frames = process->frames_count;
	const uint32_t input_events = process->in_events->size(process->in_events);
	uint32_t event_index = 0;
	uint32_t next_event_frame = input_events ? 0 : frames;

	for (uint32_t f = 0; f < frames;)
	{
		while (event_index < input_events && next_event_frame == f)
		{
			const clap_event_header_t* event = process->in_events->get(process->in_events, event_index);

			if (event->time != f)
			{
				next_event_frame = event->time;
				break;
			}

			if (event->space_id == CLAP_CORE_EVENT_SPACE_ID)
			{
				if (event->type == CLAP_EVENT_NOTE_ON)
				{
					// TODO, code path not tested yet
					const clap_event_note_t* note_event = (const clap_event_note_t*)(event);

					const int byte0 = note_event->channel | (9 << 4); // 'channel' is the same as MIDI
					const int byte1 = note_event->key;                // 'key' same as MIDI
					const int byte2 = (int)(sMaxF(sMinF((float)(note_event->velocity), 1.0f) * 127.0f, 1.0f));

					VoiceAllocatorMidi(&plugin->allocator, byte0, byte1, byte2);
				}
				else if (event->type == CLAP_EVENT_MIDI)
				{
					const clap_event_midi_t* midi = (const clap_event_midi_t*)(event);

					const int byte0 = midi->data[0];
					const int byte1 = midi->data[1];
					const int byte2 = midi->data[2];

					VoiceAllocatorMidi(&plugin->allocator, byte0, byte1, byte2);
				}
			}

			event_index += 1;

			if (event_index == input_events)
			{
				next_event_frame = frames;
				break;
			}
		}

		const uint32_t start = f;
		const uint32_t end = next_event_frame;
		VoiceAllocatorRender(&plugin->allocator, end - start, process->audio_outputs[0].data32[0] + start);
		memcpy(process->audio_outputs[0].data32[1], process->audio_outputs[0].data32[0], (end - start) * sizeof(float));

		f = next_event_frame;
	}

	return CLAP_PROCESS_CONTINUE;
}


static uint32_t sNotePortsCount(const clap_plugin_t* plugin, bool is_input)
{
	(void)plugin;
	return (is_input == true) ? 1 : 0;
}

static bool sNotePortsGet(const clap_plugin_t* plugin, uint32_t index, bool is_input, clap_note_port_info_t* info)
{
	(void)plugin;

	if (is_input == false || index != 0)
		return false;

	info->id = 0;
	info->supported_dialects = CLAP_NOTE_DIALECT_MIDI | CLAP_NOTE_DIALECT_CLAP;
	info->preferred_dialect = CLAP_NOTE_DIALECT_MIDI; // Zrythm seems to only talk in MIDI, plugin is not
	                                                  // detected by just supporting the CLAP dialect

	snprintf(info->name, sizeof(info->name), "%s", "Note Port");
	return true;
}

static uint32_t sAudioPortsCount(const clap_plugin_t* plugin, bool is_input)
{
	(void)plugin;
	return (is_input == true) ? 0 : 1;
}

static bool sAudioPortsGet(const clap_plugin_t* plugin, uint32_t index, bool is_input, clap_audio_port_info_t* info)
{
	(void)plugin;

	if (is_input == true || index != 0)
		return false;

	info->id = 0;
	info->flags = CLAP_AUDIO_PORT_IS_MAIN;
	info->port_type = CLAP_PORT_STEREO; // Zrythm is really bad at dealing with mono, it can do it, but at
	                                    // global level, affecting the whole project
	info->channel_count = 2;            // Same

	info->in_place_pair = CLAP_INVALID_ID;
	snprintf(info->name, sizeof(info->name), "%s", "Audio Output");
	return true;
}


static const clap_plugin_note_ports_t s_extension_note_ports = {
    .count = sNotePortsCount,
    .get = sNotePortsGet,
};

static const clap_plugin_audio_ports_t s_extension_audio_ports = {
    .count = sAudioPortsCount,
    .get = sAudioPortsGet,
};


static const void* sGetExtension(const struct clap_plugin* plugin, const char* id)
{
	(void)plugin;
	if (strcmp(id, CLAP_EXT_NOTE_PORTS) == 0)
		return &s_extension_note_ports;
	if (strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0)
		return &s_extension_audio_ports;
	return NULL;
}

static void sOnMainThread(const struct clap_plugin* _plugin)
{
	(void)_plugin;
}


static const clap_plugin_descriptor_t s_descriptor = {
    .clap_version = CLAP_VERSION_INIT,
    .id = "baAlex.Matsuri.v2",
    .name = "Matsuri V2",
    .vendor = "Alexander Brandt",
    .url = "https://github.com/baAlex/Matsuri/",
    .manual_url = "https://github.com/baAlex/Matsuri/",
    .support_url = "https://github.com/baAlex/Matsuri/",
    .version = "2.0",
    .description = "TR-606 Synthesizer",

    .features =
        (const char*[]){
            CLAP_PLUGIN_FEATURE_INSTRUMENT,
            CLAP_PLUGIN_FEATURE_DRUM_MACHINE,
            CLAP_PLUGIN_FEATURE_STEREO,
            NULL,
        },
};

static const clap_plugin_t s_plugin_class = {
    .desc = &s_descriptor,
    .plugin_data = NULL,
    .init = sInitialisePlugin,
    .destroy = sDestroyPlugin,
    .activate = sActivatePlugin,
    .deactivate = sDeactivatePlugin,
    .start_processing = sStartProcessingPlugin,
    .stop_processing = sStopProcessingPlugin,
    .reset = sResetPlugin,
    .process = sProcessPlugin,
    .get_extension = sGetExtension,
    .on_main_thread = sOnMainThread,
};


static uint32_t sGetPluginCount(const struct clap_plugin_factory* factory)
{
	(void)factory;
	return 1;
}

static const clap_plugin_descriptor_t* sGetDescriptor(const struct clap_plugin_factory* factory, uint32_t index)
{
	(void)factory;
	return (index == 0) ? &s_descriptor : NULL;
}

static const clap_plugin_t* sCreatePlugin(const struct clap_plugin_factory* factory, const clap_host_t* host,
                                          const char* plugin_id)
{
	(void)factory;

	if (clap_version_is_compatible(host->clap_version) == false || strcmp(plugin_id, s_descriptor.id) != 0)
		return NULL;

	struct MatsuriPlugin* plugin = calloc(1, sizeof(struct MatsuriPlugin));
	if (plugin == NULL)
		return NULL;

	plugin->host = host;
	plugin->plugin = s_plugin_class;
	plugin->plugin.plugin_data = plugin;
	return &plugin->plugin;
}


static const clap_plugin_factory_t s_factory = {
    .get_plugin_count = sGetPluginCount,
    .get_plugin_descriptor = sGetDescriptor,
    .create_plugin = sCreatePlugin,
};


static bool sInitialiseClap(const char* path)
{
	(void)path;
	return true;
}

static void sDeinitialiseClap(void) {}

static const void* sGetFactory(const char* factory_id)
{
	return (strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0) ? &s_factory : NULL;
}


const clap_plugin_entry_t clap_entry = {
    .clap_version = CLAP_VERSION_INIT,
    .init = sInitialiseClap,
    .deinit = sDeinitialiseClap,
    .get_factory = sGetFactory,
};
