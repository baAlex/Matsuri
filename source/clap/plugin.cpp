/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clap/clap.h" // IWYU pragma: keep

extern "C"
{
#include "../misc.h"
#include "../version.h"
#include "../voice-allocator.h"
}

#define UNNECESSARY_PRINTS 1

static const char* const s_features[] = {
    CLAP_PLUGIN_FEATURE_INSTRUMENT,
    CLAP_PLUGIN_FEATURE_DRUM_MACHINE,
    CLAP_PLUGIN_FEATURE_STEREO,
    nullptr,
};

static const clap_plugin_descriptor s_descriptor = {
    /* .clap_version */ CLAP_VERSION_INIT,
    /* .id */ MATSURI_URI,
    /* .name */ MATSURI_NAME,
    /* .vendor */ MATSURI_VENDOR,
    /* .url */ MATSURI_URL,
    /* .manual_url */ MATSURI_URL,
    /* .support_url */ MATSURI_URL,
    /* .version */ MATSURI_VERSION_STRING,
    /* .description */ MATSURI_DESCRIPTION,
    /* .features */ s_features,
};


#define PARAMETERS_NO 12
enum ParameterId
{
	PARAMETER_KICK_VOLUME = 0,
	PARAMETER_SNARE_VOLUME,
	PARAMETER_CLOSED_HAT_VOLUME,
	PARAMETER_OPEN_HAT_VOLUME,
	PARAMETER_CYMBAL_VOLUME,
	PARAMETER_LOW_TOM_VOLUME,
	PARAMETER_HIGH_TOM_VOLUME,
	PARAMETER_VELOCITY_VOLUME_MODULATION,
	PARAMETER_VELOCITY_TONE_MODULATION,
	PARAMETER_VELOCITY_REFERENCE,
	PARAMETER_LIMITER_DECAY,
	PARAMETER_MASTER_VOLUME,
};

struct ParameterInfo
{
	const char* group; // TODO, Neither Zrythm nor QTractor honor this
	const char* name;
	float default_value;
	float min_value;
	float max_value;

	float fixed_conversion;
	const char* unit;
};

static const ParameterInfo s_parameters_info[PARAMETERS_NO] = {
    {"Volume", "Bass drum", 100.0f, 0.0f, 100.0f, 655.36f, "%"},
    {"Volume", "Snare drum", 100.0f, 0.0f, 100.0f, 655.36f, "%"},
    {"Volume", "Closed hi-hat", 65.0f, 0.0f, 100.0f, 655.36f, "%"},
    {"Volume", "Open hi-hat", 70.0f, 0.0f, 100.0f, 655.36f, "%"},
    {"Volume", "Cymbal", 80.0f, 0.0f, 100.0f, 655.36f, "%"},
    {"Volume", "Low tom", 100.0f, 0.0f, 100.0f, 655.36f, "%"},
    {"Volume", "High tom", 100.0f, 0.0f, 100.0f, 655.36f, "%"},

    {"Velocity", "Velocity-Volume modulation", 1.0f, 0.0f, 1.0f, 65536.0f, "x"},
    {"Velocity", "Velocity-Tone modulation", 1.0f, 0.0f, 1.0f, 65536.0f, "x"},
    {"Velocity", "Velocity reference", 0.5f, 0.0f, 1.0f, 65536.0f, "x"},

    {"Other", "Limiter decay", 0.0f, 0.0f, 1000.0f, 65.536f, "ms"},
    {"Other", "Master volume", 100.0f, 0.0f, 100.0f, 655.36f, "%"},
};

struct MatsuriPlugin
{
	clap_plugin plugin;
	const clap_host* host;
	const clap_host_log* host_log;

	float sampling_frequency;
	VoiceAllocator allocator;

	atomic_int parameters_changed_offline;
	atomic_int parameter[PARAMETERS_NO];
};


static int sFloatToFixed(float v, float conversion)
{
	return static_cast<int>(v * conversion);
}

