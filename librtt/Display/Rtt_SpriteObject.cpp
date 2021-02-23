//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////
#include "Core/Rtt_Build.h"
#include "Display/Rtt_SpriteObject.h"

#include "Core/Rtt_Time.h"
#include "Display/Rtt_ImageFrame.h"
#include "Display/Rtt_ImageSheet.h"
#include "Display/Rtt_ImageSheetPaint.h"
#include "Display/Rtt_ImageSheetUserdata.h"
#include "Display/Rtt_RectPath.h"
#include "Display/Rtt_SpritePlayer.h"
#include "Rtt_Lua.h"
#include "Rtt_LuaAux.h"
#include "Rtt_LuaProxyVTable.h"
#include "Core/Rtt_Array.h"

// ----------------------------------------------------------------------------

namespace Rtt {

// ----------------------------------------------------------------------------

SpriteObject *
SpriteObject::Create(Rtt_Allocator *pAllocator, SpritePlayer &player, Real width, Real height) {
	return Rtt_NEW(pAllocator, SpriteObject(pAllocator, player, width, height));
}

SpriteObject::SpriteObject(Rtt_Allocator *pAllocator, SpritePlayer &player, Real width, Real height)
: Super(RectPath::NewRect( pAllocator, width, height )),
fSequences(pAllocator),
fPlayer(player),
fTimeScale(Rtt_REAL_1),
fCurrentSequenceIndex(0),  // Default is first sequence
fCurrentFrameIndex(0),
fCurrentEffectiveFrameIndex(0),
fStartTime(0),
fPlayTimeAtPause(0),
fProperties(0) {
	SetObjectDesc("SpriteObject");  // for introspection
}

SpriteObject::~SpriteObject() {
	fPlayer.RemoveSprite(this);
}

void SpriteObject::Initialize() {
	fPlayer.AddSprite(this);
}

void SpriteObject::AddSequence(Rtt_Allocator *pAllocator, SpriteSequence *sequence) {
	if (0 == fSequences.Length()) {
		ResetTimePerFrameArrayIteratorCacheFor(sequence);
		SetBitmapFrame(sequence->GetSheetFrameIndexForFrameIndex(0));
	}
	fSequences.Append(sequence);
}

const LuaProxyVTable &
SpriteObject::ProxyVTable() const {
	return LuaSpriteObjectProxyVTable::Constant();
}

void SpriteObject::SetBitmapFrame(int sheetFrameIndex) {	
	Paint *paint = Super::GetPath().GetFill();
	ImageSheetPaint *bitmapPaint = (ImageSheetPaint *)paint->AsPaint(Paint::kImageSheet);
	bitmapPaint->SetFrame(sheetFrameIndex);
	GetPath().Invalidate(ClosedPath::kFillSourceTexture);
}

void SpriteObject::ResetTimePerFrameArrayIteratorCacheFor(SpriteSequence *sequence) {
	Real *timePerFrameArray = sequence->GetTimePerFrameArray();
	if (timePerFrameArray != NULL) {
		fTimePerFrameArrayCachedFrameIndex = 0;
		fTimePerFrameArrayCachedNextFrameTime = timePerFrameArray[0];
	}
}

// Should only be called in Update() since it uses a cache
int SpriteObject::CalculateEffectiveFrameIndexForPlayTime(Real playTime, SpriteSequence *sequence) {
	if (sequence->GetTimePerFrameArray() == NULL) {
		int loopCount = sequence->GetLoopCount();
		if (loopCount == 0) {
			return (int)Rtt_RealDiv(playTime, sequence->GetTimePerFrame());
		} else {
			return Min(loopCount * sequence->GetNumFrames(), (int)Rtt_RealDiv(playTime, sequence->GetTimePerFrame()));
		}
	} else if (playTime < fTimePerFrameArrayCachedNextFrameTime) {
		return fTimePerFrameArrayCachedFrameIndex;
	} else {
		int numFrames = sequence->GetNumFrames();
		Real *timePerFrameArray = sequence->GetTimePerFrameArray();
		
		int loopCount = sequence->GetLoopCount();
		int numFramesInAllLoops = loopCount * sequence->GetNumFrames();

		// Increase cachedFrame until dt is lower than cachedNextFrameTime again OR numFramesInAllLoops is reached when using finite loops
		for (int i = fTimePerFrameArrayCachedFrameIndex; (0 == sequence->GetLoopCount() && playTime > fTimePerFrameArrayCachedNextFrameTime) || i < numFramesInAllLoops; ++i) {
			fTimePerFrameArrayCachedFrameIndex += 1;
			
			int nextFrame = fTimePerFrameArrayCachedFrameIndex % numFrames;
			if (nextFrame >= numFrames) {
				nextFrame = 2 * (numFrames - 1) - nextFrame;
			}
			
			fTimePerFrameArrayCachedNextFrameTime += timePerFrameArray[nextFrame];
			if (playTime < fTimePerFrameArrayCachedNextFrameTime) break;
		}

		return fTimePerFrameArrayCachedFrameIndex;
	}
}

void SpriteObject::Update(lua_State *L, U64 milliseconds) {
	if (!IsPlaying()) {
		return; // Nothing to do.
	}
		
	Real playTime = Rtt_IntToReal((U32)(milliseconds - fStartTime));
	if (!Rtt_RealIsOne(fTimeScale)) {
		playTime = Rtt_RealMul(playTime, fTimeScale);
	}
	
	SpriteSequence *sequence = GetCurrentSequence();
	const char *sequenceName = sequence->GetName();
	int initialSequenceIndex = fCurrentSequenceIndex;
	
	int effectiveFrameIndexForPlayTime = CalculateEffectiveFrameIndexForPlayTime(playTime, sequence);
	if (effectiveFrameIndexForPlayTime > fCurrentEffectiveFrameIndex) {
		int loopCount = sequence->GetLoopCount();

		for (int i = fCurrentEffectiveFrameIndex; i < effectiveFrameIndexForPlayTime; i++) {
			int nextEffectiveFrameIndex = i + 1;

			int nextFrameIndex = sequence->GetFrameIndexForEffectiveFrameIndex(nextEffectiveFrameIndex);
			if (nextFrameIndex > fCurrentFrameIndex) { // next frame
				fCurrentFrameIndex = nextFrameIndex;
				fCurrentEffectiveFrameIndex = nextEffectiveFrameIndex;
				SetBitmapFrame(sequence->GetSheetFrameIndexForFrameIndex(nextFrameIndex));
				if (HasListener(kSpriteListener)) {
					DispatchEvent(L, SpriteEvent(*this,  SpriteEvent::kNext, sequenceName, fCurrentEffectiveFrameIndex, fCurrentFrameIndex, sequence->GetSheetFrameIndexForFrameIndex(fCurrentFrameIndex)));
					if (fCurrentSequenceIndex != initialSequenceIndex || fCurrentEffectiveFrameIndex != nextEffectiveFrameIndex) {
						break; // break if dispatched event executed setSequence of setFrame
					}
				}
			} else if (loopCount == 0 || sequence->CalculateLoopCountForEffectiveFrameIndex(nextEffectiveFrameIndex) < loopCount) { // first frame again
				fCurrentFrameIndex = nextFrameIndex;
				fCurrentEffectiveFrameIndex = nextEffectiveFrameIndex;
				SetBitmapFrame(sequence->GetSheetFrameIndexForFrameIndex(nextFrameIndex));
				if (HasListener(kSpriteListener)) {
					DispatchEvent(L, SpriteEvent(*this,  SpriteEvent::kLoop, sequenceName, fCurrentEffectiveFrameIndex, fCurrentFrameIndex, sequence->GetSheetFrameIndexForFrameIndex(fCurrentFrameIndex)));
					if (fCurrentSequenceIndex != initialSequenceIndex || fCurrentEffectiveFrameIndex != nextEffectiveFrameIndex) {
						break; // break if dispatched event executed setSequence of setFrame
					}
				}
			} else { // last frame ended without any loops left
				fStartTime = 0;
				SetProperty(kIsPlayingEnded, true);
				if (HasListener(kSpriteListener)) {
					DispatchEvent(L, SpriteEvent(*this,  SpriteEvent::kEnded, sequenceName, fCurrentEffectiveFrameIndex, fCurrentFrameIndex, sequence->GetSheetFrameIndexForFrameIndex(fCurrentFrameIndex)));
				}
			}
		}
	}
}

void SpriteObject::Play(lua_State *L) {	
	if (!IsPlaying()) {
		if (fPlayTimeAtPause == 0) {
			if (IsProperty(kIsPlayingEnded)) {
				Reset();
			}
			fStartTime = fPlayer.GetAnimationTime();
			if (HasListener(kSpriteListener)) {
				DispatchEvent(L, SpriteEvent(*this,  SpriteEvent::kBegan, GetCurrentSequence()->GetName(), 1, 1, GetCurrentSequence()->GetSheetFrameIndexForFrameIndex(1)));
			}
		} else {
			fStartTime = fPlayer.GetAnimationTime() - fPlayTimeAtPause;
			fPlayTimeAtPause = 0;
		}
	}
}

void SpriteObject::Pause() {
	if (IsPlaying()) {
		fPlayTimeAtPause = fPlayer.GetAnimationTime() - fStartTime;
		fStartTime = 0;
	}
}

void SpriteObject::SetSequence(const char *name, bool shouldReset) {
	SpriteSequence *originalSequence = GetCurrentSequence();
	if (name && Rtt_StringCompare(name, originalSequence->GetName()) != 0) { // Find sequence since current sequence at 'index' does not match
		for (int i = 0, iMax = fSequences.Length(); i < iMax; i++) {
			if (Rtt_StringCompare(name, fSequences[i]->GetName()) == 0) {					
				fCurrentSequenceIndex = i;

				 if (shouldReset) {
					Reset();
				 } else {
				 	SetBitmapFrame(fSequences[i]->GetSheetFrameIndexForFrameIndex(fCurrentFrameIndex));
				 }

				break;
			}
		}
	}
}

SpriteSequence* SpriteObject::GetCurrentSequence() const {
	return fSequences[fCurrentSequenceIndex];
}

int SpriteObject::GetCurrentFrameIndex() const {
	return fCurrentFrameIndex;
}

int SpriteObject::GetCurrentEffectiveFrameIndex() const {
	return fCurrentEffectiveFrameIndex;
}

void SpriteObject::SetEffectiveFrame(int effectiveFrameIndex) {
	SpriteSequence *sequence = GetCurrentSequence();
	fCurrentFrameIndex = sequence->GetFrameIndexForEffectiveFrameIndex(effectiveFrameIndex);
	fCurrentEffectiveFrameIndex = effectiveFrameIndex;
	ResetTimePerFrameArrayIteratorCacheFor(sequence);
	SetBitmapFrame(sequence->GetSheetFrameIndexForFrameIndex(fCurrentFrameIndex));
	SetStartTimeOrPlayTimeAtPauseForEffectiveFrameIndex(effectiveFrameIndex);
}

int SpriteObject::GetCurrentLoopCount() const {
	return GetCurrentSequence()->CalculateLoopCountForEffectiveFrameIndex(fCurrentEffectiveFrameIndex);
}

void SpriteObject::SetProperty(PropertyMask mask, bool value) {
	const Properties p = fProperties;
	fProperties = (value ? p | mask : p & ~mask);
}

bool SpriteObject::IsPlaying() const {
	return fStartTime > 0;
}

void SpriteObject::SetTimeScale(Real newTimeScale) {
  	fTimeScale = newTimeScale;
	SetStartTimeOrPlayTimeAtPauseForEffectiveFrameIndex(fCurrentEffectiveFrameIndex);
}

void SpriteObject::SetStartTimeOrPlayTimeAtPauseForEffectiveFrameIndex(int effectiveFrameIndex) {
	Real playTimeForEffectiveFrameIndex = GetCurrentSequence()->CalculatePlayTimeForEffectiveFrameIndex(effectiveFrameIndex);
	if (!Rtt_RealIsOne(fTimeScale)) {
	  	playTimeForEffectiveFrameIndex = Rtt_RealDiv(playTimeForEffectiveFrameIndex, fTimeScale);
	}
	if (!IsPlaying()) {
	  	fPlayTimeAtPause = Rtt_RealToInt(playTimeForEffectiveFrameIndex);
	} else {
		fStartTime = fPlayer.GetAnimationTime() - Rtt_RealToInt(playTimeForEffectiveFrameIndex);
	}
}

void SpriteObject::Reset() {
	fStartTime = 0;
	fPlayTimeAtPause = 0;
	fCurrentFrameIndex = 0;
	fCurrentEffectiveFrameIndex = 0;
	fProperties = 0;
	
	// Set to initial frame
	SpriteSequence *sequence = GetCurrentSequence();
	ResetTimePerFrameArrayIteratorCacheFor(sequence);
	SetBitmapFrame(sequence->GetSheetFrameIndexForFrameIndex(0));
}

// ----------------------------------------------------------------------------

}  // namespace Rtt

// ----------------------------------------------------------------------------
