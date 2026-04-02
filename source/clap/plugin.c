/*

Copyright (c) 2026 Alexander Brandt

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.

This Source Code Form is "Incompatible With Secondary Licenses", as
defined by the Mozilla Public License, v. 2.0.
*/

#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../misc.h"
#include "../thirdparty/clap/include/clap/clap.h" // IWYU pragma: keep
#include "../voice-allocator.h"


// PROTIP: On Zrythm is a good idea to remove "cached_plugin_descriptors.yaml" and "plugin-settings.yaml" after updating
// this plugin.


// https://nakst.gitlab.io/tutorial/clap-part-1.html
// https://nakst.gitlab.io/tutorial/clap-part-2.html


#define PARAMETERS_NO 7
enum Parameter
{
	PARAMETER_KICK_AMPLIFY = 0,
	PARAMETER_SNARE_AMPLIFY,
	PARAMETER_CLOSED_HAT_AMPLIFY,
	PARAMETER_OPEN_HAT_AMPLIFY,
	PARAMETER_CYMBAL_AMPLIFY,
	PARAMETER_LOW_TOM_AMPLIFY,
	PARAMETER_HIGH_TOM_AMPLIFY,
};

struct MatsuriPlugin
{
	clap_plugin_t plugin;
	const clap_host_t* host;

	float sampling_frequency;
	struct VoiceAllocator allocator;

	atomic_int parameters_changed_offline;
	atomic_int parameter[PARAMETERS_NO];
};


