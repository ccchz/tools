#include <windows.h>
#include "res.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <wingdi.h>
#include "libconfig.h"
#include <winspool.h>
#include <TlHelp32.h>
#include <sys/types.h>
#include <signal.h>
#include <winbase.h>
#include <commctrl.h>

HINSTANCE hinst;
HWND DlgHwnd;
HANDLE hmutex;

#define CFG_MAX 5
#define TMPMAX 256
#define BUFMAX 512
#define WEBPPD TEXT("webppd.exe")
#define APPCLASS TEXT("webppd")
#define WM_TRAY (WM_USER + 100)  
#define WM_TASKBAR_CREATED RegisterWindowMessage(TEXT("TaskbarCreated"))  

#define APP_NAME    TEXT("WEBPOS打印工具...")  
#define START_TIP	TEXT("启动服务")
#define STOP_TIP	TEXT("停止服务")
#define SAVE_TIP	TEXT("更新配置文件")
#define TEST_TIP	TEXT("测试设备")


MENUITEMINFO mio;
NOTIFYICONDATA nid;
HMENU hMenu;  


void InitTray(HINSTANCE hInstance, HWND hWnd)
{

	SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hinst, (LPCTSTR)WND_ICON32));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = WND_ICON16;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
	nid.uCallbackMessage = WM_TRAY;
	nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(WND_ICON16));
	lstrcpy(nid.szTip, APP_NAME);

	hMenu = CreatePopupMenu();
							
	//AppendMenu(hMenu, MF_STRING, ID_SHOW, TEXT("显示"));
	//AppendMenu(hMenu, MF_STRING, ID_EXIT, TEXT("退出"));

	mio.cbSize = sizeof(mio);
	mio.fMask = MIIM_STRING | MIIM_BITMAP|MIIM_ID;
	mio.fType = MFT_STRING;
	mio.fState = MFS_ENABLED;
	mio.wID = ID_EXIT;
	mio.dwTypeData = TEXT("退出系统");
	mio.cch = 4;
	mio.hSubMenu = NULL;
	mio.hbmpItem = LoadImage(hInstance,MAKEINTRESOURCE(EXIT_BMP),IMAGE_BITMAP,16,16,LR_LOADTRANSPARENT|LR_LOADMAP3DCOLORS);
	InsertMenuItem(hMenu, 0, TRUE, &mio);
	mio.wID = ID_SHOW;
	mio.dwTypeData = TEXT("显示窗口");
	mio.hbmpItem = LoadImage(hInstance, MAKEINTRESOURCE(RESTORE_BMP), IMAGE_BITMAP, 16, 16, LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS);
	InsertMenuItem(hMenu, 0, TRUE, &mio);


	Shell_NotifyIcon(NIM_ADD, &nid);
}
/*
void ShowTrayMsg()
{
	lstrcpy(nid.szInfoTitle, APP_NAME);
	lstrcpy(nid.szInfo, TEXT("收到一条消息！"));
	nid.uTimeout = 2000;
	Shell_NotifyIcon(NIM_MODIFY, &nid);
}
*/
void trayicon(UINT type) {

	nid.uFlags   = NIF_ICON;
	nid.hIcon = LoadIcon(hinst, MAKEINTRESOURCE(type));

	Shell_NotifyIcon(NIM_MODIFY, &nid);

}
char *str_cfg[] = { "ipaddr",
					"ipport",
					"printtype",
					"printer",
					"reader"
};


char * webppd_cfg[CFG_MAX];


static void * alloc_copy(const char * buf, size_t len) {
	char * p = malloc(len + 1);
	memset(p, 0, len + 1);
	memcpy(p, buf, len);
	p[len] = '\0';
	return p;

}

int free_array(char **array, unsigned int count) {
	unsigned int i;
	if (array) {
		for (i = 0; i< count; i++) {
			if (array[i]) {
				//printf("%s\n", array[i]);
				free(array[i]);

			}
		}
		return 0;
	}
	return 1;
}

