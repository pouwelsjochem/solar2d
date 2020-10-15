//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////
#include "Core/Rtt_Build.h"
#include "Display/Rtt_SpriteSequence.h"

#include "Core/Rtt_Time.h"
#include "Display/Rtt_ImageFrame.h"
#include "Core/Rtt_Array.h"
#include "Rtt_Lua.h"
#include "Rtt_LuaAux.h"
#include "Rtt_LuaProxyVTable.h"

// ----------------------------------------------------------------------------

namespace Rtt {

// ----------------------------------------------------------------------------
SpriteSequence *
SpriteSequence::Create(Rtt_Allocator *allocator, lua_State *L, int numFramesInSheet) {
	const char kDefaultSequenceName[] = "";
	
	SpriteSequence *result = NULL;
	
	// Canonicalize the index to positive value
	int index = lua_gettop(L);
	
	lua_getfield(L, index, "name");
	const char *name = lua_tostring(L, -1);
	if (!name) {
		name = kDefaultSequenceName;
	}
	lua_pop(L, 1);
	
	lua_getfield(L, index, "loopCount");
	int loopCount = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, index, "start");
	int start = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);
	
	FrameIndex *frames = NULL;
	int numFrames;
	if (start > 0) {
		lua_getfield(L, index, "count");
		numFrames = (int)lua_tointeger(L, -1);
		lua_pop(L, 1);
	} else {
		lua_getfield(L, index, "frames");
		if (!lua_istable(L, -1)) {
			start = 1;
			numFrames = numFramesInSheet;
		} else {
			numFrames = (int)lua_objlen(L, -1);
			frames = (FrameIndex *)Rtt_MALLOC(allocator, numFrames * sizeof(FrameIndex));
			for (int i = 0; i < numFrames; i++) {
				lua_rawgeti(L, -1, i + 1);  // Lua is 1-based
				FrameIndex value = lua_tointeger(L, -1);
				frames[i] = (value - 1);  // Lua is 1-based
				lua_pop(L, 1);
			}
		}
		lua_pop(L, 1);
	}
	
	Real timePerFrame = 0;
	Real *timePerFrameArray = NULL;
	lua_getfield(L, index, "timePerFrame");
	if (lua_isnumber(L, -1)) {
		timePerFrame = luaL_toreal(L, -1);
	} else if (lua_istable(L, -1)) {
		int numFramesInTimePerFrameArray = (int)lua_objlen(L, -1);
		timePerFrameArray = (Real *)Rtt_MALLOC(allocator, numFrames * sizeof(Real));
		
		for (int i = 0; i < Min(numFrames, numFramesInTimePerFrameArray); i++)  // Resolve timePerFrameArray with available values in given lua array
		{
			lua_rawgeti(L, -1, i + 1);  // Lua is 1-based
			int value = (int)lua_tointeger(L, -1);
			if (value < 1) {
				Rtt_TRACE_SIM(("WARNING: Invalid value(%d) in 'time' array. Assuming the frame's time is 1\n", value));
				value = 1;
			}
			timePerFrameArray[i] = value;
			lua_pop(L, 1);
		}
		
		if (numFramesInTimePerFrameArray > numFrames) {
			Rtt_TRACE_SIM(("WARNING: Size of 'time' array (%d) in sequenceData differs from number of frames(%d). 'time' array will be cropped.\n", numFramesInTimePerFrameArray, numFrames));
		} else if (numFramesInTimePerFrameArray < numFrames)  // If given lua array was smaller, repeat last frame
		{
			Rtt_TRACE_SIM(("WARNING: Size of 'time' array (%d) in sequenceData differs from number of frames(%d). 'time' array will be extended with last frame time.\n", numFramesInTimePerFrameArray, numFrames));
			for (int i = numFramesInTimePerFrameArray - 1; i < numFrames; i++) {
				timePerFrameArray[i] = timePerFrameArray[numFramesInTimePerFrameArray - 1];
			}
		}
	}
	lua_pop(L, 1);
	
	if (timePerFrame == 0 && timePerFrameArray == NULL) {
		lua_getfield(L, index, "duration");
		Real duration = (int)lua_tonumber(L, -1);
		timePerFrame = Rtt_RealDiv(duration, Rtt_IntToReal(numFrames));
		lua_pop(L, 1);
	}

	if (start > 0) {
		result = Rtt_NEW(allocator, SpriteSequence(allocator, name, timePerFrame, timePerFrameArray, start - 1, numFrames, loopCount));
	} else if (frames != NULL) {
		result = Rtt_NEW(allocator, SpriteSequence(allocator, name, timePerFrame, timePerFrameArray, frames, numFrames, loopCount));
	} else {
		Rtt_TRACE_SIM(("ERROR: sequenceData missing data. One of the following must be supplied: a pair of properties 'start'/'count' or an array value for the property 'frames'.\n"));
	}
	
	return result;
}

