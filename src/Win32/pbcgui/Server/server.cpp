
#include <windows.h>
#include "pbc_config.h"
#include "gui_myconfig.h"
#include "CPBC_PropSheet.h"
#include "resource.h"
#include "globals.h"
#include <crtdbg.h>
#include <string>
#include <tchar.h>
#include <strsafe.h>
using namespace std;

#define libpbc_config_getint(p,n,d) libpbc_myconfig_getint(p,_T(n),d)
#define libpbc_config_getstring(p,n,d) libpbc_myconfig_getstring(p,_T(n),_T(d))


void CPBC_PropSheet::GetEffectiveValue(int i) {

//not used

}

void CPBC_PropSheet::Set_Delete_Button(int i) {
	HWND hDelete         = GetDlgItem(hwndDlg, IDC_Delete);

	if ((!_wcsicmp(directive[i].defined_in.c_str(),STR_THIS_WEB_INSTANCE) && wcslen(pwzInstance)) ||
		(!_wcsicmp(directive[i].defined_in.c_str(),STR_SERVER_DEFAULT) && !wcslen(pwzInstance))) {
		EnableWindow(hDelete,TRUE);
	} else {
		EnableWindow(hDelete,FALSE);
	}
}

void CPBC_PropSheet::ReadSelectedValue() {

#	define SERVER_VALS_INIT
#	include "pbc_directives.h"
#	undef SERVER_VALS_INIT

	HWND hProps	         = GetDlgItem(hwndDlg, IDC_PROPS);

	DWORD index = SendMessage(hProps, CB_GETCURSEL, 0,0); 
	LRESULT i = SendMessage(hProps, CB_GETITEMDATA, (WPARAM)index, 0 );
	directive[i].new_value = directive[i].value;

}

void CPBC_PropSheet::ReadCurrentValues() {

#	define SERVER_VALS_INIT
#	include "pbc_directives.h"
#	undef SERVER_VALS_INIT

	for (int i=0;i<NUM_DIRECTIVES;i++) {
		directive[i].new_value = directive[i].value;
	}
}
void CPBC_PropSheet::SetupPropSheet() {

		ReadCurrentValues();
		PopulateComboBox(GetDlgItem(hwndDlg, IDC_PROPS));	
		PopulatePage();

	}

void CPBC_PropSheet::WriteValues() {

	wstring RegPath;

	if  (wcslen(pwzInstance)) {
		RegPath = _T(PBC_INSTANCE_KEY);
		RegPath += L"\\";
		RegPath += pwzInstance;
	}

	for (int i=0;i<NUM_DIRECTIVES;i++) {
		if (wcsicmp(directive[i].value.c_str(),directive[i].new_value.c_str())) {
			if (directive[i].type == D_BOUND_INT || directive[i].type == D_FREE_INT) {
				WriteRegInt(RegPath.c_str(), directive[i].name.c_str(), directive[i].new_value.c_str());
			} else {
				WriteRegString(RegPath.c_str(), directive[i].name.c_str(), directive[i].new_value.c_str());
			}
		}
	}

}

void CPBC_PropSheet::DeleteValue() {
	wchar_t value_name[BUFFSIZE];
	HWND hProps	         = GetDlgItem(hwndDlg, IDC_PROPS);

	DWORD index = SendMessage(hProps, CB_GETCURSEL, 0,0); 
	LRESULT debug = SendMessage(hProps, CB_GETLBTEXT, (WPARAM)index, (LPARAM)value_name );
	
	if (wcslen(pwzInstance)) {
		wchar_t keyBuff[BUFFSIZE];
		StringCbCopy (keyBuff,BUFFSIZE,_T(PBC_INSTANCE_KEY));
		StringCbCat  (keyBuff,BUFFSIZE,L"\\");
		StringCbCat  (keyBuff,BUFFSIZE,pwzInstance);
		DeleteRegVal(keyBuff, value_name);
	} else {
		DeleteRegVal(L"", value_name);
	}
}

HKEY CPBC_PropSheet::OpenPBCKey(LPCTSTR szKey, REGSAM samDesired) {
	_TCHAR szKeyBuf[BUFFSIZE] ;

	StringCbCopy (szKeyBuf, BUFFSIZE, _T(PBC_FILTER_KEY));
	if (wcslen(szKey)) {
		StringCbCat (szKeyBuf, BUFFSIZE+MAX_PATH, L"\\");
		StringCbCat (szKeyBuf,BUFFSIZE,szKey);
	}
	ReplaceSlashes(szKeyBuf);

	return(OpenKey(szKeyBuf,samDesired));

}
