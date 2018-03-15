// DrawingToolFactoryLuce.cpp : Implementation of CDrawingToolFactoryLuce

#include "stdafx.h"
#include "DrawingToolFactoryLuce.h"


// CDrawingToolFactoryLuce

STDMETHODIMP CDrawingToolFactoryLuce::ModifyIDs(IDocument* a_pDoc, BSTR* a_pbstrIDs, BSTR a_bstrBlocked)
{
	if (a_pbstrIDs == NULL)
		return E_INVALIDARG;
	CComPtr<IDocumentRasterImage> p;
	if (a_pDoc == NULL || FAILED(a_pDoc->QueryFeatureInterface(__uuidof(IDocumentRasterImage), reinterpret_cast<void**>(&p))))
		return S_FALSE;
	if (a_bstrBlocked && wcsstr(a_bstrBlocked, L"LUCE"))
		return S_FALSE;
	LPWSTR pLuce = wcsstr(*a_pbstrIDs, L"LUCE");
	if (pLuce && (pLuce == *a_pbstrIDs || pLuce[-1] == L' ') && (pLuce[4] == L'\0' || pLuce[4] == L' '))
		return S_FALSE; // already in the list
	LPWSTR pShadow = wcsstr(*a_pbstrIDs, L"PSHADOW"); // being close to the Projected Shadow tool would be a good place for the Luce tool
	if (pShadow && pShadow[7] != L'\0' && pShadow[7] != L' ')
		pShadow = NULL;
	CComBSTR bstr(SysStringLen(*a_pbstrIDs)+5);
	ULONG nCopied = pShadow ? pShadow+7-*a_pbstrIDs : SysStringLen(*a_pbstrIDs);
	wcsncpy(bstr, *a_pbstrIDs, nCopied);
	wcscpy(bstr+nCopied, L" LUCE");
	if (pShadow)
		wcscpy(bstr+nCopied+5, pShadow+7);
	std::swap(bstr.m_str, *a_pbstrIDs);
	return S_OK;
}

int ProcessorCount()
{
	static int nProcessors = 0;
	if (nProcessors)
		return nProcessors;
	DWORD_PTR dwProcess = 0, dwSystem = 0;
	GetProcessAffinityMask(GetCurrentProcess(), &dwProcess, &dwSystem);
	while (dwProcess)
	{
		nProcessors += dwProcess&1;
		dwProcess >>= 1;
	}
	if (nProcessors == 0)
		nProcessors = 1; // WTF
	return nProcessors;
}
