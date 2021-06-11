#include "extension.h"
#include "sigs.h"

#define TIER0_NAME	SOURCE_BIN_PREFIX "tier0" SOURCE_BIN_SUFFIX SOURCE_BIN_EXT

Cleaner g_Cleaner;
SMEXT_LINK(&g_Cleaner);

CDetour *g_pDetour = 0;

char ** g_szStrings;
int g_iStrings = 0;

#if SOURCE_ENGINE >= SE_LEFT4DEAD2
DETOUR_DECL_MEMBER4(Detour_LogDirect, LoggingResponse_t, LoggingChannelID_t, channelID, LoggingSeverity_t, severity, Color, color, const tchar *, pMessage)
{
	for(int i=0;i<g_iStrings;++i)
		if(strstr(pMessage, g_szStrings[i])!=0)
			return LR_CONTINUE;
	return DETOUR_MEMBER_CALL(Detour_LogDirect)(channelID, severity, color, pMessage);
}
#else
DETOUR_DECL_STATIC2(Detour_DefSpew, SpewRetval_t, SpewType_t, channel, char *, text)
{
	for(int i=0;i<g_iStrings;++i)
		if(strstr(text, g_szStrings[i])!=0)
			return SPEW_CONTINUE;
	return DETOUR_STATIC_CALL(Detour_DefSpew)(channel, text);
}
#endif

bool Cleaner::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	CDetourManager::Init(g_pSM->GetScriptingEngine(), 0);

	char szPath[256];
	g_pSM->BuildPath(Path_SM, szPath, sizeof(szPath), "configs/cleaner.cfg");
	FILE * file = fopen(szPath, "r");

	if(file==NULL)
	{
		snprintf(error, maxlength, "Could not read configs/cleaner.cfg.");
		return false;
	}

	int c, lines = 0;
	do
	{
		c = fgetc(file);
		if (c == '\n') ++lines;
	} while (c != EOF);

	rewind(file);

	int len;
	g_szStrings = new char*[lines];
	while(!feof(file))
	{
		g_szStrings[g_iStrings] = new char[256];
		if (fgets(g_szStrings[g_iStrings], 255, file) != NULL)
		{
			len = strlen(g_szStrings[g_iStrings]);
			if(g_szStrings[g_iStrings][len-1]=='\r' || g_szStrings[g_iStrings][len-1]=='\n')
					g_szStrings[g_iStrings][len-1]=0;
			if(g_szStrings[g_iStrings][len-2]=='\r')
					g_szStrings[g_iStrings][len-2]=0;
			++g_iStrings;
		}
	}
	fclose(file);

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

	if(!fn)
	{
		snprintf(error, maxlength, "Failed to find signature. Please contact the author.");
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
		snprintf(error, maxlength, "Failed to initialize the detours. Please contact the author.");
		return false;
	}

	g_pDetour->EnableDetour();

	return true;
}

void Cleaner::SDK_OnUnload()
{
	if(g_pDetour)
		g_pDetour->Destroy();

	delete [] g_szStrings;
}
