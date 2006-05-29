/*
 *(C) 2006 Roku LLC
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU General Public License Version 2 as published 
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but 
 * without any warranty; without even the implied warranty of 
 * merchantability or fitness for a particular purpose. See the GNU General 
 * Public License for more details.
 *
 * Please read README.txt in the same directory as this source file for 
 * further license information.
 */

#include "stdafx.h"
#include "AdvancedPage.h"
#include "IniFile.h"
#include "FireflyShell.h"

LRESULT CAdvancedPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	IniFile ini(GetApplication()->GetConfigPath());
	m_server_port = ini.GetInteger(_T("general"), _T("port"), 9999);

	DoDataExchange(false);
	m_port_spin.SetRange32(1, 65535);

	switch (GetApplication()->IsAutoStartEnabled())
	{
	case 0:
		m_autostart_check.SetCheck(BST_UNCHECKED);
		break;
	case 1:
		m_autostart_check.SetCheck(BST_CHECKED);
		break;
	case 2:
		// Be sneaky here. Make it capable of showing indeterminate but
		// don't make it automatically change on click. We'll revert
		// to a normal checkbox when the click happens.
		m_autostart_check.SetButtonStyle(BS_3STATE);
		m_autostart_check.SetCheck(BST_INDETERMINATE);
		break;
	}
	UpdateControls();

	GetApplication()->ServiceStatusSubscribe(this);
	return 0;
}

void CAdvancedPage::OnDestroy()
{
	GetApplication()->ServiceStatusUnsubscribe(this);
}

void CAdvancedPage::UpdateControls()
{
	Service::Status status = GetApplication()->GetServiceStatus();
	UpdateControls(status);
}

void CAdvancedPage::UpdateControls(Service::Status status)
{
	UINT state_id;
	if (status.IsPending())
	{
		state_id = IDS_SERVER_PENDING;
		GetDlgItem(IDC_STARTSERVICE).ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STOPSERVICE).ShowWindow(SW_HIDE);
	}
	else if (status.IsRunning())
	{
		state_id = IDS_SERVER_RUNNING;
		GetDlgItem(IDC_STARTSERVICE).ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STOPSERVICE).ShowWindow(SW_SHOW);
	}
	else
	{
		state_id = IDS_SERVER_STOPPED;
		GetDlgItem(IDC_STARTSERVICE).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STOPSERVICE).ShowWindow(SW_HIDE);
	}

	const bool can_configure = GetApplication()->CanConfigure();
	GetDlgItem(IDC_SERVERPORT).EnableWindow(can_configure);
	GetDlgItem(IDC_PORTSPIN).EnableWindow(can_configure);

	// If we can't control the service then don't give the user
	// the impression that we can.
	const bool can_control = GetApplication()->CanControlService();
	GetDlgItem(IDC_STARTSERVICE).EnableWindow(can_control);
	GetDlgItem(IDC_STOPSERVICE).EnableWindow(can_control);
	GetDlgItem(IDC_AUTOSTART).EnableWindow(can_control);

	CString state;
	state.LoadString(state_id);
	if (!can_control)
	{
		CString s;
		s.LoadString(IDS_NOT_ADMIN);
		state += " ";
		state += s;
	}

	GetDlgItem(IDC_SERVERSTATE).SetWindowText(state);
}

int CAdvancedPage::OnApply()
{
	ATLTRACE("CAdvancedPage::OnApply\n");

	if (!DoDataExchange(true))
		return false;

	IniFile ini(GetApplication()->GetConfigPath());
	ini.SetInteger(_T("general"), _T("port"), m_server_port);

	switch (m_autostart_check.GetCheck())
	{
		case BST_CHECKED:
			GetApplication()->EnableAutoStart(m_hWnd, true);
			break;
		case BST_UNCHECKED:
			GetApplication()->EnableAutoStart(m_hWnd, false);
			break;
		case BST_INDETERMINATE:
			// Ignore
			break;
	}

	// Incorrectly documented in WTL
	return true;
}

LRESULT CAdvancedPage::OnStartService(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	GetApplication()->StartService(m_hWnd);
	UpdateControls();
	return 0;
}

LRESULT CAdvancedPage::OnStopService(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	GetApplication()->StopService(m_hWnd);
	UpdateControls();
	return 0;
}

LRESULT CAdvancedPage::OnWebAdmin(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// Go to the config file because we might not have committed a change yet.
	IniFile ini(GetApplication()->GetConfigPath());
	unsigned int port = ini.GetInteger(_T("general"), _T("port"), 9999);

	CString url;
	url.Format(_T("http://localhost:%u/"), port);

	::ShellExecute(m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWNORMAL);
	return 0;
}

void CAdvancedPage::OnServiceStatus(Service::Status old_status, Service::Status new_status)
{
	UpdateControls(new_status);
}
