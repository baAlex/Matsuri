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

#include "../thirdparty/clap/include/clap/clap.h" // IWYU pragma: keep

#include "../misc.h"
#include "../voice-allocator.h"


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


#define PARAMETERS_NO 11
enum ParameterId
{
	PARAMETER_VOLUME = 0,
	PARAMETER_KICK_VOLUME,
	PARAMETER_SNARE_VOLUME,
	PARAMETER_CLOSED_HAT_VOLUME,
	PARAMETER_OPEN_HAT_VOLUME,
	PARAMETER_CYMBAL_VOLUME,
	PARAMETER_LOW_TOM_VOLUME,
	PARAMETER_HIGH_TOM_VOLUME,
	PARAMETER_VELOCITY_VOLUME_MODULATION,
	PARAMETER_VELOCITY_TONE_MODULATION,
	PARAMETER_VELOCITY_REFERENCE,
};

struct ParameterInfo
{
	enum ParameterId id;
	const char* name;
	float default_value;
	float min_value;
	float max_value;

	float fixed_conversion;
	const char* unit;
};

static struct ParameterInfo s_parameters_info[PARAMETERS_NO] = {
    {PARAMETER_VOLUME, "Master Volume", 100.0f, 0.0f, 100.0f, 655.36f, "%"},

    {PARAMETER_KICK_VOLUME, "Bass Drum Volume", 100.0f, 0.0f, 100.0f, 655.36f, "%"},
    {PARAMETER_SNARE_VOLUME, "Snare Drum Volume", 100.0f, 0.0f, 100.0f, 655.36f, "%"},
    {PARAMETER_CLOSED_HAT_VOLUME, "Closed Hit-Hat Volume", 100.0f, 0.0f, 100.0f, 655.36f, "%"},
    {PARAMETER_OPEN_HAT_VOLUME, "Open Hit-Hat Volume", 100.0f, 0.0f, 100.0f, 655.36f, "%"},
    {PARAMETER_CYMBAL_VOLUME, "Cymbal Volume", 100.0f, 0.0f, 100.0f, 655.36f, "%"},
    {PARAMETER_LOW_TOM_VOLUME, "Low Tom Volume", 100.0f, 0.0f, 100.0f, 655.36f, "%"},
    {PARAMETER_HIGH_TOM_VOLUME, "High Tom Volume", 100.0f, 0.0f, 100.0f, 655.36f, "%"},

