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
		SetBitmapFrame(sequence->GetSheetFrameIndexForEffectiveFrameIndex(0));
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

int SpriteObject::CalculateEffectiveFrameIndexForPlayTime(Real playTime, SpriteSequence *sequence, int effectiveNumFrames) {
	if (sequence->GetTimePerFrameArray() == NULL) {
		return (int)Rtt_RealDiv(playTime, sequence->GetTimePerFrame());
	} else if (playTime < fTimePerFrameArrayCachedNextFrameTime) {
		return fTimePerFrameArrayCachedFrameIndex;
	} else {
		int numFrames = sequence->GetNumFrames();
		Real *timePerFrameArray = sequence->GetTimePerFrameArray();
		
		// Increase cachedFrame until dt is lower than cachedNextFrameTime again OR effectiveNumFrames is reached when using finite loops
		for (int i = fTimePerFrameArrayCachedFrameIndex; (0 == sequence->GetLoopCount() && playTime > fTimePerFrameArrayCachedNextFrameTime) || i < effectiveNumFrames; ++i) {
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
		
	SpriteSequence *sequence = GetCurrentSequence();
	int effectiveNumFrames = sequence->GetEffectiveNumFrames();
	
	Real playTime = Rtt_IntToReal((U32)(milliseconds - fStartTime));
	if (!Rtt_RealIsOne(fTimeScale)) {
		playTime = Rtt_RealMul(playTime, fTimeScale);
	}
	
	int effectiveFrameIndexForPlayTime = CalculateEffectiveFrameIndexForPlayTime(playTime, sequence, effectiveNumFrames);
	if (effectiveFrameIndexForPlayTime > fCurrentEffectiveFrameIndex) {
		int loopCount = sequence->GetLoopCount();
		for (int i = fCurrentEffectiveFrameIndex; i < effectiveFrameIndexForPlayTime; i++) {
			int nextEffectiveFrameIndex = i + 1;

			int previousSequenceFrameIndex = sequence->GetFrameIndexForEffectiveFrameIndex(fCurrentEffectiveFrameIndex);
			int sequenceFrameIndex = sequence->GetFrameIndexForEffectiveFrameIndex(nextEffectiveFrameIndex);
			if (sequenceFrameIndex > previousSequenceFrameIndex) {
				fCurrentEffectiveFrameIndex = nextEffectiveFrameIndex;
				SetBitmapFrame(sequence->GetSheetFrameIndexForEffectiveFrameIndex(nextEffectiveFrameIndex));
				if (HasListener(kSpriteListener)) {
					DispatchEvent(L, SpriteEvent(*this,  SpriteEvent::kNext));
				}
			} else if (loopCount == 0 || CalculateLoopCountForEffectiveFrameIndex(nextEffectiveFrameIndex) < loopCount) {
				fCurrentEffectiveFrameIndex = nextEffectiveFrameIndex;
				if (loopCount == 0 && sequence->GetTimePerFrameArray() != NULL) {
					fTimePerFrameArrayCachedFrameIndex = 0;
				}
				SetBitmapFrame(sequence->GetSheetFrameIndexForEffectiveFrameIndex(nextEffectiveFrameIndex));
				if (HasListener(kSpriteListener)) {
					DispatchEvent(L, SpriteEvent(*this,  SpriteEvent::kLoop));
				}
			} else {
				fStartTime = 0;
				SetProperty(kIsPlayingEnded, true);
				if (HasListener(kSpriteListener)) {
					DispatchEvent(L, SpriteEvent(*this,  SpriteEvent::kEnded));
				}
				break; // effectiveFrameIndexForPlayTime can increase beyond effectiveNumFrames, which would cause the for loop to trigger again. 
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
				DispatchEvent(L, SpriteEvent(*this,  SpriteEvent::kBegan));
			}
		} else {
			fStartTime -= fPlayTimeAtPause;
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

void SpriteObject::SetSequence(const char *name) {
	SpriteSequence *sequence = GetCurrentSequence();
	if (name && Rtt_StringCompare(name, sequence->GetName()) != 0) { // Find sequence since current sequence at 'index' does not match
		for (int i = 0, iMax = fSequences.Length(); i < iMax; i++) {
			if (Rtt_StringCompare(name, fSequences[i]->GetName()) == 0) {					
				fCurrentSequenceIndex = i;
				break;
			}
		}
		Reset();
	}
}

SpriteSequence* SpriteObject::GetCurrentSequence() const {
	return fSequences[fCurrentSequenceIndex];
}

int SpriteObject::GetNumSequences() const {
	return fSequences.Length();
}

int SpriteObject::GetCurrentEffectiveFrameIndex() const {
	return fCurrentEffectiveFrameIndex;
}

void SpriteObject::SetEffectiveFrame(int effectiveFrameIndex) {
	SpriteSequence *sequence = GetCurrentSequence();
	
	Real playTimeForTargetFrame = sequence->CalculatePlayTimeForEffectiveFrameIndex(effectiveFrameIndex);
	if (!Rtt_RealIsOne(fTimeScale)) {
		playTimeForTargetFrame = Rtt_RealDiv(playTimeForTargetFrame, fTimeScale);
	}
	
	if (!IsPlaying()) {
		fPlayTimeAtPause = Rtt_RealToInt(playTimeForTargetFrame);
	} else {
		fStartTime = fPlayer.GetAnimationTime() - Rtt_RealToInt(playTimeForTargetFrame);
	}
	
	fCurrentEffectiveFrameIndex = effectiveFrameIndex;
	ResetTimePerFrameArrayIteratorCacheFor(sequence);
	SetBitmapFrame(sequence->GetSheetFrameIndexForEffectiveFrameIndex(effectiveFrameIndex));
}

int SpriteObject::CalculateLoopCountForEffectiveFrameIndex(int effectiveFrameIndex) const {
	return Rtt_RealToInt(Rtt_RealDiv(effectiveFrameIndex, GetCurrentSequence()->GetNumFrames()));
}

int SpriteObject::GetCurrentLoopCount() const {
	return CalculateLoopCountForEffectiveFrameIndex(fCurrentEffectiveFrameIndex);
}

void SpriteObject::SetProperty(PropertyMask mask, bool value) {
	const Properties p = fProperties;
	fProperties = (value ? p | mask : p & ~mask);
}

bool SpriteObject::IsPlaying() const {
	return fStartTime > 0;
}

void SpriteObject::SetTimeScale(Real newTimeScale) {
	Real playTimeForCurrentFrame = GetCurrentSequence()->CalculatePlayTimeForEffectiveFrameIndex(fCurrentEffectiveFrameIndex);
	if (!Rtt_RealIsOne(newTimeScale)) {
	  	playTimeForCurrentFrame = Rtt_RealDiv(playTimeForCurrentFrame, newTimeScale);
	}
	if (!IsPlaying()) {
	  	fPlayTimeAtPause = Rtt_RealToInt(playTimeForCurrentFrame);
	} else {
		fStartTime = fPlayer.GetAnimationTime() - Rtt_RealToInt(playTimeForCurrentFrame);
	}
  	fTimeScale = newTimeScale;
}

void SpriteObject::Reset() {
	fStartTime = 0;
	fPlayTimeAtPause = 0;
	fCurrentEffectiveFrameIndex = 0;
	fProperties = 0;
	
	// Set to initial frame
	SpriteSequence *sequence = GetCurrentSequence();
	ResetTimePerFrameArrayIteratorCacheFor(sequence);
	SetBitmapFrame(sequence->GetSheetFrameIndexForEffectiveFrameIndex(0));
}

// ----------------------------------------------------------------------------

}  // namespace Rtt

// ----------------------------------------------------------------------------
