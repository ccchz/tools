#include <Windows.h>

#ifndef _WIN64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#define WND_ICON32 101
#define WND_ICON16 102
#define START_ICON 103
#define STOP_ICON 104
#define EXIT_BMP 105
#define RESTORE_BMP 106
#define STAB	107
#define STOB	108

#define IDD_DIALOG1                     129
#define IDC_BUTTON1                     1000
#define IDC_BUTTON2                     1001
#define IDC_RADIO1                      1002
#define IDC_RADIO2                      1003
#define IDC_COMBO1                      1004
#define IDC_START			1005
#define IDC_STOP			1006
#define IDC_STATE			1007
#define IDC_STATIC                      -1
#define ID_TIMER			1008
#define ID_SHOW				1009
#define ID_EXIT				1010


