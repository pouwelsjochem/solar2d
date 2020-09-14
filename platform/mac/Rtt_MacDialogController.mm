//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_MacDialogController.h"
#include "Rtt_MPlatform.h"
#import "AppDelegate.h"

#import <Foundation/NSBundle.h>
#import <Foundation/NSString.h>
#import <AppKit/NSAlert.h>
#import <AppKit/NSApplication.h>
#import <AppKit/NSNibLoading.h>
#import <AppKit/NSPanel.h>

// ----------------------------------------------------------------------------

@implementation DialogController

@synthesize inputs;
@synthesize userdata;
@synthesize fWindow;

-(id)initWithNibNamed:(NSString*)name
{
	return [self initWithNibNamed:name delegate:nil];
}

-(id)initWithNibNamed:(NSString*)name delegate:(id<DialogControllerDelegate>)delegate
{
	self = [super init];
	if ( self )
	{
		fDelegate = delegate;
		fSheetDelegate = nil;
		inputs = [[NSMutableDictionary alloc] init];
		userdata = NULL;
		[NSBundle loadNibNamed:name owner:self];
		[fWindow setReleasedWhenClosed:NO];
		fState = kDialogControllerNormal;
	}
	return self;
}

-(void)dealloc
{
	[fWindow release];
	[inputs release];
	[super dealloc];
}

-(NSInteger)runModal
{
	return [self runModalWithMessage:nil];
}

-(NSInteger)runModalWithMessage:(NSString *)message
{
	int response = kActionUnknown;
	if ( Rtt_VERIFY( kDialogControllerNormal == fState ) )
	{
		[fWindow center];

		if (message != nil)
		{
			[fWindow makeKeyAndOrderFront:self];
			
			NSAlert* alert = [[[NSAlert alloc] init] autorelease];
			[alert addButtonWithTitle:@"OK"];
			[alert setMessageText:message];
			[alert setAlertStyle:NSInformationalAlertStyle];
			[alert beginSheetModalForWindow:fWindow
							  modalDelegate:nil
							 didEndSelector:nil
								contextInfo:nil];
		}
		
		fState = kDialogControllerModal;
		response = (int) [[NSApplication sharedApplication] runModalForWindow:fWindow];

		// Dismiss window
		[fWindow close];

		fState = kDialogControllerNormal;
	}

	return response;
}

-(void)stopModalWithCode:(NSInteger)code
{
	[[NSApplication sharedApplication] stopModalWithCode:code];
}

-(IBAction)close:(id)sender
{
	[self stopModalWithCode:kActionClose];
}

-(void)beginSheet:(NSWindow*)parent modalDelegate:(id)delegate contextInfo:(void*)contextInfo
{
	if ( Rtt_VERIFY( kDialogControllerNormal == fState ) )
	{
		fState = kDialogControllerSheet;
		fSheetDelegate = delegate;

		// When the sheet ends, we call self's didEndSelector which calls fSheetDelegate
		// and then resets the state of the dialog controller
		SEL didEndSelector = @selector(sheetDidEnd:returnCode:contextInfo:);
		[NSApp beginSheet:fWindow modalForWindow:parent modalDelegate:self didEndSelector:didEndSelector contextInfo:contextInfo];
        [NSApp runModalForWindow:fWindow];
	}
}

-(void)sheetDidEnd:(NSWindow*)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo
{
	if ( fSheetDelegate && [fSheetDelegate respondsToSelector:@selector(sheetDidEnd:returnCode:contextInfo:)] )
	{
		[fSheetDelegate sheetDidEnd:sheet returnCode:returnCode contextInfo:contextInfo];
	}

    [self stopModalWithCode:kActionDefault];
	fSheetDelegate = nil;
	fState = kDialogControllerNormal;
}

-(BOOL)isActionEnabled:(NSInteger)code
{
	return ( ! fDelegate || ! [fDelegate respondsToSelector:@selector(isActionEnabled:forCode:)] ? YES : [fDelegate isActionEnabled:self forCode:code] );
}

-(BOOL)enabledDefault
{
	return [self isActionEnabled:kActionDefault];
}

-(BOOL)enabledAlternate
{
	return [self isActionEnabled:kActionAlternate];
}

-(BOOL)enabledOther1
{
	return [self isActionEnabled:kActionOther1];
}

-(void)handleAction:(id)sender withCode:(NSInteger)code
{
	if ( ! fDelegate || [fDelegate shouldStopModal:self withCode:code] )
	{
		if ( kDialogControllerModal == fState )
		{
			[[NSApplication sharedApplication] stopModalWithCode:code];
		}
		else
		{
			Rtt_ASSERT( kDialogControllerSheet == fState );
			[NSApp endSheet:fWindow returnCode:code];
			[fWindow orderOut:self];

			fState = kDialogControllerNormal;
		}		
	}
}

-(IBAction)actionDefault:(id)sender
{
	[self handleAction:sender withCode:kActionDefault];
}

-(IBAction)actionAlternate:(id)sender
{
	[self handleAction:sender withCode:kActionAlternate];
}

-(IBAction)actionOther1:(id)sender
{
	[self handleAction:sender withCode:kActionOther1];
}

-(IBAction)actionOther2:(id)sender
{
	[self handleAction:sender withCode:kActionOther2];
}

-(BOOL)validateValue:(id *)ioValue forKey:(NSString *)key error:(NSError **)outError
{
	if ( ! fDelegate || ! [fDelegate respondsToSelector:@selector(validateValue:forKey:error:)] )
	{
		return [super validateValue:ioValue forKey:key error:outError];
	}
	else
	{
		return [fDelegate validateValue:ioValue forKey:key error:outError];
	}
}

-(BOOL)validateValue:(id *)ioValue forKeyPath:(NSString *)inKeyPath error:(NSError **)outError
{
	if ( ! fDelegate || ! [fDelegate respondsToSelector:@selector(validateValue:forKeyPath:error:)] )
	{
		return [super validateValue:ioValue forKeyPath:inKeyPath error:outError];
	}
	else
	{
		return [fDelegate validateValue:ioValue forKeyPath:inKeyPath error:outError];
	}
}

@end


// ----------------------------------------------------------------------------

