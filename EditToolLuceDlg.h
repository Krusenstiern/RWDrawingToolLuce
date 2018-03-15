// EditToolLuceDlg.h : Declaration of the CEditToolLuceDlg

#pragma once

#include "resource.h"       // main symbols
#include <ContextHelpDlg.h>
#include <Win32LangEx.h>
#include <ObserverImpl.h>



// CEditToolLuceDlg

class CEditToolLuceDlg : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public Win32LangEx::CLangIndirectDialogImpl<CEditToolLuceDlg>,
	public CObserverImpl<CEditToolLuceDlg, ISharedStateObserver, TSharedStateChange>,
	public CDialogResize<CEditToolLuceDlg>,
	public CContextHelpDlg<CEditToolLuceDlg>,
	public CChildWindowImpl<CEditToolLuceDlg, IRasterImageEditToolWindow>
{
public:
	CEditToolLuceDlg() : m_bEnableUpdates(false)
	{
	}
	~CEditToolLuceDlg()
	{
		if (m_pSharedState)
			m_pSharedState->ObserverDel(ObserverGet(), 0);
	}

	enum { IDC_ATTENUATION_LINEAR = 200, IDC_ATTENUATION_QUADRATIC, IDC_INTENSITY };

	BEGIN_DIALOG_EX(0, 0, 110, 50, 0)
		DIALOG_FONT_AUTO()
		DIALOG_STYLE(WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|DS_CONTROL)
		DIALOG_EXSTYLE(0)
	END_DIALOG()

	BEGIN_CONTROLS_MAP()
		CONTROL_AUTORADIOBUTTON(_T("[0409]Linear attenuation[0405]Lineární útlum"), IDC_ATTENUATION_LINEAR, 5, 5, 100, 10, WS_VISIBLE | WS_GROUP, 0)
		CONTROL_AUTORADIOBUTTON(_T("[0409]Quadratic attenuation[0405]Kvadratický útlum"), IDC_ATTENUATION_QUADRATIC, 5, 19, 100, 10, WS_VISIBLE, 0)
		CONTROL_LTEXT(_T("[0409]Intensity:[0405]Intenzita:"), IDC_STATIC, 5, 35, 40, 8, WS_VISIBLE | WS_GROUP, 0)
		CONTROL_CONTROL(_T(""), IDC_INTENSITY, TRACKBAR_CLASS, TBS_HORZ | TBS_AUTOTICKS | TBS_TOP | WS_TABSTOP | WS_VISIBLE, 45, 30, 60, 18, 0)
	END_CONTROLS_MAP()

	BEGIN_MSG_MAP(CEditToolLuceDlg)
		CHAIN_MSG_MAP(CContextHelpDlg<CEditToolLuceDlg>)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CTLCOLORDLG, OnCtlColorDlg)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
		MESSAGE_HANDLER(WM_CTLCOLORBTN, OnCtlColorDlg)
		COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedSomething)
		COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedSomething)
		COMMAND_HANDLER(IDC_ATTENUATION_LINEAR, BN_CLICKED, OnChange)
		COMMAND_HANDLER(IDC_ATTENUATION_QUADRATIC, BN_CLICKED, OnChange)
		MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
		CHAIN_MSG_MAP(CDialogResize<CEditToolLuceDlg>)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CEditToolLuceDlg)
		DLGRESIZE_CONTROL(IDC_INTENSITY, DLSZ_SIZE_X)
	END_DLGRESIZE_MAP()

	BEGIN_CTXHELP_MAP(CEditToolLuceDlg)
		//CTXHELP_CONTROL_STRING(IDC_ATTENUATION_LINEAR, L"[0409][0405]")
	END_CTXHELP_MAP()

	BEGIN_COM_MAP(CEditToolLuceDlg)
		COM_INTERFACE_ENTRY(IChildWindow)
		COM_INTERFACE_ENTRY(IRasterImageEditToolWindow)
	END_COM_MAP()

	HWND Create(LPCOLESTR a_pszToolID, HWND a_hParent, LCID a_tLocaleID, ISharedStateManager* a_pSharedState, BSTR a_bstrSyncGroup)
	{
		m_tLocaleID = a_tLocaleID;
		m_pSharedState = a_pSharedState;
		m_bstrSyncToolData = a_bstrSyncGroup;
		m_pSharedState->ObserverIns(ObserverGet(), 0);
		return Win32LangEx::CLangIndirectDialogImpl<CEditToolLuceDlg>::Create(a_hParent);
	}
	LRESULT OnCtlColorDlg(UINT a_uMsg, WPARAM a_wParam, LPARAM a_lParam, BOOL& a_bHandled)
	{ return GetParent().SendMessage(WM_CTLCOLORDLG, a_wParam, a_lParam); }
	LRESULT OnCtlColorStatic(UINT a_uMsg, WPARAM a_wParam, LPARAM a_lParam, BOOL& a_bHandled)
	{ return GetParent().SendMessage(a_uMsg, a_wParam, a_lParam); }

	void DataToState()
	{
		if (m_pSharedState != NULL)
		{
			CComPtr<ISharedState> pTmp;
			RWCoCreateInstance(pTmp, __uuidof(SharedStateString));
			CComBSTR bstr;
			m_cData.ToText(&bstr);
			pTmp->FromText(bstr);
			m_bEnableUpdates = false;
			m_pSharedState->StateSet(m_bstrSyncToolData, pTmp);
			m_bEnableUpdates = true;
		}
	}
	void OwnerNotify(TCookie, TSharedStateChange a_tParams)
	{
		if (wcscmp(a_tParams.bstrName, m_bstrSyncToolData) == 0)
		{
			CComBSTR bstr;
			a_tParams.pState->ToText(&bstr);
			if (bstr)
			{
				m_cData.FromText(bstr);
				if (m_bEnableUpdates)
					DataToGUI();
			}
		}
	}

	STDMETHOD(OptimumSize)(SIZE* a_pSize)
	{
		try
		{
			GetDialogSize(a_pSize, m_tLocaleID);
			return S_OK;
		}
		catch (...)
		{
			return a_pSize == NULL ? E_POINTER : E_UNEXPECTED;
		}
	}
	STDMETHOD(SetState)(ISharedState* a_pState)
	{
		CComBSTR bstr;
		if (a_pState) a_pState->ToText(&bstr);
		if (bstr.Length() && m_cData.FromText(bstr))
		{
			DataToGUI();
		}
		return S_OK;
	}

	// IChildWindow methods
