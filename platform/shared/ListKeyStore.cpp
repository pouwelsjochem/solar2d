//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"
#include "Core/Rtt_String.h"
#include "Rtt_FileSystem.h"
#include "ListKeyStore.h"

#include <string>
#include <vector>

#if Rtt_WIN_ENV
#include "Interop/Ipc/CommandLine.h"
#endif

#if USE_JNI
#include "jniUtils.h"
#endif

#ifndef Rtt_LINUX_ENV
const char *ReplaceString(const char *c_subject, const char * c_search, const char * c_replace)
{
	std::string subject = c_subject;
	std::string search = c_search;
	std::string replace = c_replace;

	ReplaceString(subject, search, replace);

	return strdup(subject.c_str());
}

void ReplaceString(std::string& subject, const std::string& search, const std::string& replace)
{
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos)
	{
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
}
#endif

ListKeyStore::ListKeyStore() : myAliases( NULL ), myCount( 0 )
{

}

ListKeyStore::~ListKeyStore()
{
	if ( myAliases != NULL )
	{
		for ( int i = 0; i < myCount; i++ )
		{
			delete myAliases[i];
		}
		delete myAliases;
	}
}

std::string
ListKeyStore::EscapeArgument(std::string arg)
{
	std::string result = arg;

#if defined(Rtt_MAC_ENV ) || defined(Rtt_LINUX_ENV)

	// On macOS escape shell special characters in the strings by replacing single quotes with "'\''" and
	// then enclosing in single quotes
	ReplaceString(result, "'", "'\\''");	// escape single quotes
	result = "'" + result + "'";

#else

	// On Windows escape shell special characters in the strings by replacing double quotes with "\"" and
	// then enclosing in double quotes
	ReplaceString(result, "\"", "\\\"");	// escape double quotes
	result = "\"" + result + "\"";

#endif

	return result;
}

//
// ListKeyStore::IsValidKeyStore
//
// Attempting to list the keystore using the given password will tell us if the password is good
//
bool
ListKeyStore::IsValidKeyStore( const char * keyStore, const char * password )
{
	char cmdBuf[20480];
	std::string keystoreStr = EscapeArgument(keyStore);
	std::string passwordStr = EscapeArgument(password);

	const char kCmdFormat[] = "JAVA_TOOL_OPTIONS='-Duser.language=en' /usr/bin/keytool -list -keystore %s -storepass %s -rfc";

	snprintf(cmdBuf, sizeof(cmdBuf), kCmdFormat, keystoreStr.c_str(), passwordStr.c_str() );

	return system( cmdBuf );
}

bool
ListKeyStore::GetAliasList( const char * keyStore, const char * password )
{
	char cmdBuf[20480];
	std::string keystoreStr = EscapeArgument(keyStore);
	std::string passwordStr = EscapeArgument(password);

	const char kCmdFormat[] = "JAVA_TOOL_OPTIONS='-Duser.language=en' /usr/bin/keytool -list -keystore %s -storepass %s -rfc | sed -n 's/^Alias name: //p'";

	snprintf(cmdBuf, sizeof(cmdBuf), kCmdFormat, keystoreStr.c_str(), passwordStr.c_str() );

	FILE *keytoolResult = popen( cmdBuf, "r" );

	if (keytoolResult == NULL)
	{
		Rtt_LogException("ListKeyStore::GetAliasList: /usr/bin/keytool failed");
		return false;
	}

	std::vector<std::string> results;
	char buf[BUFSIZ];

	while (fgets(buf, BUFSIZ, keytoolResult) != NULL)
	{
		buf[strlen(buf) - 1] = 0; // zap trailing newline
		std::string bufStr(buf);
		results.push_back(bufStr);
	}

	pclose(keytoolResult);

	InitAliasList( (int)results.size() );

	for ( size_t i = 0; i < results.size(); i++ )
	{
		SetAlias( (int)i, results[i].c_str() );
	}

	return (results.size() > 0);
}

//
// ListKeyStore::AreKeyStoreAndAliasPasswordsValid
//
// Attempting to jarsign a fake jar file using the given keystore, alias and password will tell us if the password is good
//
bool
ListKeyStore::AreKeyStoreAndAliasPasswordsValid( const char *keyStore, const char *keyPW, const char *alias, const char *aliasPW, const char *resourcesDir)
{
	char cmdBuf[20480];
	std::string keyStoreStr = EscapeArgument(keyStore);
	std::string keyPWStr = EscapeArgument(keyPW);
	std::string aliasStr = EscapeArgument(alias);
	std::string aliasPWStr = EscapeArgument(aliasPW);
	std::string resourcesDirStr = resourcesDir;

	std::string tmpDirStr = Rtt_GetSystemTempDirectory();
	std::string srcTestJarStr = resourcesDirStr + "/_coronatest.jar";
	std::string dstTestJarStr = tmpDirStr + "/_coronatest.jar";

	if (! Rtt_CopyFile(srcTestJarStr.c_str(), dstTestJarStr.c_str()))
	{
		return -1;
	}

	Rtt_Log("Testing credentials for '%s': ", keyStore);

	const char kCmdFormat[] = "JAVA_TOOL_OPTIONS='-Duser.language=en' /usr/bin/jarsigner -tsa http://timestamp.digicert.com -keystore %s -storepass %s -keypass %s %s %s; exit $?";
	
	snprintf(cmdBuf, sizeof(cmdBuf), kCmdFormat, keyStoreStr.c_str(), keyPWStr.c_str(), aliasPWStr.c_str(), dstTestJarStr.c_str(), aliasStr.c_str() );

	return (system( cmdBuf ) == 0);

}
