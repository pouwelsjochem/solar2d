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
SpriteSequence::Create(Rtt_Allocator *allocator, lua_State *L) {
	const char kEmptyStr[] = "";
	
	SpriteSequence *result = NULL;
	
	// Canonicalize the index to positive value
	int index = lua_gettop(L);
	
	lua_getfield(L, index, "name");
	const char *name = lua_tostring(L, -1);
	if (!name) {
		name = kEmptyStr;
	}
	lua_pop(L, 1);
	
	lua_getfield(L, index, "start");
	int start = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);
	
	lua_getfield(L, index, "loopCount");
	int loopCount = (int)lua_tointeger(L, -1);
	if (loopCount < 0) {
		Rtt_TRACE_SIM(("WARNING: The 'loopCount' value(%d) cannot be negative. We'll be taking the minus sign away.\n", loopCount));
		loopCount = Abs(loopCount);
	}
	lua_pop(L, 1);
	
	FrameIndex *frames = NULL;
	int numFrames;
	if (start > 0) {
		lua_getfield(L, index, "count");
		numFrames = (int)lua_tointeger(L, -1);
		lua_pop(L, 1);
		
		if (numFrames <= 0) {
			Rtt_TRACE_SIM(("WARNING: Invalid 'count' value(%d) in sequenceData. Assuming the frame count of the sequence is 1.\n", numFrames));
			numFrames = 1;
		}
	} else {
		lua_getfield(L, index, "frames");
		if (lua_istable(L, -1)) {
			numFrames = (int)lua_objlen(L, -1);
			
			frames = (FrameIndex *)Rtt_MALLOC(allocator, numFrames * sizeof(FrameIndex));
			for (int i = 0; i < numFrames; i++) {
				lua_rawgeti(L, -1, i + 1);  // Lua is 1-based
				FrameIndex value = lua_tointeger(L, -1);
				if (value < 1) {
					Rtt_TRACE_SIM(("WARNING: Invalid value(%d) in 'frames' array.\n", value));
				}
				frames[i] = (value - 1);  // Lua is 1-based
				lua_pop(L, 1);
			}
		}
		lua_pop(L, 1);
	}
	
	Real time = 0;
	Real *timeArray = NULL;
	lua_getfield(L, index, "time");
	if (lua_isnumber(L, -1)) {
		time = luaL_toreal(L, -1);
	} else if (lua_istable(L, -1)) {
		int numFramesInTimeArray = (int)lua_objlen(L, -1);
		timeArray = (Real *)Rtt_MALLOC(allocator, numFrames * sizeof(Real));
		
		for (int i = 0; i < Min(numFrames, numFramesInTimeArray); i++)  // Resolve timeArray with available values in given lua array
		{
			lua_rawgeti(L, -1, i + 1);  // Lua is 1-based
			int value = (int)lua_tointeger(L, -1);
			if (value < 1) {
				Rtt_TRACE_SIM(("WARNING: Invalid value(%d) in 'time' array. Assuming the frame's time is 1\n", value));
				value = 1;
			}
			timeArray[i] = value;
			lua_pop(L, 1);
		}
		
		if (numFramesInTimeArray > numFrames) {
			Rtt_TRACE_SIM(("WARNING: Size of 'time' array (%d) in sequenceData differs from number of frames(%d). 'time' array will be cropped.\n", numFramesInTimeArray, numFrames));
		} else if (numFramesInTimeArray < numFrames)  // If given lua array was smaller, repeat last frame
		{
			Rtt_TRACE_SIM(("WARNING: Size of 'time' array (%d) in sequenceData differs from number of frames(%d). 'time' array will be extended with last frame time.\n", numFramesInTimeArray, numFrames));
			for (int i = numFramesInTimeArray - 1; i < numFrames; i++) {
				timeArray[i] = timeArray[numFramesInTimeArray - 1];
			}
		}
	}
	lua_pop(L, 1);
	
	if (start > 0) {
		result = Rtt_NEW(allocator, SpriteSequence(allocator, name, time, timeArray, start - 1, numFrames, loopCount));
	} else if (frames != NULL) {
		result = Rtt_NEW(allocator, SpriteSequence(allocator, name, time, timeArray, frames, numFrames, loopCount));
	} else {
		Rtt_TRACE_SIM(("ERROR: sequenceData missing data. One of the following must be supplied: a pair of properties 'start'/'count' or an array value for the property 'frames'.\n"));
	}
	
	return result;
}

// ----------------------------------------------------------------------------

SpriteSequence::SpriteSequence(
										   Rtt_Allocator *allocator,
										   const char *name,
										   Real time,
										   Real *timeArray,
										   FrameIndex start,
										   FrameIndex numFrames,
										   int loopCount)
: fName(allocator, name),
fTime(time),
fTimeArray(timeArray),
fTimePerFrame(Rtt_RealDiv(Rtt_IntToReal(time), Rtt_IntToReal(numFrames))),
fNumFrames(numFrames),
fStart(start),
fFrames(NULL),
fLoopCount(loopCount) {
	Rtt_ASSERT(loopCount >= 0);
}

// Assumes ownership of 'frames' and assumes alloc'd via Rtt_MALLOC
SpriteSequence::SpriteSequence(
										   Rtt_Allocator *allocator,
										   const char *name,
										   Real time,
										   Real *timeArray,
										   FrameIndex *frames,
										   FrameIndex numFrames,
										   int loopCount)
: fName(allocator, name),
fTime(time),
fTimeArray(timeArray),
fTimePerFrame(Rtt_RealDiv(Rtt_IntToReal(time), Rtt_IntToReal(numFrames))),
fStart(-1),
fFrames(frames),
fNumFrames(numFrames),
fLoopCount(loopCount) {
	Rtt_ASSERT(loopCount >= 0);
}

SpriteSequence::~SpriteSequence() {
	Rtt_FREE(fFrames);
	Rtt_FREE(fTimeArray);
}

int SpriteSequence::CalculatePlayTimeForEffectiveFrameIndex(int frameIndex) const {
	Real *timeArray = GetTimeArray();
	if (timeArray == NULL) {
		return frameIndex * GetTimePerFrame();
	} else {
		Real summedTime = 0;
		for (int i = 0; i < frameIndex; ++i) {
			summedTime += timeArray[i];
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