DWORD getprocbyname(PWSTR name) {
	HANDLE hproc;
	DWORD pid;
	pid = 0;
	hproc =(HANDLE) CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	if (!hproc) return 0;
	PROCESSENTRY32 prc32;
	prc32.dwSize = sizeof(prc32);
	BOOL bHave = Process32First(hproc, &prc32);
	while (bHave)
	{
		if (wcscmp(name, prc32.szExeFile) == 0)
		{
			pid = prc32.th32ProcessID;
			break;
		}
		bHave = Process32Next(hproc, &prc32);
	}
	CloseHandle(hproc);
	return pid;
}

int procstate() {
	DWORD pid;
	pid = getprocbyname(WEBPPD);
	if (pid ) {
		EnableWindow(GetDlgItem(DlgHwnd,IDC_START), FALSE);
		EnableWindow(GetDlgItem(DlgHwnd, IDC_STOP), TRUE);
		trayicon(START_ICON);
	}else{
		EnableWindow(GetDlgItem(DlgHwnd, IDC_START), TRUE);
		EnableWindow(GetDlgItem(DlgHwnd,IDC_STOP), FALSE);
		trayicon(STOP_ICON);
	}
	return 0;
}

int procstart() {
	DWORD pid;
	STARTUPINFO si = { sizeof(si)};
	PROCESS_INFORMATION pi;
	TCHAR cmdline[] = WEBPPD;
	BOOL bRet;

	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = FALSE;
	pid = getprocbyname(WEBPPD);
	if (pid) {
		EnableWindow(GetDlgItem(DlgHwnd, IDC_START), FALSE);
		EnableWindow(GetDlgItem(DlgHwnd, IDC_STOP), TRUE);
	}
	else {
		bRet = CreateProcess(NULL, cmdline, NULL,NULL,FALSE, CREATE_NEW_CONSOLE,NULL,NULL,&si,&pi);
		if (bRet) {
			EnableWindow(GetDlgItem(DlgHwnd, IDC_START), FALSE);
			EnableWindow(GetDlgItem(DlgHwnd, IDC_STOP), TRUE);
			trayicon(START_ICON);
		}
		else
		{
			trayicon(STOP_ICON);
			MessageBox(DlgHwnd,TEXT("启动服务失败！！"), TEXT("错误提示..."), MB_OK | MB_ICONERROR);
		}
	}
	return 0;
}

int procstop() {
	DWORD pid;
	HANDLE hproc;
	pid = getprocbyname(WEBPPD);
	if (pid) {
		hproc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
		if (TerminateProcess(hproc, 0)) {
			EnableWindow(GetDlgItem(DlgHwnd, IDC_START), TRUE);
			EnableWindow(GetDlgItem(DlgHwnd, IDC_STOP), FALSE);
			trayicon(STOP_ICON);
		}
		else { 
			EnableWindow(GetDlgItem(DlgHwnd, IDC_START), FALSE);
			EnableWindow(GetDlgItem(DlgHwnd, IDC_STOP), TRUE);
			trayicon(START_ICON);
		} 

	}
	return 0;

}

void * char2unicode(char * buf) {
	int j;
	PWSTR strp;

	j = MultiByteToWideChar(CP_ACP, 0, (PSTR)buf, -1, NULL, 0);
	strp = malloc(j * sizeof(wchar_t));
	memset(strp, 0, j * sizeof(wchar_t));
	MultiByteToWideChar(CP_ACP, 0, (PSTR)buf, -1, strp, j);
	return strp;


}

void * unicode2char(PWSTR str) {
	int j;
	PSTR strp;

	j = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0,NULL,NULL);
	strp = malloc(j * sizeof(char));
	memset(strp, 0, j * sizeof(char));
	WideCharToMultiByte(CP_ACP, 0, str, -1, strp, j,NULL,NULL);
	return strp;

}

