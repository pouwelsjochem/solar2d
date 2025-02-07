//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __Rtt_SpriteObject__
#define __Rtt_SpriteObject__

#include "Display/Rtt_RectObject.h"
#include "Display/Rtt_ImageSheet.h"
#include "Display/Rtt_SpriteSequence.h"
#include "Rtt_Event.h"

#include "Core/Rtt_Array.h"
#include "Core/Rtt_AutoPtr.h"
#include "Core/Rtt_String.h"

// ----------------------------------------------------------------------------

struct lua_State;

namespace Rtt
{

class ImageSheetPaint;
class SpriteObject;
class SpritePlayer;

// ----------------------------------------------------------------------------

class SpriteObject : public RectObject
{
	public:
		typedef RectObject Super;
		typedef SpriteObject Self;

	protected:
		typedef enum _PropertyMask
		{
			kIsPlayingEnded = 0x1,
			kIsMarkedToBeRemoved = 0x2
		}
		PropertyMask;

		typedef Properties U8;

	public:
		static SpriteObject* Create(lua_State * L, Rtt_Allocator *pAllocator, SpritePlayer& player, Display& display, Real width, Real height);

    public:
		SpriteObject(Rtt_Allocator *pAllocator, SpritePlayer& player, Real width, Real height);

	public:
		virtual ~SpriteObject();

		void Initialize();

	public:
		// Receiver takes ownership of 'sequence'
		void AddSequence(Rtt_Allocator *pAllocator, SpriteSequence *sequence );

	public:
		virtual const LuaProxyVTable& ProxyVTable() const;

	public:
		void ResetTimePerFrameArrayIteratorCacheFor(SpriteSequence *sequence);
		int CalculateEffectiveFrameIndexForPlayTime( Real playTime, SpriteSequence *sequence );

	protected:
		void SetBitmapFrame( int sheetFrameIndex );

	public:
		void Update( lua_State *L, U64 milliseconds );

	public:
		void Play( lua_State *L );
		void Pause();
		void SetSequence( const char *name, bool shouldReset );
		void SetEffectiveFrame( int effectiveFrameIndex );

	public:
		// Read-only properties
		SpriteSequence* GetCurrentSequence() const;
		int GetCurrentLoopIndex() const;
		int GetCurrentFrameIndex() const;
		int GetCurrentEffectiveFrameIndex() const;

	public:
		Real GetTimeScale() const { return fTimeScale; }
		void SetTimeScale( Real newValue );

	protected:
		bool IsProperty( PropertyMask mask ) const { return ( !! ( mask & fProperties ) ); }
		void SetProperty( PropertyMask mask, bool value );

	public:
		bool IsPlaying() const;

	public:
		bool IsMarkedToBeRemoved() const { return IsProperty( kIsMarkedToBeRemoved ); }
		void SetMarkedToBeRemoved( bool newValue ) { SetProperty( kIsMarkedToBeRemoved, newValue ); }

	protected:
		void Reset();
		void SetStartTimeOrPlayTimeAtPauseForEffectiveFrameIndex(int effectiveFrameIndex);

	private:
		PtrArray< SpriteSequence > fSequences;
		SpritePlayer& fPlayer;
		Real fTimeScale;
		int fCurrentSequenceIndex; // index into fSequences of current sequence
		int fCurrentFrameIndex; // index in sequence (resets with loopCount)
		int fCurrentEffectiveFrameIndex; // index in sequence (does not reset with loopCount, but is limited to loopCount * numFrames if loopCount > 0)
		U64 fStartTime;
		U64 fPlayTimeAtPause; // when paused, stores amount of time played
		int fTimePerFrameArrayCachedFrameIndex; // stores iterator state for SpriteSequence::CalculateEffectiveFrameIndexForPlayTime()
		Real fTimePerFrameArrayCachedNextFrameTime; // stores iterator state for SpriteSequence::CalculateEffectiveFrameIndexForPlayTime()
	
		Properties fProperties;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // __Rtt_SpriteObject__