static float sFixedToFloat(int v, float conversion)
{
	return static_cast<float>(v) / conversion;
}

static float sParameter(const MatsuriPlugin* plugin, int index)
{
	return sFixedToFloat(plugin->parameter[index], s_parameters_info[index].fixed_conversion) /
	       s_parameters_info[index].max_value;
}

static float sVolumeParameter(const MatsuriPlugin* plugin, int index)
{
	return ExponentialVolumeEasing(                                                          //
	    sFixedToFloat(plugin->parameter[index], s_parameters_info[index].fixed_conversion) / //
	    s_parameters_info[index].max_value);
}


/////////////////////////////
// clap_plugin_audio_ports //
/////////////////////////////

static uint32_t sPluginAudioPortsNo(const clap_plugin*, bool is_input)
{
	return (is_input == true) ? 0 : 1;
}

static bool sPluginAudioPortsGet(const clap_plugin*, uint32_t index, bool is_input, clap_audio_port_info* info)
{
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

static const clap_plugin_audio_ports s_plugin_audio_ports_extensions = {
    /* .count */ sPluginAudioPortsNo,
    /* .get */ sPluginAudioPortsGet,
};


////////////////////////////
// clap_plugin_note_ports //
////////////////////////////

static uint32_t sPluginNotePortsNo(const clap_plugin*, bool is_input)
{
	return (is_input == true) ? 1 : 0;
}

static bool sPluginNotePortsGet(const clap_plugin*, uint32_t index, bool is_input, clap_note_port_info* info)
{
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

static const clap_plugin_note_ports s_plugin_note_ports_extensions = {
    /* .count */ sPluginNotePortsNo,
    /* .get */ sPluginNotePortsGet,
};


////////////////////////////
// clap_plugin_parameters //
////////////////////////////

static uint32_t sPluginParametersNo(const clap_plugin*)
{
	return PARAMETERS_NO;
}

static bool sPluginParametersInfo(const clap_plugin*, uint32_t index, clap_param_info* out)
{
	if (index < PARAMETERS_NO)
	{
		memset(out, 0, sizeof(clap_param_info));
		out->id = index;

		// Without CLAP_PARAM_REQUIRES_PROCESS, Zrythm rejects it
		out->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_REQUIRES_PROCESS;
		out->default_value = s_parameters_info[index].default_value;
		out->min_value = s_parameters_info[index].min_value;
		out->max_value = s_parameters_info[index].max_value;

		strcpy(out->name, s_parameters_info[index].name);
		strcpy(out->module, s_parameters_info[index].group);

		return true;
	}

	return false;
}

static bool sPluginParametersValue(const clap_plugin* plugin_, clap_id id, double* value)
{
	MatsuriPlugin* plugin = (MatsuriPlugin*)(plugin_->plugin_data);
	uint32_t index = (uint32_t)(id);

	if (index < PARAMETERS_NO)
	{
		*value = (double)(sFixedToFloat(plugin->parameter[index], s_parameters_info[index].fixed_conversion));
		return true;
	}

	return false;
}

static bool sPluginParametersValueToText(const clap_plugin*, clap_id id, double value, char* display, uint32_t size)
{
	uint32_t index = (uint32_t)(id);

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

static bool sPluginParametersTextToValue(const clap_plugin*, clap_id, const char*, double*)
{
	return false; // TODO, it's important, user may enter values using the keyboard
}

static void sPluginParametersFlush(const clap_plugin* plugin_, const clap_input_events* in, const clap_output_events*)
{
	MatsuriPlugin* plugin = (MatsuriPlugin*)(plugin_->plugin_data);

#ifdef UNNECESSARY_PRINTS
	if (plugin->host_log != nullptr)
		plugin->host_log->log(plugin->host, CLAP_LOG_INFO, "### Matsuri: sPluginParametersFlush()");
#endif

	for (uint32_t event_index = 0; event_index < in->size(in); event_index += 1)
	{
		const clap_event_header* event = in->get(in, event_index);
		if (event->space_id != CLAP_CORE_EVENT_SPACE_ID || event->type != CLAP_EVENT_PARAM_VALUE)
			continue;

		const clap_event_param_value* param_event = (const clap_event_param_value*)(in->get(in, event_index));
		const uint32_t index = (uint32_t)(param_event->param_id);

		if (index < PARAMETERS_NO)
		{
			plugin->parameter[index] =
			    sFloatToFixed(static_cast<float>(param_event->value), s_parameters_info[index].fixed_conversion);
			plugin->parameters_changed_offline = 1;
		}
	}
}

static const clap_plugin_params s_plugin_parameters_extensions = {
    /* .count */ sPluginParametersNo,
    /* .get_info */ sPluginParametersInfo,
    /* .get_value */ sPluginParametersValue,
    /* .value_to_text */ sPluginParametersValueToText,
    /* .text_to_value */ sPluginParametersTextToValue,
    /* .flush */ sPluginParametersFlush,
};


////////////////
// clap_state //
////////////////

static bool sPluginStateSave(const clap_plugin* plugin_, const clap_ostream* stream)
{
	MatsuriPlugin* plugin = (MatsuriPlugin*)(plugin_->plugin_data);

#ifdef UNNECESSARY_PRINTS
	if (plugin->host_log != nullptr)
		plugin->host_log->log(plugin->host, CLAP_LOG_INFO, "### Matsuri: sPluginStateSave()");
#endif

	int p[PARAMETERS_NO];
	for (int i = 0; i < PARAMETERS_NO; i += 1)
		p[i] = plugin->parameter[i]; // Not sure if needed, but, calling an pseudo fwrite() on
		                             // top of atomic types feels bad

	// Write version
	const int version = MATSURI_VERSION_MAJOR;
	if (stream->write(stream, &version, sizeof(int)) != sizeof(int))
	{
		if (plugin->host_log != nullptr)
			plugin->host_log->log(plugin->host, CLAP_LOG_ERROR, "Matsuri | Host error writing state/preset\n");
		return false;
	}

	// Write parameters
	for (int i = 0; i < PARAMETERS_NO; i += 1)
	{
		if (stream->write(stream, &p[i], sizeof(int)) != sizeof(int))
		{
			if (plugin->host_log != nullptr)
				plugin->host_log->log(plugin->host, CLAP_LOG_ERROR, "Matsuri | Host error writing state/preset\n");
			return false;
		}
	}

	// Bye!
	return true;
}

static bool sPluginStateLoad(const clap_plugin* plugin_, const clap_istream* stream)
{
	MatsuriPlugin* plugin = (MatsuriPlugin*)(plugin_->plugin_data);

#ifdef UNNECESSARY_PRINTS
	if (plugin->host_log != nullptr)
		plugin->host_log->log(plugin->host, CLAP_LOG_INFO, "### Matsuri: sPluginStateLoad()");
#endif

	int p[PARAMETERS_NO];

	// Read version
	int version;
	if (stream->read(stream, &version, sizeof(int)) != sizeof(int))
	{
		if (plugin->host_log != nullptr)
			plugin->host_log->log(plugin->host, CLAP_LOG_ERROR, "Matsuri | Host error reading state/preset\n");
		return false;
	}

	if (version != MATSURI_VERSION_MAJOR) // There isn't old versions yet
	{
		if (plugin->host_log != nullptr)
			plugin->host_log->log(plugin->host, CLAP_LOG_ERROR, "Matsuri | Unknown state/preset version\n");
		return false;
	}

	// Read parameters
	for (int i = 0; i < PARAMETERS_NO; i += 1)
	{
		if (stream->read(stream, &p[i], sizeof(int)) != sizeof(int))
		{
			if (plugin->host_log != nullptr)
				plugin->host_log->log(plugin->host, CLAP_LOG_ERROR, "Matsuri | Host error reading state/preset\n");
			return false;
		}
	}

	for (int i = 0; i < PARAMETERS_NO; i += 1)
		plugin->parameter[i] = p[i];

	// Bye!
	return true;
}

static const clap_plugin_state s_plugin_state_extension = {
    /* .save */ sPluginStateSave,
    /* .load */ sPluginStateLoad,
};


//////////////
// clap_gui //
//////////////

// https://nakst.gitlab.io/tutorial/clap-part-3.html

#define GUI_API CLAP_WINDOW_API_X11

static bool sPluginIsAPISupported(const clap_plugin*, const char* api, bool is_floating)
{
	if (strcmp(api, GUI_API) == 0 && is_floating == false)
		return true;

	return false;
}

static bool sGetPreferredApi(const clap_plugin*, const char** api, bool* is_floating)
{
	*api = GUI_API;
	*is_floating = false;
	return true;
}

static bool sCreate(const clap_plugin* plugin_, const char* api, bool is_floating)
{
	if (sPluginIsAPISupported(plugin_, api, is_floating) == false)
		return false;
	// We'll define GUICreate in our platform specific code file.
	// GUICreate((MyPlugin*)plugin_->plugin_data);
	return true;
}

static void sDestroy(const clap_plugin* plugin_)
{
	(void)plugin_;
	// We'll define GUIDestroy in our platform specific code file.
	// GUIDestroy((MyPlugin*)plugin_->plugin_data);
}

static bool sSetScale(const clap_plugin* plugin_, double scale)
{
	(void)plugin_;
	(void)scale;
	return false;
}

static bool sGetSize(const clap_plugin* plugin_, uint32_t* width, uint32_t* height)
{
	(void)plugin_;
	*width = 640;
	*height = 480;
	return true;
}

static bool sCanResize(const clap_plugin* plugin_)
{
	(void)plugin_;
	return false;
}

static bool sGetResizeHints(const clap_plugin* plugin_, clap_gui_resize_hints* hints)
{
	(void)plugin_;
	(void)hints;
	return false;
}

static bool sAdjustSize(const clap_plugin* plugin_, uint32_t* width, uint32_t* height)
{
	return sGetSize(plugin_, width, height);
}

static bool sSetSize(const clap_plugin* plugin_, uint32_t width, uint32_t height)
{
	(void)plugin_;
	(void)width;
	(void)height;
	return true;
}

static bool sSetParent(const clap_plugin* plugin_, const clap_window* window)
{
	assert(strcmp(window->api, GUI_API) == 0);
	(void)plugin_;
	// We'll define GUISetParent in our platform specific code file.
	// GUISetParent((MyPlugin*)plugin_->plugin_data, window);
	return true;
}

static bool sSetTransient(const clap_plugin* plugin_, const clap_window* window)
{
	(void)plugin_;
	(void)window;
	return false;
}

static void sSuggestTitle(const clap_plugin* plugin_, const char* title)
{
	(void)plugin_;
	(void)title;
}

static bool sShow(const clap_plugin* plugin_)
{
	(void)plugin_;

	// We'll define GUISetVisible in our platform specific code file.
	// GUISetVisible((MyPlugin*)plugin_->plugin_data, true);
	return true;
}

static bool sHide(const clap_plugin* plugin_)
{
	(void)plugin_;

	// GUISetVisible((MyPlugin*)plugin_->plugin_data, false);
	return true;
}

static const clap_plugin_gui s_plugin_gui_extension = {
    /* .is_api_supported */ sPluginIsAPISupported,
    /* .get_preferred_api */ sGetPreferredApi,
    /* .create */ sCreate,
    /* .destroy */ sDestroy,
    /* .set_scale */ sSetScale,
    /* .get_size */ sGetSize,
    /* .can_resize */ sCanResize,
    /* .get_resize_hints */ sGetResizeHints,
    /* .adjust_size */ sAdjustSize,
    /* .set_size */ sSetSize,
    /* .set_parent */ sSetParent,
    /* .set_transient */ sSetTransient,
    /* .suggest_title */ sSuggestTitle,
    /* .show */ sShow,
    /* .hide */ sHide,
};


/////////////////
// clap_plugin //
/////////////////

static bool sPluginInitialise(const clap_plugin* plugin_)
{
	MatsuriPlugin* plugin = (MatsuriPlugin*)(plugin_->plugin_data);

	plugin->host_log = (const clap_host_log*)(plugin->host->get_extension(plugin->host, CLAP_EXT_LOG));

#ifdef UNNECESSARY_PRINTS
	if (plugin->host_log != nullptr)
		plugin->host_log->log(plugin->host, CLAP_LOG_INFO, "### Matsuri: sPluginInitialise()");
#endif

	atomic_init(&plugin->parameters_changed_offline, 1); // Force an initial update

	for (int i = 0; i < PARAMETERS_NO; i += 1)
	{
		atomic_init(&plugin->parameter[i],
		            sFloatToFixed(s_parameters_info[i].default_value, s_parameters_info[i].fixed_conversion));
	}

	return true;
}

static void sPluginDestroy(const clap_plugin* plugin_)
{
	MatsuriPlugin* plugin = (MatsuriPlugin*)(plugin_->plugin_data);
	free(plugin);
}

static bool sPluginActivate(const clap_plugin* plugin_, double sampling_frequency, uint32_t, uint32_t)
{
	MatsuriPlugin* plugin = (MatsuriPlugin*)(plugin_->plugin_data);
	plugin->sampling_frequency = static_cast<float>(sampling_frequency);
	VoiceAllocatorSet(&plugin->allocator, plugin->sampling_frequency, MAX_MAX_ITEMS);
	// TODO, should I set parameters again?, like in Initialise()???

	plugin->host_log = (const clap_host_log*)(plugin->host->get_extension(plugin->host, CLAP_EXT_LOG));

#ifdef UNNECESSARY_PRINTS
	if (plugin->host_log != nullptr)
		plugin->host_log->log(plugin->host, CLAP_LOG_INFO, "### Matsuri: sPluginActivate()");
#endif

	return true;
}

static void sPluginDeactivate(const clap_plugin*) {}

static bool sPluginStartProcessing(const clap_plugin*)
{
	return true;
}

static void sPluginStopProcessing(const clap_plugin*) {}

static void sPluginReset(const clap_plugin* plugin_)
{
	MatsuriPlugin* plugin = (MatsuriPlugin*)(plugin_->plugin_data);
	VoiceAllocatorSet(&plugin->allocator, plugin->sampling_frequency, MAX_MAX_ITEMS);
	// TODO, should I set parameters again?, like in Initialise()???

	plugin->host_log = (const clap_host_log*)(plugin->host->get_extension(plugin->host, CLAP_EXT_LOG));

#ifdef UNNECESSARY_PRINTS
	if (plugin->host_log != nullptr)
		plugin->host_log->log(plugin->host, CLAP_LOG_INFO, "### Matsuri: sPluginReset()");
#endif
}

static void sPluginProcessEvent(MatsuriPlugin* plugin, const clap_event_header* event)
{
	if (event->type == CLAP_EVENT_NOTE_ON)
	{
		const clap_event_note* note_on_event = (const clap_event_note*)(event);

		const int byte0 = note_on_event->channel | (9 << 4); // 'channel' is the same as MIDI
		const int byte1 = note_on_event->key;                // 'key' same as MIDI
		const int byte2 =
		    static_cast<int>(MaxF(MinF(static_cast<float>(note_on_event->velocity), 1.0f) * 127.0f, 1.0f));

		VoiceAllocatorMidi(&plugin->allocator, byte0, byte1, byte2);
	}
	else if (event->type == CLAP_EVENT_MIDI)
	{
		const clap_event_midi* midi_event = (const clap_event_midi*)(event);

		const int byte0 = midi_event->data[0];
		const int byte1 = midi_event->data[1];
		const int byte2 = midi_event->data[2];

		VoiceAllocatorMidi(&plugin->allocator, byte0, byte1, byte2);
	}
	else if (event->type == CLAP_EVENT_PARAM_VALUE)
	{
		const clap_event_param_value* param_event = (const clap_event_param_value*)(event);
		const uint32_t index = (uint32_t)(param_event->param_id);

		if (index >= PARAMETERS_NO)
			return;

		// Keep parameter value around
		plugin->parameter[index] =
		    sFloatToFixed(static_cast<float>(param_event->value), s_parameters_info[index].fixed_conversion);

		// Handle special cases
		if (index == PARAMETER_VELOCITY_VOLUME_MODULATION || index == PARAMETER_VELOCITY_TONE_MODULATION ||
		    index == PARAMETER_VELOCITY_REFERENCE || index == PARAMETER_LIMITER_DECAY ||
		    index == PARAMETER_MASTER_VOLUME)
		{
			VoiceAllocatorConfigure(&plugin->allocator,
			                        sParameter(plugin, static_cast<int>(PARAMETER_VELOCITY_VOLUME_MODULATION)), //
			                        sParameter(plugin, static_cast<int>(PARAMETER_VELOCITY_TONE_MODULATION)),   //
			                        sParameter(plugin, static_cast<int>(PARAMETER_VELOCITY_REFERENCE)),         //
			                        sParameter(plugin, static_cast<int>(PARAMETER_LIMITER_DECAY)),              //
			                        sVolumeParameter(plugin, static_cast<int>(PARAMETER_MASTER_VOLUME)));
			return; // Nothing more to do
		}

		// Handle volume parameters
		const float v =
		    ExponentialVolumeEasing((static_cast<float>(param_event->value) / s_parameters_info[index].max_value));

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

static clap_process_status sPluginProcess(const clap_plugin* plugin_, const clap_process* process)
{
	MatsuriPlugin* plugin = (MatsuriPlugin*)(plugin_->plugin_data);

	assert(process->audio_outputs_count == 1);
	assert(process->audio_inputs_count == 0);

	const uint32_t frames = process->frames_count;
	const uint32_t input_events = process->in_events->size(process->in_events);
	uint32_t event_index = 0;
	uint32_t next_event_frame = input_events ? 0 : frames;

	if (plugin->parameters_changed_offline != 0) // Outside code changed our guys (changed them not using events)
	{
		plugin->parameters_changed_offline = 0;

		VoiceAllocatorConfigure(&plugin->allocator,
		                        sParameter(plugin, static_cast<int>(PARAMETER_VELOCITY_VOLUME_MODULATION)), //
		                        sParameter(plugin, static_cast<int>(PARAMETER_VELOCITY_TONE_MODULATION)),   //
		                        sParameter(plugin, static_cast<int>(PARAMETER_VELOCITY_REFERENCE)),         //
		                        sParameter(plugin, static_cast<int>(PARAMETER_LIMITER_DECAY)),              //
		                        sVolumeParameter(plugin, static_cast<int>(PARAMETER_MASTER_VOLUME)));

		VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_KICK,
		                             sVolumeParameter(plugin, static_cast<int>(PARAMETER_KICK_VOLUME)));
		VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_SNARE,
		                             sVolumeParameter(plugin, static_cast<int>(PARAMETER_SNARE_VOLUME)));
		VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_CLOSED_HAT,
		                             sVolumeParameter(plugin, static_cast<int>(PARAMETER_CLOSED_HAT_VOLUME)));
		VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_OPEN_HAT,
		                             sVolumeParameter(plugin, static_cast<int>(PARAMETER_OPEN_HAT_VOLUME)));
		VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_CYMBAL,
		                             sVolumeParameter(plugin, static_cast<int>(PARAMETER_CYMBAL_VOLUME)));
		VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_LOW_TOM,
		                             sVolumeParameter(plugin, static_cast<int>(PARAMETER_LOW_TOM_VOLUME)));
		VoiceAllocatorConfigureVoice(&plugin->allocator, TYPE_HIGH_TOM,
		                             sVolumeParameter(plugin, static_cast<int>(PARAMETER_HIGH_TOM_VOLUME)));
	}

	for (uint32_t f = 0; f < frames;)
	{
		while (event_index < input_events && next_event_frame == f)
		{
			const clap_event_header* event = process->in_events->get(process->in_events, event_index);

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

static const void* sPluginGetExtension(const clap_plugin*, const char* id)
{
	if (strcmp(id, CLAP_EXT_NOTE_PORTS) == 0)
		return &s_plugin_note_ports_extensions;
	if (strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0)
		return &s_plugin_audio_ports_extensions;
	if (strcmp(id, CLAP_EXT_PARAMS) == 0)
		return &s_plugin_parameters_extensions;
	if (strcmp(id, CLAP_EXT_STATE) == 0)
		return &s_plugin_state_extension;
	if (strcmp(id, CLAP_EXT_GUI) == 0)
		return &s_plugin_gui_extension;
	return nullptr;
}

static void sPluginOnMainThread(const clap_plugin*) {}

static const clap_plugin s_plugin_class = {
    /* .desc */ &s_descriptor,
    /* .plugin_data */ nullptr,
    /* .init */ sPluginInitialise,
    /* .destroy */ sPluginDestroy,
    /* .activate */ sPluginActivate,
    /* .deactivate */ sPluginDeactivate,
    /* .start_processing */ sPluginStartProcessing,
    /* .stop_processing */ sPluginStopProcessing,
    /* .reset */ sPluginReset,
    /* .process */ sPluginProcess,
    /* .get_extension */ sPluginGetExtension,
    /* .on_main_thread */ sPluginOnMainThread,
};

static const clap_plugin* sPluginCreate(const clap_plugin_factory*, const clap_host* host, const char* plugin_id)
{
	if (clap_version_is_compatible(host->clap_version) == false || strcmp(plugin_id, s_descriptor.id) != 0)
		return nullptr;

	MatsuriPlugin* plugin = (MatsuriPlugin*)(calloc(1, sizeof(MatsuriPlugin)));
	if (plugin == nullptr)
		return nullptr;

	plugin->host = host;
	plugin->plugin = s_plugin_class;
	plugin->plugin.plugin_data = plugin;
	return &plugin->plugin;
}


/////////////////////////
// clap_plugin_factory //
/////////////////////////

static uint32_t sFactoryPluginsNo(const clap_plugin_factory*)
{
	return 1;
}

static const clap_plugin_descriptor* sFactoryGetDescriptor(const clap_plugin_factory*, uint32_t index)
{
	return (index == 0) ? &s_descriptor : nullptr;
}

static const clap_plugin_factory s_factory = {
    /* get_plugin_count */ sFactoryPluginsNo,
    /* get_plugin_descriptor */ sFactoryGetDescriptor,
    /* create_plugin */ sPluginCreate,
};


////////////////
// clap_entry //
////////////////

static bool sEntryInitialisation(const char*)
{
	return true;
}

static void sEntryDeinitialisation(void) {}

static const void* sEntryGetFactory(const char* factory_id)
{
	return (strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0) ? &s_factory : nullptr;
}

extern "C"
{
	EXPORT const clap_plugin_entry clap_entry = {
	    /* .clap_version */ CLAP_VERSION_INIT,
	    /* .init */ sEntryInitialisation,
	    /* .deinit */ sEntryDeinitialisation,
	    /* .get_factory */ sEntryGetFactory,
	};

	EXPORT const char* copyright = MATSURI_COPYRIGHT;
}
