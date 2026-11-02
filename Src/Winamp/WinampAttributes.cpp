#include "main.h"
#include "api.h"
#include "WinampAttributes.h"
#include "PlaybackConfigGroup.h"
#include "EQConfigGroup.h"
#include "DeveloperConfigGroup.h"
#include "AccessibilityConfigGroup.h"
#include "../config/config.h"
extern Config config;


/* --- Internet --- */

/* --- Playback --- */
PlaybackConfigGroup playbackConfigGroup;
_unsigned config_audio_bits(16);
_bool config_audio_mono(false);
_bool config_audio_surround(true);
_bool config_audio_dither(true);
_bool config_replaygain(false);
_unsigned config_replaygain_mode(RG_MODE_GAIN_NOCLIP);
_unsigned config_replaygain_source(RG_SOURCE_TRACK);
_bool config_replaygain_preferred_only(false);
_float config_replaygain_non_rg_gain(-6);
_float config_replaygain_preamp(0);
_int config_playback_thread_priority(THREAD_PRIORITY_HIGHEST);

/* --- EQ --- */
EQConfigGroup eqConfigGroup;
_int config_eq_frequencies(EQ_FREQUENCIES_WINAMP);
_int config_eq_type(EQ_TYPE_4FRONT);
_bool config_eq_limiter(true);

/* --- Developer --- */
DeveloperConfigGroup developerConfigGroup;


/* --- Accessibility --- */
AccessibilityConfigGroup accessibilityConfigGroup;
_bool config_accessibility_modalbeep(false);
_bool config_accessibility_modalflash(true);
	
void RegisterConfigGroups()
{
	config.RegisterGroup(&playbackConfigGroup);
	config.RegisterGroup(&eqConfigGroup);
	config.RegisterGroup(&developerConfigGroup);
	config.RegisterGroup(&accessibilityConfigGroup);
}

