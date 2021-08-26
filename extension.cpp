#include "extension.h"
#include "sigs.h"

#define TIER0_NAME  SOURCE_BIN_PREFIX "tier0" SOURCE_BIN_SUFFIX SOURCE_BIN_EXT

Cleaner g_Cleaner;
SMEXT_LINK(&g_Cleaner);

CDetour *g_pDetour = 0;

char **g_szStrings;
int g_iStrings = 0;

#if SOURCE_ENGINE >= SE_LEFT4DEAD2
	DETOUR_DECL_MEMBER4(Detour_LogDirect, LoggingResponse_t, LoggingChannelID_t, channelID, LoggingSeverity_t, severity, Color, color, const tchar *, pMessage)
	{
		for (int i = 0; i < g_iStrings; ++i)
		{
			// make sure we're stripping at least 2 or more chars just in case we accidentally inhale a \0
			// also there's no reason to strip a single char ever
			if (strlen(g_szStrings[i]) >= 2 && strstr(pMessage, g_szStrings[i]) != 0)
			{
				return LR_CONTINUE;
			}
		}
		return DETOUR_MEMBER_CALL(Detour_LogDirect)(channelID, severity, color, pMessage);
	}
#else
	DETOUR_DECL_STATIC2(Detour_DefSpew, SpewRetval_t, SpewType_t, channel, char *, text)
	{
		for (int i = 0; i < g_iStrings; ++i)
		{
			// make sure we're stripping at least 2 or more chars just in case we accidentally inhale a \0
			// also there's no reason to strip a single char ever
			if (strlen(g_szStrings[i]) >= 2 && strstr(text, g_szStrings[i]) != 0)
			{
				return SPEW_CONTINUE;
			}
		}
		return DETOUR_STATIC_CALL(Detour_DefSpew)(channel, text);
	}
#endif

// https://stackoverflow.com/questions/10178700/c-strip-non-ascii-characters-from-string
static bool badChar(char c)
{
	// everything below space excluding null term and del or above
	return (c != 0 && (c < 32 || c > 126));
}

static void stripBadChars(std::string & str)
{
	// remove all chars matching our "badchar" func
	str.erase(remove_if(str.begin(),str.end(), badChar), str.end());
}

bool Cleaner::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	CDetourManager::Init(g_pSM->GetScriptingEngine(), 0);

	char szPath[256];
	g_pSM->BuildPath(Path_SM, szPath, sizeof(szPath), "configs/cleaner.cfg");

	FILE * file = fopen(szPath, "r");

	if (file == NULL)
	{
		rootconsole->ConsolePrint("[CLEANER] Could not read configs/cleaner.cfg.");
		return false;
	}

	// step thru the file char by char and log the number of lines we have
	int lines = 0;
	for (int c = fgetc(file); c != EOF; c = fgetc(file))
	{
		if (c == '\n')
		{
			++lines;
		}
	}

	rootconsole->ConsolePrint("[CLEANER] %i lines in cleaner.cfg", lines);

	rewind(file);

	// need lines+1 because the feget runs ++lines even if it's at EOF
	g_szStrings = new char*[lines+1];

	while (!feof(file))
	{
		// we don't need to have 256 chars to work with here as most strings are far smaller than that
		g_szStrings[g_iStrings] = new char[128];
		// fgets stops at n - 1 aka 127
		if (fgets(g_szStrings[g_iStrings], 128, file) != NULL)
		{
			// make things a little easier on ourselves
			std::string thisstring = g_szStrings[g_iStrings];

			// significantly more robust way of stripping evil chars from our string so we don't crash
			// when we try to strip them. this includes newlines, control chars, non ascii unicde, etc.
			stripBadChars(thisstring);

			// copy our std::string back to char*
			// Disgusting.
			char* c_thisstring = &thisstring[0];

			int len = strlen(c_thisstring);

			// don't strip tiny (including 0 len or less) strings
			if (len <= 1)
			{
				rootconsole->ConsolePrint("[CLEANER] Not stripping string on -> L%i with 1 or less length! Length: %i", g_iStrings+1, strlen(c_thisstring));
			}
			else
			{
				rootconsole->ConsolePrint("[CLEANER] Stripping string on     -> L%i: \"%s\" - length: %i", g_iStrings+1, c_thisstring, strlen(c_thisstring));
			}

			strcpy(g_szStrings[g_iStrings], c_thisstring);

			++g_iStrings;
		}
	}
	fclose(file);


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

	// we set lines to lines+1 earlier so this needs to be <=
	for (int i = 0; i <= g_iStrings; ++i)
	{
		delete [] g_szStrings[i];
	}

	delete [] g_szStrings;
}