    {PARAMETER_VELOCITY_VOLUME_MODULATION, "Velocity-Volume Modulation", 1.0f, 0.0f, 1.0f, 65536.0f, "x"},
    {PARAMETER_VELOCITY_TONE_MODULATION, "Velocity-Tone Modulation", 1.0f, 0.0f, 1.0f, 65536.0f, "x"},
    {PARAMETER_VELOCITY_REFERENCE, "Velocity Reference", 0.5f, 0.0f, 1.0f, 65536.0f, "x"},
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


static int sFloatToFixed(float v, float conversion)
{
	return (int)(v * conversion);
}

static float sFixedToFloat(int v, float conversion)
{
	return (float)(v) / conversion;
}

static float sParameterToVolume(const struct MatsuriPlugin* plugin, int index)
{
	return ExponentialVolumeEasing(                                                          //
	    sFixedToFloat(plugin->parameter[index], s_parameters_info[index].fixed_conversion) / //
	    s_parameters_info[index].max_value);
}


/////////////////////////////
// clap_plugin_audio_ports //
/////////////////////////////

static uint32_t sPluginAudioPortsNo(const clap_plugin_t* plugin, bool is_input)
{
	(void)plugin;
	return (is_input == true) ? 0 : 1;
}

static bool sPluginAudioPortsGet(const clap_plugin_t* plugin, uint32_t index, bool is_input,
                                 clap_audio_port_info_t* info)
{
	(void)plugin;

	if (is_input == true || index != 0)
		return false;

	info->id = 0;
	info->flags = CLAP_AUDIO_PORT_IS_MAIN;
	info->port_type = CLAP_PORT_STEREO; // Zrythm is really bad at dealing with mono, it can do it, but at
	                                    // a global level, affecting the whole project
	info->channel_count = 2;            // Same

	info->in_place_pair = CLAP_INVALID_ID;
	snprintf(info->name, sizeof(info->name), "%s", "Audio Output");
	return true;
}

static const clap_plugin_audio_ports_t s_plugin_audio_ports_extensions = {
    .count = sPluginAudioPortsNo,
    .get = sPluginAudioPortsGet,
};


////////////////////////////
// clap_plugin_note_ports //
////////////////////////////

static uint32_t sPluginNotePortsNo(const clap_plugin_t* plugin, bool is_input)
{
	(void)plugin;
	return (is_input == true) ? 1 : 0;
}

static bool sPluginNotePortsGet(const clap_plugin_t* plugin, uint32_t index, bool is_input, clap_note_port_info_t* info)
{
	(void)plugin;

	if (is_input == false || index != 0)
		return false;

	info->id = 0;
	info->supported_dialects = CLAP_NOTE_DIALECT_MIDI | CLAP_NOTE_DIALECT_CLAP;
	info->preferred_dialect = CLAP_NOTE_DIALECT_MIDI;

	// Zrythm seems to only talk in MIDI, plugin is not detected
	// by just supporting the CLAP dialect

	// QTractor is the opposite, only talks in CLAP.
	// Worse, it doesn't reject the plugin in any discernible way,
	// it simply sends the event and we never take that code path.

	snprintf(info->name, sizeof(info->name), "%s", "Note Port");
	return true;
}

static const clap_plugin_note_ports_t s_plugin_note_ports_extensions = {
    .count = sPluginNotePortsNo,
    .get = sPluginNotePortsGet,
};


////////////////////////////
// clap_plugin_parameters //
////////////////////////////

static uint32_t sPluginParametersNo(const clap_plugin_t* plugin)
{
	(void)plugin;
	return PARAMETERS_NO;
}

static bool sPluginParametersInfo(const clap_plugin_t* plugin, uint32_t index, clap_param_info_t* out)
{
	(void)plugin;

	if (index < PARAMETERS_NO)
	{
		memset(out, 0, sizeof(clap_param_info_t));
		out->id = index;

		out->flags = CLAP_PARAM_IS_AUTOMATABLE;
		out->default_value = s_parameters_info[index].default_value;
		out->min_value = s_parameters_info[index].min_value;
		out->max_value = s_parameters_info[index].max_value;
		strcpy(out->name, s_parameters_info[index].name);

		return true;
	}

	return false;
}

static bool sPluginParametersValue(const clap_plugin_t* plugin_, clap_id id, double* value)
{
	struct MatsuriPlugin* plugin = (struct MatsuriPlugin*)(plugin_->plugin_data);
	const uint32_t index = (uint32_t)id;

	if (index < PARAMETERS_NO)
	{
		*value = (double)(sFixedToFloat(plugin->parameter[index], s_parameters_info[index].fixed_conversion));
		return true;
	}

	return false;
}

static bool sPluginParametersValueToText(const clap_plugin_t* plugin, clap_id id, double value, char* display,
                                         uint32_t size)
{
	(void)plugin;
	const uint32_t index = (uint32_t)id;

	if (index < PARAMETERS_NO)
	{
		if (s_parameters_info[index].max_value > 10.0f)
			snprintf(display, size, "%.1f%s", value, s_parameters_info[index].unit);
		else
			snprintf(display, size, "%.2f%s", value, s_parameters_info[index].unit);

		return true;
	}

	return false;
}

static bool sPluginParametersTextToValue(const clap_plugin_t* plugin, clap_id id, const char* display, double* value)
{
	(void)plugin;
	(void)id;
	(void)display;
	(void)value;
	return false; // TODO, it's important, user may enter values using the keyboard
}

static void sPluginParametersFlush(const clap_plugin_t* plugin_, const clap_input_events_t* in,
                                   const clap_output_events_t* out)
{
	struct MatsuriPlugin* plugin = (struct MatsuriPlugin*)(plugin_->plugin_data);
	(void)out;

	for (uint32_t event_index = 0; event_index < in->size(in); event_index += 1)
	{
		const clap_event_header_t* event = in->get(in, event_index);
		if (event->space_id != CLAP_CORE_EVENT_SPACE_ID || event->type != CLAP_EVENT_PARAM_VALUE)
			continue;

		const clap_event_param_value_t* param_event = (const clap_event_param_value_t*)(in->get(in, event_index));
		const uint32_t index = (uint32_t)param_event->param_id;

		if (index < PARAMETERS_NO)
		{
			plugin->parameter[index] =
			    sFloatToFixed((float)(param_event->value), s_parameters_info[index].fixed_conversion);
			plugin->parameters_changed_offline = 1;
		}
	}
}

static const clap_plugin_params_t s_plugin_parameters_extensions = {
    .count = sPluginParametersNo,
    .get_info = sPluginParametersInfo,
    .get_value = sPluginParametersValue,
    .value_to_text = sPluginParametersValueToText,
    .text_to_value = sPluginParametersTextToValue,
    .flush = sPluginParametersFlush,
};


/////////////////
// clap_plugin //
/////////////////

static bool sPluginInitialise(const struct clap_plugin* plugin_)
{
	struct MatsuriPlugin* plugin = (struct MatsuriPlugin*)(plugin_->plugin_data);

	atomic_init(&plugin->parameters_changed_offline, 1); // Force an initial update

	for (int i = 0; i < PARAMETERS_NO; i += 1)
	{
		atomic_init(&plugin->parameter[i],
		            sFloatToFixed(s_parameters_info[i].default_value, s_parameters_info[i].fixed_conversion));
	}

	return true;
}

static void sPluginDestroy(const struct clap_plugin* plugin_)
{
	struct MatsuriPlugin* plugin = (struct MatsuriPlugin*)(plugin_->plugin_data);
	free(plugin);
}

static bool sPluginActivate(const struct clap_plugin* plugin_, double sampling_frequency, uint32_t minimum_frames_count,
                            uint32_t maximum_frames_count)
{
	(void)minimum_frames_count;
	(void)maximum_frames_count;

	struct MatsuriPlugin* plugin = (struct MatsuriPlugin*)(plugin_->plugin_data);
	plugin->sampling_frequency = (float)(sampling_frequency);
	VoiceAllocatorSet(&plugin->allocator, plugin->sampling_frequency, MAX_MAX_ITEMS);
	// TODO, should I set parameters again?, like in Initialise()???

	return true;
}

static void sPluginDeactivate(const struct clap_plugin* plugin_)
{
	(void)plugin_;
}

static bool sPluginStartProcessing(const struct clap_plugin* plugin_)
{
	(void)plugin_;
	return true;
}

static void sPluginStopProcessing(const struct clap_plugin* plugin_)
{
	(void)plugin_;
}

static void sPluginReset(const struct clap_plugin* plugin_)
{
	struct MatsuriPlugin* plugin = (struct MatsuriPlugin*)(plugin_->plugin_data);
	VoiceAllocatorSet(&plugin->allocator, plugin->sampling_frequency, MAX_MAX_ITEMS);
	// TODO, should I set parameters again?, like in Initialise()???
}

static void sPluginProcessEvent(struct MatsuriPlugin* plugin, const clap_event_header_t* event)
{
	if (event->type == CLAP_EVENT_NOTE_ON)
	{
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

		if (index >= PARAMETERS_NO)
			return;

		// Keep parameter value around
		plugin->parameter[index] =
		    sFloatToFixed((float)(param_event->value), s_parameters_info[index].fixed_conversion);

		// Handle special cases
		if (index == PARAMETER_VOLUME)
		{
			plugin->parameters_changed_offline = 1; // Force all other voices to update
			return;                                 // Nothing more to do
		}
		else if (index == PARAMETER_VELOCITY_VOLUME_MODULATION || index == PARAMETER_VELOCITY_TONE_MODULATION ||
		         index == PARAMETER_VELOCITY_REFERENCE)
		{
			VoiceAllocatorConfigure(
			    &plugin->allocator,
			    sFixedToFloat(plugin->parameter[(int)(PARAMETER_VELOCITY_VOLUME_MODULATION)],
			                  s_parameters_info[(int)(PARAMETER_VELOCITY_VOLUME_MODULATION)].fixed_conversion), //
			    sFixedToFloat(plugin->parameter[(int)(PARAMETER_VELOCITY_TONE_MODULATION)],
			                  s_parameters_info[(int)(PARAMETER_VELOCITY_TONE_MODULATION)].fixed_conversion), //
			    sFixedToFloat(plugin->parameter[(int)(PARAMETER_VELOCITY_REFERENCE)],
			                  s_parameters_info[(int)(PARAMETER_VELOCITY_REFERENCE)].fixed_conversion));
			return; // Nothing more to do
		}

		// Handle volume parameter
		const float v = sParameterToVolume(plugin, (int)(PARAMETER_VOLUME)) *
		                ExponentialVolumeEasing(((float)(param_event->value) / s_parameters_info[index].max_value));

		switch (index)
		{
		case PARAMETER_KICK_VOLUME: VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_KICK, v); break;
		case PARAMETER_SNARE_VOLUME: VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_SNARE, v); break;
		case PARAMETER_CLOSED_HAT_VOLUME: VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_CLOSED_HAT, v); break;
		case PARAMETER_OPEN_HAT_VOLUME: VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_OPEN_HAT, v); break;
		case PARAMETER_CYMBAL_VOLUME: VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_CYMBAL, v); break;
		case PARAMETER_LOW_TOM_VOLUME: VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_LOW_TOM, v); break;
		case PARAMETER_HIGH_TOM_VOLUME: VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_HIGH_TOM, v); break;
		default: break;
		}
	}
}