static bool sInitialisePlugin(const struct clap_plugin* _plugin)
{
	struct MatsuriPlugin* plugin = (struct MatsuriPlugin*)(_plugin->plugin_data);

	atomic_init(&plugin->parameters_changed_offline, 1); // Force an initial update

	atomic_init(&plugin->parameter[(int)(PARAMETER_KICK_AMPLIFY)], (int)(1.0 * 4096.0));
	atomic_init(&plugin->parameter[(int)(PARAMETER_SNARE_AMPLIFY)], (int)(1.0 * 4096.0));
	atomic_init(&plugin->parameter[(int)(PARAMETER_CLOSED_HAT_AMPLIFY)], (int)(1.0 * 4096.0));
	atomic_init(&plugin->parameter[(int)(PARAMETER_OPEN_HAT_AMPLIFY)], (int)(1.0 * 4096.0));
	atomic_init(&plugin->parameter[(int)(PARAMETER_CYMBAL_AMPLIFY)], (int)(1.0 * 4096.0));
	atomic_init(&plugin->parameter[(int)(PARAMETER_LOW_TOM_AMPLIFY)], (int)(1.0 * 4096.0));
	atomic_init(&plugin->parameter[(int)(PARAMETER_HIGH_TOM_AMPLIFY)], (int)(1.0 * 4096.0));

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


static clap_process_status sProcessPlugin(const struct clap_plugin* _plugin, const clap_process_t* process)
{
	struct MatsuriPlugin* plugin = (struct MatsuriPlugin*)(_plugin->plugin_data);

	assert(process->audio_outputs_count == 1);
	assert(process->audio_inputs_count == 0);

	const uint32_t frames = process->frames_count;
	const uint32_t input_events = process->in_events->size(process->in_events);
	uint32_t event_index = 0;
	uint32_t next_event_frame = input_events ? 0 : frames;

	if (plugin->parameters_changed_offline != 0) // Outside code changed our guys (changed them not using events)
	{
		plugin->parameters_changed_offline = 0;

		VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_KICK,
		                             (float)(plugin->parameter[(int)(PARAMETER_KICK_AMPLIFY)]) / 4096.0f);
		VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_SNARE,
		                             (float)(plugin->parameter[(int)(PARAMETER_SNARE_AMPLIFY)]) / 4096.0f);
		VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_CLOSED_HAT,
		                             (float)(plugin->parameter[(int)(PARAMETER_CLOSED_HAT_AMPLIFY)]) / 4096.0f);
		VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_OPEN_HAT,
		                             (float)(plugin->parameter[(int)(PARAMETER_OPEN_HAT_AMPLIFY)]) / 4096.0f);
		VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_CYMBAL,
		                             (float)(plugin->parameter[(int)(PARAMETER_CYMBAL_AMPLIFY)]) / 4096.0f);
		VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_LOW_TOM,
		                             (float)(plugin->parameter[(int)(PARAMETER_LOW_TOM_AMPLIFY)]) / 4096.0f);
		VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_HIGH_TOM,
		                             (float)(plugin->parameter[(int)(PARAMETER_HIGH_TOM_AMPLIFY)]) / 4096.0f);
	}

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
					const clap_event_note_t* note_on_event = (const clap_event_note_t*)(event);

					const int byte0 = note_on_event->channel | (9 << 4); // 'channel' is the same as MIDI
					const int byte1 = note_on_event->key;                // 'key' same as MIDI
					const int byte2 = (int)(MaxF(MinF((float)(note_on_event->velocity), 1.0f) * 127.0f, 1.0f));

					VoiceAllocatorMidi(&plugin->allocator, byte0, byte1, byte2);
				}
				else if (event->type == CLAP_EVENT_MIDI)
				{
					const clap_event_midi_t* midi_event = (const clap_event_midi_t*)(event);

					const int byte0 = midi_event->data[0];
					const int byte1 = midi_event->data[1];
					const int byte2 = midi_event->data[2];

					VoiceAllocatorMidi(&plugin->allocator, byte0, byte1, byte2);
				}
				else if (event->type == CLAP_EVENT_PARAM_VALUE)
				{
					const clap_event_param_value_t* param_event = (const clap_event_param_value_t*)(event);
					const uint32_t index = (uint32_t)param_event->param_id;

					if (index >= 0 && index < PARAMETERS_NO)
					{
						plugin->parameter[index] = (int)(param_event->value * 4096.0);
					}

					switch (index)
					{
					case PARAMETER_KICK_AMPLIFY:
						VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_KICK, (float)(param_event->value));
						break;
					case PARAMETER_SNARE_AMPLIFY:
						VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_SNARE, (float)(param_event->value));
						break;
					case PARAMETER_CLOSED_HAT_AMPLIFY:
						VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_CLOSED_HAT, (float)(param_event->value));
						break;
					case PARAMETER_OPEN_HAT_AMPLIFY:
						VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_OPEN_HAT, (float)(param_event->value));
						break;
					case PARAMETER_CYMBAL_AMPLIFY:
						VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_CYMBAL, (float)(param_event->value));
						break;
					case PARAMETER_LOW_TOM_AMPLIFY:
						VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_LOW_TOM, (float)(param_event->value));
						break;
					case PARAMETER_HIGH_TOM_AMPLIFY:
						VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_HIGH_TOM, (float)(param_event->value));
						break;
					default: break;
					}
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
		memcpy(process->audio_outputs[0].data32[1] + start, process->audio_outputs[0].data32[0] + start,
		       (end - start) * sizeof(float));

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


static uint32_t sParametersCount(const clap_plugin_t* plugin)
{
	(void)plugin;
	return PARAMETERS_NO;
}

static void sVolumeParameter(const char* label, uint32_t index, clap_param_info_t* out)
{
	memset(out, 0, sizeof(clap_param_info_t));
	out->id = index;

	out->flags = CLAP_PARAM_IS_AUTOMATABLE;
	out->min_value = 0.0f;
	out->max_value = 1.0f;
	out->default_value = 1.0f;
	strcpy(out->name, label);
}