int print_msg(char *buf,HANDLE hwnd) {
	PWSTR strp;

	strp = char2unicode(buf);
	MessageBox(hwnd, strp, TEXT("提示..."), MB_OK|MB_ICONASTERISK);
	free(strp);
	return 0;
}
int updatestat() {
	char buftmp[TMPMAX];
	char buf[BUFMAX];
	PWSTR strp;

	memset(buf, 0, BUFMAX);
	if (webppd_cfg[3]) {
		memset(buftmp, 0, TMPMAX);
		sprintf_s(buftmp, TMPMAX, "当前打印机：%s \n", webppd_cfg[3]);
		strcat_s(buf,BUFMAX, buftmp);
	}

	if (webppd_cfg[4]) {
		memset(buftmp, 0, TMPMAX);
		sprintf_s(buftmp, TMPMAX, "当前读卡器：%s \n", webppd_cfg[4]);
		strcat_s(buf,BUFMAX, buftmp);
	}

	strp = char2unicode(buf);
	SetWindowText(GetDlgItem(DlgHwnd,IDC_STATE), strp);
	//MessageBox(0, strp, TEXT("错误提示..."), MB_OK);
	free(strp);
	return 0;
}
int init_conf() {

	config_t cfg;
	const char *str;
	unsigned int i;
	char buf[TMPMAX];



	config_init(&cfg);
	memset(webppd_cfg, 0, CFG_MAX * sizeof(char *));
	/* Read the file. If there is an error, report it and exit. */
	if (!config_read_file(&cfg, "webppd.conf"))
	{
			config_destroy(&cfg);
		return 1;
	}

	/* Get the store name. */
	for (i = 0; i< CFG_MAX; i++) {
		if (config_lookup_string(&cfg, str_cfg[i], &str)) {
			webppd_cfg[i] = alloc_copy(str, strlen(str));
		}
		else {
			memset(buf,0,TMPMAX);

			sprintf_s((char*)buf,TMPMAX, "配置文件中没有 '%s' 的参数！！", str_cfg[i]);
			print_msg(buf,NULL);
		}
	}

	config_destroy(&cfg);

	return 0;


}

int save_conf() {
	config_t cfg;
	config_setting_t *setting;
	char buf[TMPMAX];
	LRESULT i;
	PSTR str;

	config_init(&cfg);
	if (!config_read_file(&cfg, "webppd.conf"))
	{
		config_destroy(&cfg);
		return 1;
	}
	if (SendMessage(GetDlgItem(DlgHwnd, IDC_RADIO1), BM_GETCHECK, 0, 0) == BST_CHECKED) {
		setting = config_lookup(&cfg, str_cfg[2]);
		if (!setting) {
			setting = config_setting_add(cfg.root, str_cfg[2], CONFIG_TYPE_STRING);

		//	return 1;
		}
		sprintf_s(buf,TMPMAX, "port");
		config_setting_set_string(setting, buf);

	}
	else {
		setting = config_lookup(&cfg, str_cfg[2]);
		if (!setting) {
			setting = config_setting_add(cfg.root, str_cfg[2], CONFIG_TYPE_STRING);
			//return 1;
		}
		sprintf_s(buf,TMPMAX, "driver");
		config_setting_set_string(setting, buf);
	}

	i= SendMessage(GetDlgItem(DlgHwnd, IDC_COMBO1), CB_GETCURSEL, 0, 0);
	SendMessage(GetDlgItem(DlgHwnd, IDC_COMBO1), CB_GETLBTEXT, i, (LPARAM)buf);

	setting = config_lookup(&cfg, str_cfg[3]);
	if (!setting) {
		setting = config_setting_add(cfg.root, str_cfg[3], CONFIG_TYPE_STRING);
		//return 1;
	}
	str = unicode2char((PWSTR)buf);
	config_setting_set_string(setting,str);
	free(str);
	if (!config_write_file(&cfg, "webppd.conf")) return 1;
	config_destroy(&cfg);
	return 0;
}

