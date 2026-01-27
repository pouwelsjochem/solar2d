//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"
#include "Rtt_GameControllerDB.h"
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <iomanip>

namespace Rtt
{

GameControllerDB* GameControllerDB::sInstance = nullptr;

GameControllerDB::GameControllerDB()
{
}

GameControllerDB::~GameControllerDB()
{
	Clear();
}

GameControllerDB* GameControllerDB::GetInstance()
{
	if (!sInstance)
	{
		sInstance = new GameControllerDB();
	}
	return sInstance;
}

bool GameControllerDB::LoadFromFile(const char* filePath)
{
	if (!filePath)
	{
		return false;
	}

	FILE* file = fopen(filePath, "r");
	if (!file)
	{
		return false;
	}

	char line[4096];
	const char* platform = GetCurrentPlatform();
	int lineCount = 0;
	int mappedCount = 0;

	while (fgets(line, sizeof(line), file))
	{
		lineCount++;
		if (ParseLine(line, platform))
		{
			mappedCount++;
		}
	}

	fclose(file);

	// Success if we loaded at least some mappings
	return mappedCount > 0;
}

bool GameControllerDB::LoadFromString(const char* data)
{
	if (!data)
	{
		return false;
	}

	const char* platform = GetCurrentPlatform();
	std::string str(data);
	std::istringstream stream(str);
	std::string line;
	int mappedCount = 0;

	while (std::getline(stream, line))
	{
		if (ParseLine(line.c_str(), platform))
		{
			mappedCount++;
		}
	}

	return mappedCount > 0;
}

bool GameControllerDB::ParseLine(const char* line, const char* currentPlatform)
{
	if (!line || line[0] == '\0' || line[0] == '#')
	{
		return false; // Empty or comment line
	}

	// SDL gamecontrollerdb.txt format:
	// GUID,Name,mapping,mapping,...,platform:PlatformName

	// Find the first comma (separates GUID from name)
	const char* firstComma = strchr(line, ',');
	if (!firstComma)
	{
		return false;
	}

	// Extract GUID
	size_t guidLen = firstComma - line;
	if (guidLen == 0 || guidLen > 64)
	{
		return false;
	}

	std::string guid(line, guidLen);

	// Find the second comma (separates name from mappings)
	const char* secondComma = strchr(firstComma + 1, ',');
	if (!secondComma)
	{
		return false;
	}

	// Extract name
	size_t nameLen = secondComma - (firstComma + 1);
	if (nameLen == 0)
	{
		return false;
	}

	std::string name(firstComma + 1, nameLen);

	// Find platform tag
	const char* platformTag = strstr(secondComma, "platform:");
	std::string platform;

	if (platformTag)
	{
		// Skip "platform:"
		platformTag += 9;

		// Find end of platform name (comma or end of line)
		const char* platformEnd = platformTag;
		while (*platformEnd && *platformEnd != ',' && *platformEnd != '\n' && *platformEnd != '\r')
		{
			platformEnd++;
		}

		platform.assign(platformTag, platformEnd - platformTag);
	}

	// Only store mappings for the current platform (or mappings without platform tag)
	if (platform.empty() || strcmp(platform.c_str(), currentPlatform) == 0)
	{
		ControllerMapping mapping;
		mapping.guid = guid;
		mapping.name = name;
		mapping.platform = platform;

		fMappings[guid] = mapping;
		return true;
	}

	return false;
}

const GameControllerDB::ControllerMapping* GameControllerDB::FindByGUID(const char* guid) const
{
	if (!guid)
	{
		return nullptr;
	}

	auto it = fMappings.find(guid);
	if (it != fMappings.end())
	{
		return &it->second;
	}

	return nullptr;
}

std::string GameControllerDB::GenerateGUID(U16 vendorId, U16 productId, U8 busType)
{
	// SDL GUID format for USB devices:
	// Byte 0-1:   Bus type (0x03 for USB)
	// Byte 2-3:   0x0000
	// Byte 4-5:   Vendor ID (little-endian)
	// Byte 6-7:   0x0000
	// Byte 8-9:   Product ID (little-endian)
	// Byte 10-15: 0x000000000000

	std::ostringstream oss;
	oss << std::hex << std::setfill('0');

	// Bus type (little-endian)
	oss << std::setw(2) << (int)busType;
	oss << "000000";

	// Vendor ID (little-endian)
	oss << std::setw(2) << (vendorId & 0xFF);
	oss << std::setw(2) << ((vendorId >> 8) & 0xFF);
	oss << "0000";

	// Product ID (little-endian)
	oss << std::setw(2) << (productId & 0xFF);
	oss << std::setw(2) << ((productId >> 8) & 0xFF);

	// Padding
	oss << "000000000000";

	return oss.str();
}

const char* GameControllerDB::GetCurrentPlatform()
{
#if defined(Rtt_WIN_ENV)
	return "Windows";
#elif defined(Rtt_MAC_ENV)
	return "Mac OS X";
#elif defined(Rtt_LINUX_ENV)
	return "Linux";
#elif defined(Rtt_IPHONE_ENV)
	return "iOS";
#elif defined(Rtt_TVOS_ENV)
	return "tvOS";
#elif defined(Rtt_ANDROID_ENV)
	return "Android";
#elif defined(Rtt_SWITCH_ENV)
	return "Nintendo Switch";
#else
	return "Unknown";
#endif
}

void GameControllerDB::Clear()
{
	fMappings.clear();
}

} // namespace Rtt
