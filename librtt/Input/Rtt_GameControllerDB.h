//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_GameControllerDB_H__
#define _Rtt_GameControllerDB_H__

#include "Core/Rtt_Types.h"
#include <map>
#include <string>

namespace Rtt
{

/// Parses and provides access to SDL_GameControllerDB mappings.
/// This database contains controller names and button mappings for hundreds of controllers.
class GameControllerDB
{
	public:
		struct ControllerMapping
		{
			std::string guid;
			std::string name;
			std::string platform;
			// We only care about the name for controller type detection
		};

		/// Gets the singleton instance of the database
		static GameControllerDB* GetInstance();

		/// Loads the gamecontrollerdb.txt file from the given path.
		/// @param filePath Path to gamecontrollerdb.txt
		/// @return True if loaded successfully, false otherwise
		bool LoadFromFile(const char* filePath);

		/// Loads the database from a string (useful for embedded databases).
		/// @param data The database content as a string
		/// @return True if loaded successfully, false otherwise
		bool LoadFromString(const char* data);

		/// Finds a controller mapping by its SDL GUID.
		/// @param guid The SDL GUID string (e.g., "03000000de280000ff11000001000000")
		/// @return Pointer to mapping if found, nullptr otherwise
		const ControllerMapping* FindByGUID(const char* guid) const;

		/// Generates an SDL-compatible GUID from vendor and product IDs.
		/// Format: 03000000VVVVPPPP0000000000000000 (little-endian)
		/// @param vendorId USB vendor ID
		/// @param productId USB product ID
		/// @param busType Bus type (default 0x03 for USB)
		/// @return SDL GUID string
		static std::string GenerateGUID(U16 vendorId, U16 productId, U8 busType = 0x03);

		/// Clears all loaded mappings
		void Clear();

	private:
		GameControllerDB();
		~GameControllerDB();

		// Singleton instance
		static GameControllerDB* sInstance;

		// Map of GUID -> ControllerMapping
		std::map<std::string, ControllerMapping> fMappings;

		// Helper to parse a single line from the database
		bool ParseLine(const char* line, const char* currentPlatform);

		// Helper to get the current platform string
		static const char* GetCurrentPlatform();
};

} // namespace Rtt

#endif // _Rtt_GameControllerDB_H__
