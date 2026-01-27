//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_ControllerTypeClassifier_H__
#define _Rtt_ControllerTypeClassifier_H__

#include "Core/Rtt_Types.h"

namespace Rtt
{

/// Classifies game controllers into families (Xbox, PlayStation, Nintendo, etc.)
/// for the purpose of displaying appropriate button glyphs in UI.
class ControllerTypeClassifier
{
	public:
		/// Controller family types
		enum Type
		{
			kUnknown,
			kXbox,
			kPlayStation,
			kNintendo,
			kSteam,
			kGeneric
		};

		/// Classifies a controller based on USB vendor and product IDs.
		/// @param vendorId The USB vendor ID (e.g., 0x054C for Sony)
		/// @param productId The USB product ID
		/// @return The controller type classification
		static Type ClassifyByVendorProduct(U16 vendorId, U16 productId);

		/// Classifies a controller based on its product/display name.
		/// This is a fallback for when vendor/product IDs are not available.
		/// @param name The controller name (e.g., "Xbox 360 Controller", "DualShock 4")
		/// @return The controller type classification
		static Type ClassifyByName(const char* name);

		/// Converts a controller type to a string identifier suitable for Lua.
		/// @param type The controller type
		/// @return String like "xbox", "playstation", "nintendo", "steam", "generic", "unknown"
		static const char* GetTypeString(Type type);

	private:
		/// Checks if a name contains any of the given patterns (case-insensitive).
		static bool NameContains(const char* name, const char* pattern);
};

} // namespace Rtt

#endif // _Rtt_ControllerTypeClassifier_H__
