//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"
#include "Rtt_ControllerTypeClassifier.h"
#include <string.h>
#include <ctype.h>

namespace Rtt
{

// Known USB Vendor IDs
static const U16 VENDOR_SONY = 0x054C;
static const U16 VENDOR_MICROSOFT = 0x045E;
static const U16 VENDOR_NINTENDO = 0x057E;
static const U16 VENDOR_VALVE = 0x28DE;
static const U16 VENDOR_LOGITECH = 0x046D;
static const U16 VENDOR_RAZER = 0x1532;
static const U16 VENDOR_8BITDO = 0x2DC8;
static const U16 VENDOR_HORI = 0x0F0D;
static const U16 VENDOR_POWERA = 0x20D6;

// Known product IDs for disambiguation
static const U16 PRODUCT_STEAM_CONTROLLER = 0x1102;
static const U16 PRODUCT_STEAM_CONTROLLER_V2 = 0x1142;
static const U16 PRODUCT_STEAM_DECK = 0x1205;

ControllerTypeClassifier::Type
ControllerTypeClassifier::ClassifyByVendorProduct(U16 vendorId, U16 productId)
{
	// Check by vendor ID first
	switch (vendorId)
	{
		case VENDOR_SONY:
			return kPlayStation;

		case VENDOR_MICROSOFT:
			return kXbox;

		case VENDOR_NINTENDO:
			return kNintendo;

		case VENDOR_VALVE:
			// All Valve controllers are Steam controllers/deck
			return kSteam;

		case VENDOR_LOGITECH:
			// Logitech makes controllers for multiple platforms
			// Check specific product IDs if needed, otherwise generic
			// Most Logitech gamepads use XInput and follow Xbox layout
			return kXbox;

		case VENDOR_RAZER:
			// Razer controllers typically follow Xbox layout
			return kXbox;

		case VENDOR_8BITDO:
			// 8BitDo controllers can emulate different types
			// Default to Nintendo since that's their most common target
			return kNintendo;

		case VENDOR_HORI:
		case VENDOR_POWERA:
			// These are third-party manufacturers
			// Could be for any platform, need name-based detection
			return kGeneric;

		default:
			return kUnknown;
	}
}

ControllerTypeClassifier::Type
ControllerTypeClassifier::ClassifyByName(const char* name)
{
	if (!name || name[0] == '\0')
	{
		return kUnknown;
	}

	// PlayStation patterns
	if (NameContains(name, "ps4") ||
		NameContains(name, "ps5") ||
		NameContains(name, "dualshock") ||
		NameContains(name, "dualsense") ||
		NameContains(name, "playstation") ||
		NameContains(name, "sony"))
	{
		return kPlayStation;
	}

	// Xbox patterns
	if (NameContains(name, "xbox") ||
		NameContains(name, "x-box") ||
		NameContains(name, "xinput") ||
		NameContains(name, "360") ||
		NameContains(name, "one") ||
		NameContains(name, "series"))
	{
		return kXbox;
	}

	// Nintendo patterns
	if (NameContains(name, "nintendo") ||
		NameContains(name, "switch") ||
		NameContains(name, "joy-con") ||
		NameContains(name, "joycon") ||
		NameContains(name, "pro controller"))
	{
		return kNintendo;
	}

	// Steam patterns
	if (NameContains(name, "steam") ||
		NameContains(name, "valve"))
	{
		return kSteam;
	}

	// If we couldn't identify it, it's generic
	return kGeneric;
}

const char*
ControllerTypeClassifier::GetTypeString(Type type)
{
	switch (type)
	{
		case kXbox:
			return "xbox";
		case kPlayStation:
			return "playstation";
		case kNintendo:
			return "nintendo";
		case kSteam:
			return "steam";
		case kGeneric:
			return "generic";
		case kUnknown:
		default:
			return "unknown";
	}
}

bool
ControllerTypeClassifier::NameContains(const char* name, const char* pattern)
{
	if (!name || !pattern)
	{
		return false;
	}

	// Simple case-insensitive substring search
	size_t nameLen = strlen(name);
	size_t patternLen = strlen(pattern);

	if (patternLen > nameLen)
	{
		return false;
	}

	for (size_t i = 0; i <= nameLen - patternLen; i++)
	{
		bool match = true;
		for (size_t j = 0; j < patternLen; j++)
		{
			if (tolower((unsigned char)name[i + j]) != tolower((unsigned char)pattern[j]))
			{
				match = false;
				break;
			}
		}
		if (match)
		{
			return true;
		}
	}

	return false;
}

} // namespace Rtt
