/*
    Stereo widener plugin for DeaDBeeF
    Copyright (C) 2010 Steven McDonald <steven.mcdonald@libremail.me>
    See COPYING file for modification and redistribution conditions.
*/

#include <deadbeef/deadbeef.h>

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

// Simply transforming samples causes centre-panned
// instruments to sound quiet and distant. We correct
// for this by assigning weights to the effect on the
// mid and side channels.
#define MIDWEIGHT 0.2
#define SIDEWEIGHT 1.0

static DB_dsp_t plugin;
DB_functions_t *deadbeef;

static short enabled = 0;
static float width = 1;
static float lsample, rsample, mid, side, midamp, sideamp;

static void
stereo_widener_reset (void);

static void
recalc_amp () {
    midamp = 1 - (((width * MIDWEIGHT) + 1) / 2);
    sideamp = ((width * SIDEWEIGHT) + 1) / 2;

    // apply corrective gain (prevents clipping)
    float gain = midamp > sideamp ? 0.5 / midamp : 0.5 / sideamp;
    gain = gain > 1 ? 1 : gain;
    midamp = gain * midamp;
    sideamp = gain * sideamp;

    return;
}

static int
stereo_widener_on_configchanged (DB_event_t *ev, uintptr_t data) {
    int e = deadbeef->conf_get_int ("stereo_widener.enable", 0);
    if (e != enabled) {
        if (e) {
            stereo_widener_reset ();
        }
        enabled = e;
    }

    float w = deadbeef->conf_get_float ("stereo_widener.width", 0) / 100;
    if (w > 1) { w = 1; deadbeef->conf_set_float ("stereo_widener.width", 100); }
    if (w < -1) { w = -1; deadbeef->conf_set_float ("stereo_widener.width", -100); }
    if (w != width) {
        width = w;
        recalc_amp ();
    }
    return 0;
}

static int
stereo_widener_plugin_start (void) {
    enabled = deadbeef->conf_get_int ("stereo_widener.enable", 0);
    width = deadbeef->conf_get_float ("stereo_widener.width", 0) / 100;
    if (width > 1) { width = 1; deadbeef->conf_set_float ("stereo_widener.width", 100); }
    if (width < -1) { width = -1; deadbeef->conf_set_float ("stereo_widener.width", -100); }
    recalc_amp ();
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_CONFIGCHANGED, DB_CALLBACK (stereo_widener_on_configchanged), 0);
}

static int
stereo_widener_plugin_stop (void) {
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_CONFIGCHANGED, DB_CALLBACK (stereo_widener_on_configchanged), 0);
}

static int
stereo_widener_process_int16 (int16_t *samples, int nsamples, int nch, int bps, int srate) {
    if (nch != 2 || width == 0) return nsamples;
    for (unsigned i = 0; i < nsamples; i++) {
        lsample = (float)samples[i*2];
        rsample = (float)samples[(i*2) + 1];
        mid = midamp * (lsample + rsample);
        side = sideamp * (lsample - rsample);
        samples[i*2] = (int16_t)(mid + side);
        samples[i*2 + 1] = (int16_t)(mid - side);
    }
    return nsamples;
}

static void
stereo_widener_reset (void) {
    return;
}

static void
stereo_widener_enable (int e) {
    if (e != enabled) {
        deadbeef->conf_set_int ("stereo_widener.enable", e);
        if (e && !enabled) {
            stereo_widener_reset ();
        }
        enabled = e;
    }
    return;
}

static int
stereo_widener_enabled (void) {
    return enabled;
}

static const char settings_dlg[] =
    "property \"Enable\" checkbox stereo_widener.enable 0;\n"
    "property \"Effect intensity\" hscale[-100,100,1] stereo_widener.width 0;\n"
;

static DB_dsp_t plugin = {
    .plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.api_vminor = DB_API_VERSION_MINOR,
    .plugin.type = DB_PLUGIN_DSP,
    .plugin.id = "stereo_widener",
    .plugin.name = "Stereo widener",
    .plugin.descr = "Stereo widener plugin",
    .plugin.author = "Steven McDonald",
    .plugin.email = "steven.mcdonald@libremail.me",
    .plugin.website = "http://gitorious.org/deadbeef-sm-plugins/pages/Home",
    .plugin.start = stereo_widener_plugin_start,
    .plugin.stop = stereo_widener_plugin_stop,
    .plugin.configdialog = settings_dlg,
    .process_int16 = stereo_widener_process_int16,
    .reset = stereo_widener_reset,
    .enable = stereo_widener_enable,
    .enabled = stereo_widener_enabled,
};

DB_plugin_t *
stereo_widener_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
