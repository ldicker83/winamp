#include "FormatNames.h"
#include <cstring>

namespace
{
	struct FormatName
	{
		const char *extension;
		const char *name;
	};

	// Names from libopenmpt 0.8.10's soundlib/Tables.cpp (modFormatInfo and modContainerInfo).
	// libopenmpt has no API that maps an extension to a name, and Winamp asks for a file's
	// format by extension alone, so the names are kept here. Where several formats share an
	// extension, the name covers them. Extensions a newer libopenmpt adds that aren't listed
	// here still play; they just get the generic "EXT Module" name.
	constexpr FormatName kFormatNames[] =
	{
		{ "mptm",   "OpenMPT" },
		{ "mod",    "ProTracker" },
		{ "s3m",    "Scream Tracker 3" },
		{ "xm",     "FastTracker 2" },
		{ "it",     "Impulse Tracker" },
		{ "667",    "Composer 667" },
		{ "669",    "Composer 669 / UNIS 669" },
		{ "amf",    "ASYLUM / DSMI Advanced Music Format" },
		{ "ams",    "Extreme's Tracker / Velvet Studio" },
		{ "c67",    "CDFM / Composer 670" },
		{ "cba",    "Chuck Biscuits / Black Artist" },
		{ "dbm",    "DigiBooster Pro" },
		{ "digi",   "DigiBooster" },
		{ "dmf",    "X-Tracker / DSMI Compact Advanced Music Format" },
		{ "dsm",    "DSIK / Dynamic Studio" },
		{ "dsym",   "Digital Symphony" },
		{ "dtm",    "Digital Tracker" },
		{ "etx",    "EasyTrax" },
		{ "far",    "Farandole Composer" },
		{ "fc",     "Future Composer" },
		{ "fc13",   "Future Composer" },
		{ "fc14",   "Future Composer" },
		{ "fmt",    "FM Tracker" },
		{ "fst",    "ProTracker" },
		{ "ftm",    "Face The Music" },
		{ "imf",    "Imago Orpheus" },
		{ "ims",    "Images Music System" },
		{ "ice",    "Ice Tracker" },
		{ "itp",    "Impulse Tracker Project" },
		{ "j2b",    "Galaxy Sound System" },
		{ "m15",    "Soundtracker" },
		{ "mdl",    "Digitrakker" },
		{ "med",    "OctaMED" },
		{ "mms",    "MultiMedia Sound" },
		{ "mt2",    "MadTracker 2" },
		{ "mtm",    "MultiTracker" },
		{ "mus",    "Karl Morton Music Format" },
		{ "nst",    "NoiseTracker" },
		{ "okt",    "Oktalyzer" },
		{ "plm",    "Disorder Tracker 2" },
		{ "psm",    "Epic Megagames MASI" },
		{ "pt36",   "ProTracker" },
		{ "ptm",    "PolyTracker" },
		{ "puma",   "Puma Tracker" },
		{ "rtm",    "Real Tracker 2" },
		{ "sfx",    "SoundFX" },
		{ "sfx2",   "SoundFX" },
		{ "smod",   "Future Composer" },
		{ "st26",   "SoundTracker 2.6" },
		{ "stk",    "Soundtracker" },
		{ "stm",    "Scream Tracker 2" },
		{ "stx",    "Scream Tracker Music Interface Kit" },
		{ "stp",    "Soundtracker Pro II" },
		{ "symmod", "Symphonie" },
		{ "tcb",    "TCB Tracker" },
		{ "gmc",    "Game Music Creator" },
		{ "gtk",    "Graoumf Tracker" },
		{ "gt2",    "Graoumf Tracker 1 / 2" },
		{ "ult",    "UltraTracker" },
		{ "unic",   "UNIC Tracker" },
		{ "wow",    "Mod's Grave" },
		{ "xmf",    "Astroidea XMF" },
		{ "gdm",    "General Digital Music" },
		{ "mo3",    "Un4seen MO3" },
		{ "oxm",    "OggMod FastTracker 2" },
		{ "mdz",    "Compressed ProTracker" },
		{ "mdr",    "Compressed Module" },
		{ "s3z",    "Compressed Scream Tracker 3" },
		{ "xmz",    "Compressed FastTracker 2" },
		{ "itz",    "Compressed Impulse Tracker" },
		{ "mptmz",  "Compressed OpenMPT" },
		// containers, reworded where libopenmpt's name reads badly with " Module" after it
		{ "umx",    "Unreal Music" },
		{ "xpk",    "XPK-packed" },
		{ "ppm",    "PowerPack PP20" },
		{ "mmcmp",  "MMCMP-compressed" },
	};
}

const char *GetModFormatName(const char *extension)
{
	if (!extension)
		return nullptr;

	for (const FormatName &format : kFormatNames)
	{
		if (!_stricmp(format.extension, extension))
			return format.name;
	}
	return nullptr;
}

const char *GetModFormatName(const wchar_t *extension)
{
	if (!extension)
		return nullptr;

	// extensions in the table are short and ASCII, so anything else can't match
	char narrow[8] = {};
	for (size_t i = 0; extension[i]; i++)
	{
		if (i + 1 >= sizeof(narrow) || extension[i] > 0x7F)
			return nullptr;
		narrow[i] = static_cast<char>(extension[i]);
	}
	return GetModFormatName(narrow);
}
