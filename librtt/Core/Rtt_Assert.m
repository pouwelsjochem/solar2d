//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Config.h"

#include "Core/Rtt_Macros.h"

#include "Core/Rtt_Assert.h"

// ----------------------------------------------------------------------------

Rtt_EXPORT_BEGIN

// ----------------------------------------------------------------------------

//#if defined( Rtt_MAC_ENV ) || defined( Rtt_WIN_ENV ) || defined( Rtt_IPHONE_ENV )
//	#define Rtt_VPRINTF_SUPPORTED
//#endif

#include <stdio.h>
#include <stdarg.h>
#include <Foundation/Foundation.h>

#include <wchar.h>

//
// vfprintf_utf8
//
// This gets around a deficiency in stdargs which corrupts UTF8 characters like "😃" to "üòÉ" when
// they appear in printf arguments other than the format string
//
static int vfprintf_utf8(FILE *fp, const char *format, va_list ap)
{
	long n = mbstowcs(0, format, 0);  // determine wide length of format

	if (n==-1)
	{
		return -1;
	}

	wchar_t wfmt[n+1];
	mbstowcs(wfmt, format, n+1);

	int result = vfwprintf(fp, wfmt, ap);

	va_end(ap);

	return result;
}

#ifdef NOT_USED
static int fprintf_utf8(FILE *fp, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);

	int result = vfprintf_utf8(fp, format, ap);

	va_end(ap);

	return result;
}

static int vsnprintf_utf8(char *buf, size_t bufLen, const char *format, va_list ap)
{
	long n = mbstowcs(0, format, 0);

	if (n==-1)
	{
		return -1;
	}

	wchar_t wfmt[n+1];
	mbstowcs(wfmt, format, n+1);

	size_t wbuflen = 10240;
	wchar_t wbuf[10204];

	int result = vswprintf(wbuf, wbuflen, wfmt, ap);

	wcstombs(buf, wbuf, bufLen);

	va_end(ap);

	return result;
}
#endif // NOT_USED

int
Rtt_LogException( const char *format, ... )
{
	int result = 0;
    va_list ap;

    va_start( ap, format );
    result = Rtt_VLogException( format, ap );
    va_end( ap );

	return result;
}

int Rtt_VLogException_UseStdout = -1;

int
Rtt_VLogException( const char *format, va_list ap )
{
	int result = 0;

	if (Rtt_LogIsEnabled())
	{
		Rtt_InvokeLogCallback( format, ap );

		/* With general purpose functions low level functions like this to be used in C code, we can't
		 * guarantee the existence of an autorelease pool. We must create one every single time for safety.
		 */
        @autoreleasepool
		{
			// Log output generally goes to stdout.
			// Optionally, by setting a default, it can be sent to the system log.
			if (Rtt_VLogException_UseStdout == -1)
			{
				Rtt_VLogException_UseStdout = true;

#ifndef Rtt_AUTHORING_SIMULATOR
				// Debug output should always go to stdout in the Simulator, but in OS X desktop apps
				// we need to detect whether we're being run
				// from the command line so we direct the output to either the terminal or the system console
				// (shells set the environment variable "_" to the name of the last process they started)
				NSDictionary *env = [[NSProcessInfo processInfo] environment];
				NSString* underscore = [env objectForKey:@"_"];

				if (underscore == nil || [underscore isEqualToString:@"/usr/bin/open"])
				{
					Rtt_VLogException_UseStdout = false;
				}
#endif // Rtt_AUTHORING_SIMULATOR

				if ([[NSUserDefaults standardUserDefaults] boolForKey:@"useSystemLog"])
				{
					Rtt_VLogException_UseStdout = false;
				}
			}

			if (Rtt_VLogException_UseStdout)
			{
				if (isatty(STDOUT_FILENO))
				{
					// Output a timestamp
					NSDateFormatter *dateFormat = [[[NSDateFormatter alloc] init] autorelease];
					[dateFormat setDateFormat:@"MMM dd HH:mm:ss.SSS: "];
					NSString *timestamp = [dateFormat stringFromDate:[NSDate date]];

					fputs([timestamp UTF8String], stdout);
				}

				// Include the process name if it's not "Corona Simulator" (distinguishes output
				// from OS X apps run from build dialog)
				NSString *processName = [[NSProcessInfo processInfo] processName];

				if (! [processName isEqualToString:@"Corona Simulator"])
				{
					fputs([processName UTF8String], stdout);
					fputs(": ", stdout);
				}

				result = vfprintf_utf8(stdout, format, ap);

				// For result to be greater than 0, format must be at least one character long
				if (result == 0 || format[strlen(format)-1] != '\n')
				{
					fputs("\n", stdout);
				}

				fflush(stdout);
			}
			else
			{
				NSString *fmtStr = [NSString stringWithUTF8String:format];

				if (fmtStr == nil)
				{
					fmtStr = [NSString stringWithCString:format encoding:NSASCIIStringEncoding];
				}

				NSLogv(fmtStr, ap);
			}
		}
	}

	return result;
}

int
Rtt_Log( const char *format, ... )
{
	int result = 0;
    va_list ap;

    va_start( ap, format );
    result = Rtt_VLogException( format, ap );
    va_end( ap );

	return result;
}

// ----------------------------------------------------------------------------

Rtt_EXPORT_END

// ----------------------------------------------------------------------------
