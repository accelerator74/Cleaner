#include "extension.h"
#include "sigs.h"
#include "khook.hpp"

using namespace std;

#define TIER0_NAME  SOURCE_BIN_PREFIX "tier0" SOURCE_BIN_SUFFIX SOURCE_BIN_EXT

Cleaner g_Cleaner;
SMEXT_LINK(&g_Cleaner);

unordered_set<string> szStrings;

#if defined(SIG_LINUX) || defined(SIG_WINDOWS)
KHook::Function<LoggingResponse_t, void*, LoggingChannelID_t, LoggingSeverity_t, Color, const tchar*>* g_pDetour = nullptr;
KHook::Return<LoggingResponse_t> Detour_LogDirect(void*, LoggingChannelID_t channelID, LoggingSeverity_t severity, Color color, const tchar* pMessage)
{
	for (const auto& str : szStrings)
	{
		const char* str_cstr = str.c_str();
		if (strstr(pMessage, str_cstr) != nullptr)
		{
			return { KHook::Action::Supersede, LR_CONTINUE };
		}
	}

	return { KHook::Action::Ignore };
}
#else
KHook::Function<SpewRetval_t, SpewType_t, char*>* g_pDetour = nullptr;
KHook::Return<SpewRetval_t> Detour_DefSpew(SpewType_t channel, char* text)
{
	for (const auto& str : szStrings)
	{
		const char* str_cstr = str.c_str();
		if (strstr(text, str_cstr) != nullptr)
		{
			return { KHook::Action::Supersede, SPEW_CONTINUE };
		}
	}

	return { KHook::Action::Ignore };
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
	str.erase(remove_if(str.begin(), str.end(), badChar), str.end());
}

bool Cleaner::SDK_OnLoad(char* error, size_t maxlength, bool late)
{
	char szPath[256];
	g_pSM->BuildPath(Path_SM, szPath, sizeof(szPath), "configs/cleaner.cfg");

	rootconsole->ConsolePrint("[CLEANER] Reading strings to clean from 'cleaner.cfg'");
	ifstream cleanerConfig(szPath);
	string line;
	int counter = 1;
	while (getline(cleanerConfig, line))
	{
		// significantly more robust way of stripping evil chars from our string so we don't crash
		// when we try to strip them. this includes newlines, control chars, non ascii unicde, etc.
		stripBadChars(line);

		// don't strip tiny (including 1 len or less) strings
		if (line.length() >= 2)
		{
			szStrings.insert(line);
		}
		else
		{
			rootconsole->ConsolePrint("[CLEANER] Not stripping string on -> L%i with 1 or less length! Length: %i", counter, line.length());
		}

		counter++;
	}

	rootconsole->ConsolePrint("[CLEANER] %i strings added from cleaner.cfg", szStrings.size());
	cleanerConfig.close();

	// init our detours
#if defined(SIG_LINUX) || defined(SIG_WINDOWS)
#ifdef PLATFORM_WINDOWS
	HMODULE tier0 = GetModuleHandle(TIER0_NAME);
	void* fn = memutils->FindPattern(tier0, SIG_WINDOWS, SIG_WIN_SIZE);
#elif defined PLATFORM_LINUX
	void* tier0 = dlopen(TIER0_NAME, RTLD_NOW);
	void* fn = memutils->ResolveSymbol(tier0, SIG_LINUX);
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
	fn = (void*)((intptr_t)fn + SIG_LINUX_OFFSET);
#endif
#endif

	g_pDetour = new KHook::Function<LoggingResponse_t, void*, LoggingChannelID_t, LoggingSeverity_t, Color, const tchar*>(
		reinterpret_cast<LoggingResponse_t (*)(void*, LoggingChannelID_t, LoggingSeverity_t, Color, const tchar*)>(fn),
		&Detour_LogDirect,
		nullptr
	);
#else
	void* fn = (gpointer)GetSpewOutputFunc();
	if (!fn)
	{
		rootconsole->ConsolePrint("[CLEANER] Failed to find SpewOutputFunc. Please contact the author.");
		return false;
	}

	g_pDetour = new KHook::Function<SpewRetval_t, SpewType_t, char*>(
		reinterpret_cast<SpewRetval_t (*)(SpewType_t, char*)>(fn),
		&Detour_DefSpew,
		nullptr
	);
#endif

	if (g_pDetour == nullptr)
	{
		rootconsole->ConsolePrint("[CLEANER] Failed to initialize the detours. Please contact the author.");
		return false;
	}

	return true;
}

void Cleaner::SDK_OnUnload()
{
	if (g_pDetour)
	{
		delete g_pDetour;
		g_pDetour = nullptr;
	}
}
