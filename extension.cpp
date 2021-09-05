#include "extension.h"
#include "sigs.h"
#include <vector>
#include <fstream>
#include <string>

using namespace std;

#define TIER0_NAME  SOURCE_BIN_PREFIX "tier0" SOURCE_BIN_SUFFIX SOURCE_BIN_EXT

Cleaner g_Cleaner;
SMEXT_LINK(&g_Cleaner);

CDetour *g_pDetour = 0;

vector<string> szStrings;

#if SOURCE_ENGINE >= SE_LEFT4DEAD2
	DETOUR_DECL_MEMBER4(Detour_LogDirect, LoggingResponse_t, LoggingChannelID_t, channelID, LoggingSeverity_t, severity, Color, color, const tchar *, pMessage)
	{
		for (int i = 0; i < szStrings.size(); ++i)
		{
			// make sure we're stripping at least 2 or more chars just in case we accidentally inhale a \0
			// also there's no reason to strip a single char ever
			if (szStrings[i].length() >= 2 && strstr(pMessage, szStrings[i].c_str()) != 0)
			{
				return LR_CONTINUE;
			}
		}
		return DETOUR_MEMBER_CALL(Detour_LogDirect)(channelID, severity, color, pMessage);
	}
#else
	DETOUR_DECL_STATIC2(Detour_DefSpew, SpewRetval_t, SpewType_t, channel, char *, text)
	{
		for (int i = 0; i < szStrings.size(); ++i)
		{
			// make sure we're stripping at least 2 or more chars just in case we accidentally inhale a \0
			// also there's no reason to strip a single char ever
			if (szStrings[i].length() >= 2 && strstr(text, szStrings[i].c_str()) != 0)
			{
				return SPEW_CONTINUE;
			}
		}
		return DETOUR_STATIC_CALL(Detour_DefSpew)(channel, text);
	}
#endif

// https://stackoverflow.com/questions/10178700/c-strip-non-ascii-characters-from-string
static bool badChar(char& c)
{
	// everything below space excluding null term and del or above
	return (c != 0 && (c < 32 || c > 126));
}

static void stripBadChars(string& str)
{
	// remove all chars matching our "badchar" func
	str.erase(remove_if(str.begin(),str.end(), badChar), str.end());
}

bool Cleaner::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	CDetourManager::Init(g_pSM->GetScriptingEngine(), 0);

	char szPath[256];
	g_pSM->BuildPath(Path_SM, szPath, sizeof(szPath), "configs/cleaner.cfg");

	rootconsole->ConsolePrint("[CLEANER] Reading strings to clean from 'cleaner.cfg'");
	ifstream cleanerConfig(szPath);
	string line;
	while (getline(cleanerConfig, line))
	{
		// significantly more robust way of stripping evil chars from our string so we don't crash
		// when we try to strip them. this includes newlines, control chars, non ascii unicde, etc.
		stripBadChars(line);

		// don't strip tiny (including 1 len or less) strings
		if (line.length() < 1)
		{
			rootconsole->ConsolePrint("[CLEANER] Not stripping string on -> L%i with 1 or less length! Length: %i", szStrings.size(), line.length());
		}
		else
		{
			rootconsole->ConsolePrint("[CLEANER] Stripping string on	 -> L%i: \"%s\" - length: %i", szStrings.size(), line.c_str(), line.length());
		}
		szStrings.push_back(line);
	}
	rootconsole->ConsolePrint("[CLEANER] %i strings added from cleaner.cfg", szStrings.size());
	cleanerConfig.close();

	// init our detours
	#if SOURCE_ENGINE >= SE_LEFT4DEAD2
		#ifdef PLATFORM_WINDOWS
			HMODULE tier0 = GetModuleHandle(TIER0_NAME);
			void * fn = memutils->FindPattern(tier0, SIG_WINDOWS, SIG_WIN_SIZE);
		#elif defined PLATFORM_LINUX
			void * tier0 = dlopen(TIER0_NAME, RTLD_NOW);
			void * fn = memutils->ResolveSymbol(tier0, SIG_LINUX);
			dlclose(tier0);
		#else
			#error "Unsupported OS"
		#endif

		if (!fn)
		{
			rootconsole->ConsolePrint("[CLEANER] Failed to find signature. Please contact the author.");
			return false;
		}
		#if defined SIG_LINUX_OFFSET
			#ifdef PLATFORM_LINUX
				fn = (void *)((intptr_t)fn + SIG_LINUX_OFFSET);
			#endif
		#endif
			g_pDetour = DETOUR_CREATE_MEMBER(Detour_LogDirect, fn);
		#else
			g_pDetour = DETOUR_CREATE_STATIC(Detour_DefSpew, (gpointer)GetSpewOutputFunc());
	#endif

	if (g_pDetour == NULL)
	{
		rootconsole->ConsolePrint("[CLEANER] Failed to initialize the detours. Please contact the author.");
		return false;
	}

	g_pDetour->EnableDetour();

	return true;
}

void Cleaner::SDK_OnUnload()
{
	if(g_pDetour)
	{
		g_pDetour->Destroy();
		g_pDetour = NULL;
	}
}