static bool sParametersInfo(const clap_plugin_t* plugin, uint32_t index, clap_param_info_t* out)
{
	(void)plugin;

	switch (index)
	{
	case PARAMETER_KICK_AMPLIFY: sVolumeParameter("Kick Gain", index, out); return true;
	case PARAMETER_SNARE_AMPLIFY: sVolumeParameter("Snare Gain", index, out); return true;
	case PARAMETER_CLOSED_HAT_AMPLIFY: sVolumeParameter("Closed Hat Gain", index, out); return true;
	case PARAMETER_OPEN_HAT_AMPLIFY: sVolumeParameter("Open Hat Gain", index, out); return true;
	case PARAMETER_CYMBAL_AMPLIFY: sVolumeParameter("Cymbal Gain", index, out); return true;
	case PARAMETER_LOW_TOM_AMPLIFY: sVolumeParameter("Low Tom Gain", index, out); return true;
	case PARAMETER_HIGH_TOM_AMPLIFY: sVolumeParameter("High Tom Gain", index, out); return true;
	default: break;
	}

	return false;
}

static bool sParametersValue(const clap_plugin_t* _plugin, clap_id id, double* value)
{
	struct MatsuriPlugin* plugin = (struct MatsuriPlugin*)(_plugin->plugin_data);
	const uint32_t index = (uint32_t)id;

	if (index >= 0 && index < PARAMETERS_NO)
	{
		*value = (double)(plugin->parameter[index]) / 4096.0;
		return true;
	}

	return false;
}

static bool sParametersValueToText(const clap_plugin_t* plugin, clap_id id, double value, char* display, uint32_t size)
{
	(void)plugin;
	const uint32_t index = (uint32_t)id;

	if (index >= 0 && index < PARAMETERS_NO)
	{
		snprintf(display, size, "%f", value);
		return true;
	}

	return false;
}

static bool sParametersTextToValue(const clap_plugin_t* plugin, clap_id id, const char* display, double* value)
{
	(void)plugin;
	(void)id;
	(void)display;
	(void)value;
	// TODO, it's important, user may enter values using the keyboard
	return false;
}

static void sParametersFlush(const clap_plugin_t* _plugin, const clap_input_events_t* in,
                             const clap_output_events_t* out)
{
	struct MatsuriPlugin* plugin = (struct MatsuriPlugin*)(_plugin->plugin_data);
	(void)out;

	for (uint32_t event_index = 0; event_index < in->size(in); event_index += 1)
	{
		const clap_event_header_t* event = in->get(in, event_index);
		if (event->space_id != CLAP_CORE_EVENT_SPACE_ID || event->type != CLAP_EVENT_PARAM_VALUE)
			continue;

		const clap_event_param_value_t* param_event = (const clap_event_param_value_t*)(in->get(in, event_index));
		const uint32_t index = (uint32_t)param_event->param_id;

		if (index >= 0 && index < PARAMETERS_NO)
		{
			plugin->parameter[index] = (int)(param_event->value * 4096.0);
			plugin->parameters_changed_offline = 1;
		}
	}
}


static const clap_plugin_note_ports_t s_extension_note_ports = {
    .count = sNotePortsCount,
    .get = sNotePortsGet,
};

static const clap_plugin_audio_ports_t s_extension_audio_ports = {
    .count = sAudioPortsCount,
    .get = sAudioPortsGet,
};

static const clap_plugin_params_t s_extension_parameters = {
    .count = sParametersCount,
    .get_info = sParametersInfo,
    .get_value = sParametersValue,
    .value_to_text = sParametersValueToText,
    .text_to_value = sParametersTextToValue,
    .flush = sParametersFlush,
};

static const void* sGetExtension(const struct clap_plugin* plugin, const char* id)
{
	(void)plugin;
	if (strcmp(id, CLAP_EXT_NOTE_PORTS) == 0)
		return &s_extension_note_ports;
	if (strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0)
		return &s_extension_audio_ports;
	if (strcmp(id, CLAP_EXT_PARAMS) == 0)
		return &s_extension_parameters;
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


EXPORT const clap_plugin_entry_t clap_entry = {
    .clap_version = CLAP_VERSION_INIT,
    .init = sInitialiseClap,
    .deinit = sDeinitialiseClap,
    .get_factory = sGetFactory,
};
