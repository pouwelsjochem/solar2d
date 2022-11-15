//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Rtt_LinuxRuntimeDelegate.h"
//#include "Rtt_Lua.h"
#include "Rtt_Runtime.h"
#include "Rtt_LuaContext.h"

// Lua Loader for flattened directories
// This allows .so plugins to load.
extern "C" int loader_Cflat(lua_State * L);

namespace Rtt
{
	/// Creates a new delegate used to receive events from the Corona runtime.
	LinuxRuntimeDelegate::LinuxRuntimeDelegate()
		: RuntimeDelegate()
	{
	}

	/// Destructor. Cleans up allocated resources.
	LinuxRuntimeDelegate::~LinuxRuntimeDelegate()
	{
	}

#pragma endregion

#pragma region Public Member Functions
	void LinuxRuntimeDelegate::DidInitLuaLibraries(const Runtime& sender) const
	{
		lua_State* L = sender.VMContext().L();
		Lua::InsertPackageLoader(L, &loader_Cflat, -1);
	}

	/// Called just before the "main.lua" file has been loaded.
	/// This is the application's opportunity to register custom APIs into Lua.
	void LinuxRuntimeDelegate::WillLoadMain(const Runtime& sender) const
	{
		lua_State* L = sender.VMContext().L();

		if (Rtt_VERIFY(const_cast<Runtime&>(sender).PushLaunchArgs(true) > 0))
		{
			// NativeToJavaBridge::GetInstance()->PushLaunchArgumentsToLuaTable(L);
			lua_pop(L, 1);
		}
	}

	void LinuxRuntimeDelegate::WillLoadConfig(const Runtime& sender, lua_State* L) const
	{
	}

	void LinuxRuntimeDelegate::DidLoadConfig(const Runtime& sender, lua_State* L) const
	{
	}

#pragma endregion
}; // namespace Rtt
