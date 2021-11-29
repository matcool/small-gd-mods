#ifndef HACKPRO_EXT_H
#define HACKPRO_EXT_H

#define HP_IMPORT _cdecl

#include <Windows.h>

#include <stdbool.h>

typedef bool (HP_IMPORT *ptr_HackproIsReady)();
typedef void *(HP_IMPORT *ptr_HackproInitialiseExt)(const char *str);
typedef bool (HP_IMPORT *ptr_HackproAddButton)(void *ptr, const char *str, void(__stdcall *callback)(void *));
typedef void *(HP_IMPORT *ptr_HackproAddCheckbox)(void *ptr, const char *str, void(__stdcall *checked_callback)(void *), void(__stdcall *unchecked_callback)(void *));
typedef bool (HP_IMPORT *ptr_HackproSetCheckbox)(void *checkbox, bool state);
typedef void *(HP_IMPORT *ptr_HackproAddComboBox)(void *ptr, void(__stdcall *callback)(void *, int, const char *));
typedef bool (HP_IMPORT *ptr_HackproSetComboBoxStrs)(void *combo, const char **strs);
typedef bool (HP_IMPORT *ptr_HackproSetComboBoxIndex)(void *combo, int idx);
typedef void *(HP_IMPORT *ptr_HackproAddTextBox)(void *ptr, void(__stdcall *callback)(void *));
typedef bool (HP_IMPORT *ptr_HackproSetTextBoxText)(void *box, const char *str);
typedef bool (HP_IMPORT *ptr_HackproSetTextBoxPlaceholder)(void *box, const char *str);
typedef const char *(HP_IMPORT *ptr_HackproGetTextBoxText)(void *box);
typedef bool (HP_IMPORT *ptr_HackproCommitExt)(void *ptr);
typedef bool (HP_IMPORT *ptr_HackproWithdrawExt)(void *ptr);
typedef void (HP_IMPORT *ptr_HackproSetUserData)(void *ptr, void *data);
typedef void *(HP_IMPORT *ptr_HackproGetUserData)(void *ptr);

ptr_HackproIsReady HackproIsReady = NULL;
ptr_HackproInitialiseExt HackproInitialiseExt = NULL;
ptr_HackproAddButton HackproAddButton = NULL;
ptr_HackproAddCheckbox HackproAddCheckbox= NULL;
ptr_HackproSetCheckbox HackproSetCheckbox= NULL;
ptr_HackproAddComboBox HackproAddComboBox= NULL;
ptr_HackproSetComboBoxStrs HackproSetComboBoxStrs = NULL;
ptr_HackproSetComboBoxIndex HackproSetComboBoxIndex = NULL;
ptr_HackproAddTextBox HackproAddTextBox = NULL;
ptr_HackproSetTextBoxText HackproSetTextBoxText = NULL;
ptr_HackproSetTextBoxPlaceholder HackproSetTextBoxPlaceholder = NULL;
ptr_HackproGetTextBoxText HackproGetTextBoxText = NULL;
ptr_HackproCommitExt HackproCommitExt = NULL;
ptr_HackproWithdrawExt HackproWithdrawExt = NULL;
ptr_HackproSetUserData HackproSetUserData = NULL;
ptr_HackproGetUserData HackproGetUserData = NULL;

bool InitialiseHackpro()
{
	HMODULE hMod = GetModuleHandleA("hackpro.dll");

	if (!hMod)
		return false;

	HackproIsReady = (ptr_HackproIsReady)GetProcAddress(hMod, "?HackproIsReady@@YA_NXZ");
	HackproInitialiseExt = (ptr_HackproInitialiseExt)GetProcAddress(hMod, "?HackproInitialiseExt@@YAPAXPBD@Z");
	HackproAddButton = (ptr_HackproAddButton)GetProcAddress(hMod, "?HackproAddButton@@YA_NPAXPBDP6GX0@Z@Z");
	HackproAddCheckbox = (ptr_HackproAddCheckbox)GetProcAddress(hMod, "?HackproAddCheckbox@@YAPAXPAXPBDP6GX0@Z2@Z");
	HackproSetCheckbox = (ptr_HackproSetCheckbox)GetProcAddress(hMod, "?HackproSetCheckbox@@YA_NPAX_N@Z");
	HackproAddComboBox = (ptr_HackproAddComboBox)GetProcAddress(hMod, "?HackproAddComboBox@@YAPAXPAXP6GX0HPBD@Z@Z");
	HackproSetComboBoxStrs = (ptr_HackproSetComboBoxStrs)GetProcAddress(hMod, "?HackproSetComboBoxStrs@@YA_NPAXPAPBD@Z");
	HackproSetComboBoxIndex = (ptr_HackproSetComboBoxIndex)GetProcAddress(hMod, "?HackproSetComboBoxIndex@@YA_NPAXH@Z");
	HackproAddTextBox = (ptr_HackproAddTextBox)GetProcAddress(hMod, "?HackproAddTextBox@@YAPAXPAXP6GX0@Z@Z");
	HackproSetTextBoxText = (ptr_HackproSetTextBoxText)GetProcAddress(hMod, "?HackproSetTextBoxText@@YA_NPAXPBD@Z");
	HackproSetTextBoxPlaceholder = (ptr_HackproSetTextBoxPlaceholder)GetProcAddress(hMod, "?HackproSetTextBoxPlaceholder@@YA_NPAXPBD@Z");
	HackproGetTextBoxText = (ptr_HackproGetTextBoxText)GetProcAddress(hMod, "?HackproGetTextBoxText@@YAPBDPAX@Z");
	HackproCommitExt = (ptr_HackproCommitExt)GetProcAddress(hMod, "?HackproCommitExt@@YA_NPAX@Z");
	HackproWithdrawExt = (ptr_HackproWithdrawExt)GetProcAddress(hMod, "?HackproWithdrawExt@@YA_NPAX@Z");
	HackproSetUserData = (ptr_HackproSetUserData)GetProcAddress(hMod, "?HackproSetUserData@@YAXPAX0@Z");
	HackproGetUserData = (ptr_HackproGetUserData)GetProcAddress(hMod, "?HackproGetUserData@@YAPAXPAX@Z");

	return true;
}

#endif