int test() {
	LRESULT i;
	CHAR buf[TMPMAX];
	CHAR buf1[TMPMAX];
	CHAR testmsg[TMPMAX];
	PWSTR str;
	HANDLE hfile;
	DWORD wb;
	DOC_INFO_1 doc;

	sprintf_s(testmsg,TMPMAX, "This is test.\n这是测试。\n");
	if (SendMessage(GetDlgItem(DlgHwnd, IDC_RADIO1), BM_GETCHECK, 0, 0) == BST_CHECKED) {
		i = SendMessage(GetDlgItem(DlgHwnd, IDC_COMBO1), CB_GETCURSEL, 0, 0);
		SendMessage(GetDlgItem(DlgHwnd, IDC_COMBO1), CB_GETLBTEXT, i, (LPARAM)buf);
		memset(buf1, 0, sizeof(buf1));
		sprintf_s(buf1,TMPMAX, "\\\\.\\");
		str = char2unicode(buf1);
		memset(buf1, 0, sizeof(buf1));
		wcscat((wchar_t *)buf1, str);
		free(str);
		wcscat((wchar_t *)buf1,(wchar_t*) buf);
		//MessageBox(DlgHwnd, (LPCWSTR)buf1, TEXT("提示..."), MB_OK);
		hfile = CreateFile((LPCWSTR)buf1, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hfile == INVALID_HANDLE_VALUE) {
			MessageBox(DlgHwnd, TEXT("端口打开失败！！"), TEXT("提示..."), MB_OK | MB_ICONERROR);
			return 1;
		}

		WriteFile(hfile, testmsg, (DWORD)strlen(testmsg), &wb, NULL);
		CloseHandle(hfile);

	}
	else {
		i = SendMessage(GetDlgItem(DlgHwnd, IDC_COMBO1), CB_GETCURSEL, 0, 0);
		SendMessage(GetDlgItem(DlgHwnd, IDC_COMBO1), CB_GETLBTEXT, i, (LPARAM)buf);

		//MessageBox(DlgHwnd, (LPCWSTR)buf, TEXT("提示..."), MB_OK);
		if (!OpenPrinter((LPTSTR)buf, &hfile, 0)) {
			MessageBox(DlgHwnd, TEXT("打印机打开失败！！"), TEXT("提示..."), MB_OK | MB_ICONERROR);
			return 1;
		}
		doc.pDatatype = TEXT("TEXT");
		doc.pDocName = NULL;
		doc.pOutputFile = NULL;
		if (!StartDocPrinter(hfile, 1,(LPBYTE) &doc)) {
			MessageBox(DlgHwnd, TEXT("打印失败！！"), TEXT("提示..."), MB_OK | MB_ICONERROR);
			ClosePrinter(hfile);
			return 1;
		}
		if (! StartPagePrinter(hfile)){
			MessageBox(DlgHwnd, TEXT("打印失败！！"), TEXT("提示..."), MB_OK | MB_ICONERROR);
			ClosePrinter(hfile);
			return 1;
		}
		WritePrinter(hfile, testmsg, (DWORD)strlen(testmsg), &wb);
		EndPagePrinter(hfile);
		EndDocPrinter(hfile);
		ClosePrinter(hfile);
	}
	return 0;
}

int init_print(int type) {
	LPBYTE port;
	DWORD pneed;
	DWORD pret;
	PRINTER_INFO_2 *pport;
	DWORD i;
	port = 0;

	EnumPrinters(PRINTER_ENUM_CONNECTIONS | PRINTER_ENUM_LOCAL, NULL, 2, NULL, 0, &pneed, &pret);

	port = malloc(pneed+4);
	memset(port, 0, pneed);

	EnumPrinters(PRINTER_ENUM_CONNECTIONS|PRINTER_ENUM_LOCAL, NULL, 2, port, pneed, &pneed, &pret);


		pport = (PRINTER_INFO_2 *)port;
		if (pret > 0) {
			SendMessage(GetDlgItem(DlgHwnd, IDC_COMBO1), CB_RESETCONTENT, 0, 0);
			for (i = 0; i < pret; i++) {
				SendMessage(GetDlgItem(DlgHwnd, IDC_COMBO1), CB_ADDSTRING, 0,(LPARAM) pport->pPrinterName);
				pport++;
			}
			SendMessage(GetDlgItem(DlgHwnd, IDC_COMBO1), CB_SETCURSEL, 0, 0);
		}
	free(port);
	return 0;

}

int init_port() {
	int i;
	char buf[TMPMAX];
	PWSTR strp;


	SendMessage(GetDlgItem(DlgHwnd, IDC_COMBO1), CB_RESETCONTENT, 0, 0);
	for (i = 1; i < 8; i++) {
		buf[0] = '\0';
		if (i < 4) {
			sprintf_s(buf,TMPMAX, "LPT%d", i);
		}
		else {
			sprintf_s(buf,TMPMAX, "COM%d", i-3);
		}
		strp = char2unicode(buf);
		SendMessage(GetDlgItem(DlgHwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)strp);
		free(strp);
	}
	SendMessage(GetDlgItem(DlgHwnd, IDC_COMBO1), CB_SETCURSEL, 0, 0);
	return 0;
}

