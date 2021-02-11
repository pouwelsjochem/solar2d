//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __Rtt_SpriteSequence__
#define __Rtt_SpriteSequence__

#include "Core/Rtt_Array.h"
#include "Core/Rtt_String.h"
#include "Rtt_Lua.h"
#include "Rtt_LuaAux.h"
#include "Rtt_LuaProxyVTable.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

class SpriteSequence;

// ----------------------------------------------------------------------------

// A sequence defines which frames in the ImageSheet will be used
class SpriteSequence
{
	public:
		typedef S16 FrameIndex;

	public:
		static SpriteSequence* Create( Rtt_Allocator *allocator, lua_State *L, int numFramesInSheet);

	public:
		// Assumes ownership of 'frames'/'timePerFrameArray' and assumes alloc'd via Rtt_MALLOC
		SpriteSequence(
			Rtt_Allocator *allocator,
			const char *name,
			Real timePerFrame,
			Real *timePerFrameArray,
			FrameIndex start,
			FrameIndex *frames,
			FrameIndex numFrames,
			int loopCount);

	public:
		~SpriteSequence();

	public:
		const char* GetName() const { return fName.GetString(); }
		Real *GetTimePerFrameArray() const { return fTimePerFrameArray; }
		Real GetTimePerFrame() const { return fTimePerFrame; }

	public:
		int CalculateLoopCountForEffectiveFrameIndex( int effectiveFrameIndex ) const;
		int CalculatePlayTimeForEffectiveFrameIndex( int effectiveFrameIndex ) const;

	public:
		int GetNumFrames() const { return fNumFrames; }

	public:
		FrameIndex GetFrameIndexForEffectiveFrameIndex(int effectiveFrameIndex) const;
		FrameIndex GetSheetFrameIndexForFrameIndex(int frameIndex) const;

	public:
		bool IsConsecutiveFrames() const { return fStart >= 0; }
		int GetLoopCount() const { return fLoopCount; }

	private:
		String fName;
		Real fTimePerFrame;
		Real fTimePerFrameArrayDuration;
		Real *fTimePerFrameArray;
		FrameIndex fNumFrames;	// Raw number of frames
		FrameIndex fStart;		// Sequence is defined by consecutive frames in the sheet
		FrameIndex *fFrames;	// or an array of frame indices. 
		int fLoopCount;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // __Rtt_SpriteSequence__