// ----------------------------------------------------------------------------

SpriteSequence::SpriteSequence(
										   Rtt_Allocator *allocator,
										   const char *name,
										   Real timePerFrame,
										   Real *timePerFrameArray,
										   FrameIndex start,
										   FrameIndex numFrames,
										   int loopCount)
: fName(allocator, name),
fTimePerFrameArray(timePerFrameArray),
fTimePerFrame(timePerFrame),
fNumFrames(numFrames),
fStart(start),
fFrames(NULL),
fLoopCount(loopCount) {
}

// Assumes ownership of 'frames' and assumes alloc'd via Rtt_MALLOC
SpriteSequence::SpriteSequence(
										   Rtt_Allocator *allocator,
										   const char *name,
										   Real timePerFrame,
										   Real *timePerFrameArray,
										   FrameIndex *frames,
										   FrameIndex numFrames,
										   int loopCount)
: fName(allocator, name),
fTimePerFrameArray(timePerFrameArray),
fTimePerFrame(timePerFrame),
fStart(-1),
fFrames(frames),
fNumFrames(numFrames),
fLoopCount(loopCount) {
}

SpriteSequence::~SpriteSequence() {
	Rtt_FREE(fFrames);
	Rtt_FREE(fTimePerFrameArray);
}

int SpriteSequence::CalculatePlayTimeForEffectiveFrameIndex(int frameIndex) const {
	Real *timePerFrameArray = GetTimePerFrameArray();
	if (timePerFrameArray == NULL) {
		return frameIndex * fTimePerFrame;
	} else {
		Real summedTime = 0;
		for (int i = 0; i < frameIndex; ++i) {
			summedTime += timePerFrameArray[i];
		}
		return summedTime;
	}
}

SpriteSequence::FrameIndex
SpriteSequence::GetFrameIndexForEffectiveFrameIndex(int effectiveFrameIndex) const {
	effectiveFrameIndex = effectiveFrameIndex % fNumFrames;
	if (effectiveFrameIndex >= fNumFrames) {
		effectiveFrameIndex = effectiveFrameIndex % fNumFrames;
	}
	return effectiveFrameIndex;
}

SpriteSequence::FrameIndex
SpriteSequence::GetSheetFrameIndexForEffectiveFrameIndex(int effectiveFrameIndex) const {
	effectiveFrameIndex = effectiveFrameIndex % fNumFrames;
	if (effectiveFrameIndex >= fNumFrames) {
		effectiveFrameIndex = effectiveFrameIndex % fNumFrames;
	}
	return (IsConsecutiveFrames() ? (fStart + effectiveFrameIndex) : fFrames[effectiveFrameIndex]);
}

int SpriteSequence::GetEffectiveNumFrames() const {
	return fLoopCount == 0 ? fNumFrames : fNumFrames * fLoopCount;
}

// ----------------------------------------------------------------------------

}  // namespace Rtt

// ----------------------------------------------------------------------------