int init_ctrl() {

	if (webppd_cfg[2]) {
		if (!strcmp("port", webppd_cfg[2])) {
			SendMessage(GetDlgItem(DlgHwnd, IDC_RADIO1), BM_SETCHECK, TRUE, 0);
			init_port();
		}
		else {
			SendMessage(GetDlgItem(DlgHwnd, IDC_RADIO2), BM_SETCHECK, TRUE, 0);
			init_print(1);
		}
	}

	return 0;
}

void CreateMyTooltip() {

	INITCOMMONCONTROLSEX iccex;
	HWND hwndTT;

	TOOLINFO ti;
	//char tooltip[30] = "A main window";
	RECT rect;

	iccex.dwICC = ICC_WIN95_CLASSES;
	iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	//InitCommonControlsEx(&iccex);

	hwndTT = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP ,
		0, 0, 0, 0, DlgHwnd, NULL, NULL, NULL);

	SetWindowPos(hwndTT, HWND_TOPMOST, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	SendMessage(hwndTT, TTM_SETDELAYTIME, TTDT_AUTOPOP, 2000);
	
	GetClientRect(GetDlgItem(DlgHwnd,IDC_START), &rect);

	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_SUBCLASS;
	ti.hwnd = GetDlgItem(DlgHwnd, IDC_START);
	ti.hinst = NULL;
	ti.uId = 0;
	ti.lpszText =(LPWSTR) START_TIP;
	ti.rect.left = rect.left;
	ti.rect.top = rect.top;
	ti.rect.right = rect.right;
	ti.rect.bottom = rect.bottom;

	SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
	GetClientRect(GetDlgItem(DlgHwnd, IDC_STOP), &rect);
	ti.hwnd = GetDlgItem(DlgHwnd, IDC_STOP);
	ti.lpszText = (LPWSTR)STOP_TIP;
	ti.rect.left = rect.left;
	ti.rect.top = rect.top;
	ti.rect.right = rect.right;
	ti.rect.bottom = rect.bottom;

	SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
	SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
	GetClientRect(GetDlgItem(DlgHwnd, IDC_BUTTON1), &rect);
	ti.hwnd = GetDlgItem(DlgHwnd, IDC_BUTTON1);
	ti.lpszText = (LPWSTR)SAVE_TIP;
	ti.rect.left = rect.left;
	ti.rect.top = rect.top;
	ti.rect.right = rect.right;
	ti.rect.bottom = rect.bottom;

	SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
	SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
	GetClientRect(GetDlgItem(DlgHwnd, IDC_BUTTON2), &rect);
	ti.hwnd = GetDlgItem(DlgHwnd, IDC_BUTTON2);
	ti.lpszText = (LPWSTR)TEST_TIP;
	ti.rect.left = rect.left;
	ti.rect.top = rect.top;
	ti.rect.right = rect.right;
	ti.rect.bottom = rect.bottom;

	SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
}

