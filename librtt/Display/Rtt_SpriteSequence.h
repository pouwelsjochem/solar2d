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
		static SpriteSequence* Create( Rtt_Allocator *allocator, lua_State *L );

	public:
		SpriteSequence(
			Rtt_Allocator *allocator,
			const char *name,
			Real time,
			Real *timeArray,
			FrameIndex start,
			FrameIndex numFrames,
			int loopCount);

		// Assumes ownership of 'frames' and assumed alloc'd via Rtt_MALLOC
		SpriteSequence(
			Rtt_Allocator *allocator,
			const char *name,
			Real time,
			Real *timeArray,
			FrameIndex *frames,
			FrameIndex numFrames,
			int loopCount);

	public:
		~SpriteSequence();

	public:
		const char* GetName() const { return fName.GetString(); }
		Real GetTime() const { return fTime; }
		Real *GetTimeArray() const { return fTimeArray; }
		Real GetTimePerFrame() const { return fTimePerFrame; }

	public:
		int CalculatePlayTimeForEffectiveFrameIndex( int frameIndex ) const;

	public:
		int GetNumFrames() const { return fNumFrames; }

	public:
		FrameIndex GetFrameIndexForEffectiveFrameIndex(int effectiveFrameIndex) const;
		FrameIndex GetSheetFrameIndexForEffectiveFrameIndex(int effectiveFrameIndex) const;

	public:
		// Returns number of frames in sequence.
		int GetEffectiveNumFrames() const;
		bool IsConsecutiveFrames() const { return fStart >= 0; }
		int GetLoopCount() const { return fLoopCount; }

	private:
		String fName;
		Real fTime;				// Length of sequence in ms
		Real *fTimeArray;
		Real fTimePerFrame;
		FrameIndex fNumFrames;	// Raw number of frames
		FrameIndex fStart;		// Sequence is defined by consecutive frames in the sheet
		FrameIndex *fFrames;	// or an array of frame indices. 
		int fLoopCount;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // __Rtt_SpriteSequence__