public:
	STDMETHOD(PreTranslateMessage)(MSG const* a_pMsg, BOOL a_bBeforeAccel)
	{
		if (!a_bBeforeAccel && m_hWnd && IsDialogMessage(const_cast<LPMSG>(a_pMsg)))
			return S_OK;
		return S_FALSE;
	}


	LRESULT OnInitDialog(UINT a_uMsg, WPARAM a_wParam, LPARAM a_lParam, BOOL& a_bHandled)
	{
		m_wndAttLine = GetDlgItem(IDC_ATTENUATION_LINEAR);
		m_wndAttQuad = GetDlgItem(IDC_ATTENUATION_QUADRATIC);
		m_wndIntensity = GetDlgItem(IDC_INTENSITY);
		m_wndIntensity.SetRange(0, 40, FALSE);
		m_wndIntensity.SetPageSize(5);
		m_wndIntensity.SetTicFreq(5);

		if (m_pSharedState != NULL)
		{
			CComPtr<ISharedState> pState;
			m_pSharedState->StateGet(m_bstrSyncToolData, __uuidof(ISharedState), reinterpret_cast<void**>(&pState));
			CComBSTR bstr;
			if (pState != NULL) pState->ToText(&bstr);
			if (bstr) m_cData.FromText(bstr);
		}
		DataToGUI();

		DlgResize_Init(false, false, 0);

		return 1;  // Let the system set the focus
	}
	LRESULT OnClickedSomething(WORD a_wNotifyCode, WORD a_wID, HWND a_hWndCtl, BOOL& a_bHandled)
	{
		return 0;
	}
	LRESULT OnChange(WORD a_wNotifyCode, WORD a_wID, HWND a_hWndCtl, BOOL& a_bHandled)
	{
		if (!m_bEnableUpdates)
			return 0;

		GUIToData();
		DataToGUI();
		DataToState();
		return 0;
	}
	LRESULT OnHScroll(UINT a_uMsg, WPARAM a_wParam, LPARAM a_lParam, BOOL& a_bHandled)
	{
		int nInt = m_cData.fIntensity <= 0.25f ? 0 : (m_cData.fIntensity >= 4.0f ? 40 : logf(m_cData.fIntensity/0.25f)/logf(4.0f/0.25f)*40+0.5f);
		int nNew = m_wndIntensity.GetPos();
		if (nInt != nNew)
		{
			m_cData.fIntensity = 0.25f * powf(4.0f/0.25f, nNew/40.0f);
			DataToState();
		}
		return 0;
	}

	void GUIToData()
	{
		m_cData.bQuadraticAtt = m_wndAttQuad.GetCheck() == BST_CHECKED;
	}
	void DataToGUI()
	{
		m_bEnableUpdates = false;
		int nInt = m_cData.fIntensity <= 0.25f ? 0 : (m_cData.fIntensity >= 4.0f ? 40 : logf(m_cData.fIntensity/0.25f)/logf(4.0f/0.25f)*40+0.5f);
		m_wndIntensity.SetPos(nInt);
		m_wndAttLine.SetCheck(m_cData.bQuadraticAtt ? BST_UNCHECKED : BST_CHECKED);
		m_wndAttQuad.SetCheck(m_cData.bQuadraticAtt ? BST_CHECKED : BST_UNCHECKED);
		m_bEnableUpdates = true;
	}

private:
	CButton m_wndAttLine;
	CButton m_wndAttQuad;
	CTrackBarCtrl m_wndIntensity;
	CComPtr<ISharedStateManager> m_pSharedState;
	CComBSTR m_bstrSyncToolData;
	CEditToolDataLuce m_cData;
	bool m_bEnableUpdates;
};