INT_PTR CALLBACK maindlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		DlgHwnd = hDlg;
		updatestat();
		init_ctrl();
		InitTray(hinst, hDlg);
		SetTimer(DlgHwnd, ID_TIMER, 5000, NULL);
		procstart();
		CreateMyTooltip();
		SendMessage(hDlg, WM_SETTEXT,0,(LPARAM)APP_NAME);
		SendMessage(GetDlgItem(hDlg,IDC_START), BM_SETIMAGE,IMAGE_ICON , (LPARAM)LoadImage(hinst, MAKEINTRESOURCE(STAB), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
		SendMessage(GetDlgItem(hDlg,IDC_STOP), BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadImage(hinst, MAKEINTRESOURCE(STOB), IMAGE_ICON, 16, 16,LR_DEFAULTCOLOR) );
		return TRUE;
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED) {
			ShowWindow(hDlg, SW_HIDE);
		}
		return TRUE;
	case WM_CLOSE:
		if (MessageBox(hDlg, TEXT("是否要退出？？"), TEXT("提示..."), MB_YESNO|MB_ICONQUESTION) == IDNO) {
			return FALSE;
		}
		KillTimer(DlgHwnd, ID_TIMER);
		Shell_NotifyIcon(NIM_DELETE, &nid);
		free_array(webppd_cfg, CFG_MAX);
		EndDialog(hDlg, LOWORD(wParam));
		return TRUE;
	case WM_TIMER:
		if (LOWORD(wParam) == ID_TIMER){
			//MessageBox(0, TEXT("timer."), TEXT("提示..."), MB_OK);
			procstate();
		//	ShowTrayMsg();
		}
	case WM_COMMAND:
		if (LOWORD(wParam) == ID_SHOW) {
			ShowWindow(hDlg, SW_RESTORE);
			SetForegroundWindow(hDlg);
			break;
		}
		if (LOWORD(wParam) == IDC_START)
		{
			procstart();
			return TRUE;
		}
		if (LOWORD(wParam) == IDC_STOP)
		{
			procstop();
			return TRUE;
		}
		if (LOWORD(wParam) == IDC_RADIO1) {
			init_port();
			return TRUE;
		}
		if (LOWORD(wParam) == IDC_RADIO2) {
			init_print(1);
			return TRUE;
		}
		if (LOWORD(wParam) == IDC_BUTTON2) {
			if (MessageBox(hDlg, TEXT("是否要测试？？"), TEXT("提示..."), MB_YESNO | MB_ICONWARNING) == IDNO) {
				return FALSE;
			}
			test();
			return TRUE;
		}
		if (LOWORD(wParam) == IDC_BUTTON1) {
			if(MessageBox(hDlg, TEXT("是否要更新配置文件？？"), TEXT("提示..."), MB_YESNO | MB_ICONQUESTION) == IDNO){
				return FALSE;
				}
			if (save_conf()) {
				MessageBox(hDlg, TEXT("更新配置文件出错！！"), TEXT("提示..."), MB_OK | MB_ICONERROR);
				return FALSE;
			}
			free_array(webppd_cfg, CFG_MAX);
			if (init_conf()) {
				MessageBox(hDlg, TEXT("读取配置文件出错！！"), TEXT("提示..."), MB_OK | MB_ICONERROR);
				return FALSE;
			}
			updatestat();
			return TRUE;
		}
	case WM_TRAY:
		switch (lParam)
		{

			case WM_RBUTTONDOWN:
			{
	
			POINT pt; GetCursorPos(&pt);
			SetForegroundWindow(hDlg);
			//EnableMenuItem(hMenu, ID_SHOW, MF_GRAYED);      

			int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hDlg,
				NULL);
			if (cmd == ID_SHOW )
				PostMessage(hDlg, WM_COMMAND, ID_SHOW, 0);
			if (cmd == ID_EXIT)
				PostMessage(DlgHwnd, WM_CLOSE, 0, 0);
			}
			break;
			case WM_LBUTTONDOWN:
				PostMessage(hDlg, WM_COMMAND, ID_SHOW,0);
				break;
			case WM_LBUTTONDBLCLK:
				break;
		}
	break;
	}
	if (message == WM_TASKBAR_CREATED) {
		Shell_NotifyIcon(NIM_ADD, &nid);
	}
	return FALSE;
}



int WINAPI WinMain(HINSTANCE hinstance,
		HINSTANCE hPrevhinstance,
		LPSTR lpCmdLine,
		int nCmdShow)
{
;


	hinst=hinstance;
	hmutex = NULL;
	hmutex = CreateMutex(0, TRUE, APPCLASS);
	if (hmutex && (GetLastError()==ERROR_ALREADY_EXISTS)) {
		//MessageBox(0, TEXT("程序已经运行！！"), TEXT("错误提示..."), MB_OK | MB_ICONERROR);
		HANDLE hDlgwnd = FindWindow(NULL, APP_NAME);
		PostMessage(hDlgwnd, WM_COMMAND, ID_SHOW, 0);
		ExitProcess(0);
	}

	if (init_conf())
		MessageBox(0, TEXT("读取配置文件出错！！"), TEXT("错误提示..."), MB_OK|MB_ICONERROR);

	DialogBox(hinst, MAKEINTRESOURCE(IDD_DIALOG1), 0, maindlg);
	if (hmutex) {
		CloseHandle(hmutex);
	}
	ExitProcess (0);

}