static clap_process_status sPluginProcess(const struct clap_plugin* plugin_, const clap_process_t* process)
{
	struct MatsuriPlugin* plugin = (struct MatsuriPlugin*)(plugin_->plugin_data);

	assert(process->audio_outputs_count == 1);
	assert(process->audio_inputs_count == 0);

	const uint32_t frames = process->frames_count;
	const uint32_t input_events = process->in_events->size(process->in_events);
	uint32_t event_index = 0;
	uint32_t next_event_frame = input_events ? 0 : frames;

	if (plugin->parameters_changed_offline != 0) // Outside code changed our guys (changed them not using events)
	{
		plugin->parameters_changed_offline = 0;
		const float v = sParameterToVolume(plugin, (int)(PARAMETER_VOLUME));

		VoiceAllocatorConfigure(
		    &plugin->allocator,
		    sFixedToFloat(plugin->parameter[(int)(PARAMETER_VELOCITY_VOLUME_MODULATION)],
		                  s_parameters_info[(int)(PARAMETER_VELOCITY_VOLUME_MODULATION)].fixed_conversion), //
		    sFixedToFloat(plugin->parameter[(int)(PARAMETER_VELOCITY_TONE_MODULATION)],
		                  s_parameters_info[(int)(PARAMETER_VELOCITY_TONE_MODULATION)].fixed_conversion), //
		    sFixedToFloat(plugin->parameter[(int)(PARAMETER_VELOCITY_REFERENCE)],
		                  s_parameters_info[(int)(PARAMETER_VELOCITY_REFERENCE)].fixed_conversion));

		VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_KICK,
		                             v * sParameterToVolume(plugin, (int)(PARAMETER_KICK_VOLUME)));
		VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_SNARE,
		                             v * sParameterToVolume(plugin, (int)(PARAMETER_SNARE_VOLUME)));
		VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_CLOSED_HAT,
		                             v * sParameterToVolume(plugin, (int)(PARAMETER_CLOSED_HAT_VOLUME)));
		VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_OPEN_HAT,
		                             v * sParameterToVolume(plugin, (int)(PARAMETER_OPEN_HAT_VOLUME)));
		VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_CYMBAL,
		                             v * sParameterToVolume(plugin, (int)(PARAMETER_CYMBAL_VOLUME)));
		VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_LOW_TOM,
		                             v * sParameterToVolume(plugin, (int)(PARAMETER_LOW_TOM_VOLUME)));
		VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_HIGH_TOM,
		                             v * sParameterToVolume(plugin, (int)(PARAMETER_HIGH_TOM_VOLUME)));
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
				sPluginProcessEvent(plugin, event);
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

