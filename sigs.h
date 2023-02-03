#ifndef _SIGS_H_
#define _SIGS_H_

#if SOURCE_ENGINE == SE_CSGO
#define SIG_WINDOWS 		"\x55\x8B\xEC\x83\xE4\xF8\x8B\x45\x08\x83\xEC\x14"
#define SIG_WIN_SIZE		12
#define SIG_LINUX 		"LoggingSystem_Log"
#define SIG_LINUX_OFFSET	576
#elif SOURCE_ENGINE == SE_LEFT4DEAD2
#define SIG_WINDOWS 		"\x55\x8B\xEC\x83\xEC\x1C\xA1\x2A\x2A\x2A\x2A"
#define SIG_WIN_SIZE		11
#define SIG_LINUX 		"_ZN14CLoggingSystem9LogDirectEi17LoggingSeverity_t5ColorPKc"
#endif

#endif // _SIGS_H_