static const void* sPluginGetExtension(const struct clap_plugin* plugin, const char* id)
{
	(void)plugin;
	if (strcmp(id, CLAP_EXT_NOTE_PORTS) == 0)
		return &s_plugin_note_ports_extensions;
	if (strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0)
		return &s_plugin_audio_ports_extensions;
	if (strcmp(id, CLAP_EXT_PARAMS) == 0)
		return &s_plugin_parameters_extensions;
	return NULL;
}

static void sPluginOnMainThread(const struct clap_plugin* plugin_)
{
	(void)plugin_;
}

static const clap_plugin_t s_plugin_class = {
    .desc = &s_descriptor,
    .plugin_data = NULL,
    .init = sPluginInitialise,
    .destroy = sPluginDestroy,
    .activate = sPluginActivate,
    .deactivate = sPluginDeactivate,
    .start_processing = sPluginStartProcessing,
    .stop_processing = sPluginStopProcessing,
    .reset = sPluginReset,
    .process = sPluginProcess,
    .get_extension = sPluginGetExtension,
    .on_main_thread = sPluginOnMainThread,
};

static const clap_plugin_t* sPluginCreate(const struct clap_plugin_factory* factory, const clap_host_t* host,
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


/////////////////////////
// clap_plugin_factory //
/////////////////////////

static uint32_t sFactoryPluginsNo(const struct clap_plugin_factory* factory)
{
	(void)factory;
	return 1;
}

static const clap_plugin_descriptor_t* sFactoryGetDescriptor(const struct clap_plugin_factory* factory, uint32_t index)
{
	(void)factory;
	return (index == 0) ? &s_descriptor : NULL;
}

static const clap_plugin_factory_t s_factory = {
    .get_plugin_count = sFactoryPluginsNo,
    .get_plugin_descriptor = sFactoryGetDescriptor,
    .create_plugin = sPluginCreate,
};


////////////////
// clap_entry //
////////////////

static bool sEntryInitialisation(const char* path)
{
	(void)path;
	return true;
}

static void sEntryDeinitialisation(void) {}

static const void* sEntryGetFactory(const char* factory_id)
{
	return (strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0) ? &s_factory : NULL;
}

EXPORT const clap_plugin_entry_t clap_entry = {
    .clap_version = CLAP_VERSION_INIT,
    .init = sEntryInitialisation,
    .deinit = sEntryDeinitialisation,
    .get_factory = sEntryGetFactory,
};
