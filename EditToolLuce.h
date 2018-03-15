
#pragma once

#include "luce.h"
#include <GammaCorrection.h>

struct CEditToolDataLuce
{
	CEditToolDataLuce() : fIntensity(1.0f), bQuadraticAtt(false) {}

	bool FromText(LPCWSTR a_psz)
	{
		if (a_psz == NULL) return false;
		float fI = fIntensity;
		float fA = bQuadraticAtt ? 2.0f : 1.0f;
		if (2 != swscanf(a_psz, L"%f, %f", &fI, &fA))
			return false;
		fIntensity = fI;
		bQuadraticAtt = fA > 1.5f ? true : false;
		return true;
	}
	void ToText(BSTR* a_pbstr)
	{
		wchar_t sz[64];
		_swprintf(sz, L"%g, %i", fIntensity, bQuadraticAtt ? 2 : 1);
		*a_pbstr = SysAllocString(sz);
	}
	bool operator==(CEditToolDataLuce const& a_rhs) const
	{
		return fabsf(fIntensity-a_rhs.fIntensity) < 1e-3f && bQuadraticAtt == a_rhs.bQuadraticAtt;
	}
	bool operator!=(CEditToolDataLuce const& a_rhs) const { return !operator==(a_rhs); }

	float fIntensity;
	bool bQuadraticAtt;
};

#include "EditToolLuceDlg.h"

class CEditToolLuce :
	public CComObjectRootEx<CComMultiThreadModel>,
	public IRasterImageEditTool,
	public IRasterImageEditToolScripting
{
public:
	CEditToolLuce() : m_bLightValid(false), m_bBufferInvalid(true), m_eBM(EBMDrawOver)
	{
	}

	BEGIN_COM_MAP(CEditToolLuce)
		COM_INTERFACE_ENTRY(IRasterImageEditTool)
		COM_INTERFACE_ENTRY(IRasterImageEditToolScripting)
	END_COM_MAP()

	// IRasterImageEditTool methods
public:
	STDMETHOD(GetImageTile)(LONG a_nX, LONG a_nY, ULONG a_nSizeX, ULONG a_nSizeY, float a_fGamma, ULONG a_nStride, TRasterImagePixel* a_pBuffer)
	{
		if (!m_bLightValid)
			return m_pWindow->GetImageTile(a_nX, a_nY, a_nSizeX, a_nSizeY, a_fGamma, a_nStride, EITIBackground, a_pBuffer);
		if (a_nX < 0 || a_nY < 0 || LONG(a_nX+a_nSizeX) > LONG(m_nSizeX) || LONG(a_nY+a_nSizeY) > LONG(m_nSizeY))
			m_pWindow->GetImageTile(a_nX, a_nY, a_nSizeX, a_nSizeY, a_fGamma, a_nStride, EITIBackground, a_pBuffer);
		RECT const rc =
		{
			max(a_nX, 0),
			max(a_nY, 0),
			min(LONG(a_nX+a_nSizeX), LONG(m_nSizeX)),
			min(LONG(a_nY+a_nSizeY), LONG(m_nSizeY))
		};
		if (rc.right <= rc.left || rc.bottom <= rc.top) return S_FALSE;

		int const mat = 32768;

		CAutoPtr<CGammaTables> pGamma;
		CGammaTables* pG = NULL;
		if (a_fGamma == 2.2 || a_fGamma == 0.0f)
		{
			pG = m_pGamma;
		}
		else
		{
			pGamma.Attach(new CGammaTables(a_fGamma));
			pG = pGamma;
		}
		if (m_bBufferInvalid || m_cBuffer.m_p == NULL)
		{
			ObjectLock cLock(this);
			if (a_fGamma == 2.2 || a_fGamma == 0.0f)
			{
				if (m_pGamma.m_p == NULL)
					m_pGamma.Attach(new CGammaTables(2.2f));
			}
			if (m_bBufferInvalid || m_cBuffer.m_p == NULL)
			{
				if (m_cBuffer.m_p == NULL)
				{
					m_cBuffer.Allocate(m_nSizeX*m_nSizeY);
				}
				CAutoVectorPtr<TRasterImagePixel> cSrc(new TRasterImagePixel[m_nSizeX*m_nSizeY*2]);
				m_pWindow->GetImageTile(0, 0, m_nSizeX, m_nSizeY, 2.2f, m_nSizeX, EITIContent, cSrc.m_p+m_nSizeX*m_nSizeY);
				TRasterImagePixel* pS = cSrc.m_p+m_nSizeX*m_nSizeY;
				TRasterImagePixel16* pD = reinterpret_cast<TRasterImagePixel16*>(cSrc.m_p);
				for (ULONG n = m_nSizeX*m_nSizeY; n > 0; --n, ++pS, ++pD)
				{
					unsigned const a = pS->bA;
					unsigned const ia = mat*(255-a);
					pD->wB = (pG->m_aGamma[pS->bB]*a + ia + 128)/255;
					pD->wG = (pG->m_aGamma[pS->bG]*a + ia + 128)/255;
					pD->wR = (pG->m_aGamma[pS->bR]*a + ia + 128)/255;
					pD->wA = a|(a<<8);
				}
				CLuce cLuce;
				LuceOptions cOpt;
				cOpt.InitDefault();
				cOpt.SetCenter(m_tLight.fX/m_nSizeX, m_tLight.fY/m_nSizeY);
				if (m_eBM == EBMDrawOver)
					cOpt.SetAddSource();
				else
					cOpt.SetNotAddSource();
				cOpt.SetNegativeIntensity(m_cData.fIntensity);
				cOpt.SetPositiveIntensity(m_cData.fIntensity);
				if (m_cData.bQuadraticAtt)
					cOpt.SetQuadraticAttenuation();
				else
					cOpt.SetLinearAttenuation();
				cOpt.SetNumThreadsToUse(ProcessorCount());
				//cOpt.SetDirection((m_tLight.fX-m_nSizeX*0.5)/(m_nSizeX*0.5f), (m_tLight.fY-m_nSizeY*0.5)/(m_nSizeY*0.5f));
				cLuce.pOpt = &cOpt;
				cLuce.width = m_nSizeX;
				cLuce.height = m_nSizeY;
				if (/*m_cData.bColored*/true)
				{
					cLuce.input.iPixelSize = 8;
					cLuce.input.iStride = 8*m_nSizeX;
					cLuce.input.pBits = reinterpret_cast<BYTE*>(cSrc.m_p);
					cLuce.output.iPixelSize = 8;
					cLuce.output.iStride = 8*m_nSizeX;
					cLuce.output.pBits = reinterpret_cast<BYTE*>(m_cBuffer.m_p);
					cLuce.alpha = cLuce.input;
					cLuce.alpha.pBits += 6;
					for (int i = 0; i < 4; ++i)
					{
						if (i == 4-1)
						{
							cLuce.alpha.pBits = NULL;
						}
						cLuce.currentZero = i;
						cLuce.Do16Bit();
						cLuce.input.pBits += 2;
						cLuce.output.pBits += 2;
					}
				}
				else
				{
					cLuce.numchannels = 4;
					cLuce.alphaInInputChannel = 3;
					cLuce.input.iPixelSize = 8;
					cLuce.input.iStride = 8*m_nSizeX;
					cLuce.input.pBits = reinterpret_cast<BYTE*>(cSrc.m_p);
					cLuce.output.iPixelSize = 8;
					cLuce.output.iStride = 8*m_nSizeX;
					cLuce.output.pBits = reinterpret_cast<BYTE*>(m_cBuffer.m_p);
					cLuce.alpha.iPixelSize = cLuce.alpha.iStride = 0;
					cLuce.alpha.pBits = NULL;
					cLuce.Do16Bit();
				}
				m_bBufferInvalid = false;
			}
		}

		for (LONG y = rc.top; y < rc.bottom; ++y)
		{
			TRasterImagePixel* pD = a_pBuffer+(y-a_nY)*a_nStride+rc.left-a_nX;
			TRasterImagePixel16 const* pS = m_cBuffer.m_p+y*m_nSizeX+rc.left;
			for (TRasterImagePixel16 const* pSEnd = pS+(rc.right-rc.left); pS != pSEnd; ++pD, ++pS)
			{
				int const nA = pS->wA>>8;
				if (nA)
				{
					int nR = ((pS->wR-32768)*255)/nA+32768;
					int nG = ((pS->wG-32768)*255)/nA+32768;
					int nB = ((pS->wB-32768)*255)/nA+32768;
					pD->bB = pG->InvGamma(nB < 0 ? 0 : (nB > 65535 ? 65535 : nB));
					pD->bG = pG->InvGamma(nG < 0 ? 0 : (nG > 65535 ? 65535 : nG));
					pD->bR = pG->InvGamma(nR < 0 ? 0 : (nR > 65535 ? 65535 : nR));
					pD->bA = nA;
				}
				else
				{
					static TRasterImagePixel const t0 = {0, 0, 0, 0};
					*pD = t0;
				}
			}
		}
		return S_OK;
	}

	STDMETHOD(GetSelectionInfo)(RECT* /*a_pBoundingRectangle*/, BOOL* /*a_bEntireRectangle*/)
	{ return E_NOTIMPL; }
	STDMETHOD(GetSelectionTile)(LONG /*a_nX*/, LONG /*a_nY*/, ULONG /*a_nSizeX*/, ULONG /*a_nSizeY*/, ULONG /*a_nStride*/, BYTE* /*a_pBuffer*/)
	{ return E_NOTIMPL; }

	STDMETHOD(Init)(IRasterImageEditWindow* a_pWindow)
	{
		m_pWindow = a_pWindow;
		return S_OK;
	}

	STDMETHOD(SetState)(ISharedState* a_pState)
	{
		if (a_pState)
		{
			CComBSTR bstr;
			a_pState->ToText(&bstr);
			if (bstr.Length())
			{
				CEditToolDataLuce cCopy = m_cData;
				m_cData.FromText(bstr);
				if (cCopy != m_cData && m_bLightValid)
				{
					m_bBufferInvalid = true;
					RECT rc = {0, 0, m_nSizeX, m_nSizeY};
					m_pWindow->RectangleChanged(&rc);
				}
			}
		}
		return S_OK;
	}
	STDMETHOD(SetOutline)(BYTE a_bEnabled, float a_fWidth, TColor const* a_pColor)
	{ return E_NOTIMPL; }
	STDMETHOD(SetBrush)(IRasterImageBrush* a_pBrush)
	{
		return S_FALSE;
	}
	STDMETHOD(SetGlobals)(EBlendingMode a_eBlendingMode, ERasterizationMode a_eRasterizationMode, ECoordinatesMode a_eCoordinatesMode)
	{
		if ((a_eBlendingMode == EBMDrawOver && m_eBM == EBMReplace) || (m_eBM == EBMDrawOver && a_eBlendingMode == EBMReplace))
		{
			m_eBM = a_eBlendingMode;
			if (m_bLightValid)
			{
				m_bBufferInvalid = true;
				RECT rc = {0, 0, m_nSizeX, m_nSizeY};
				m_pWindow->RectangleChanged(&rc);
			}
		}
		return S_OK;
	}

	STDMETHOD(Reset)()
	{
		m_tLight.fX = m_tLight.fY = 0.0f;
		if (m_bLightValid)
		{
			m_bLightValid = false;
			m_pWindow->ControlPointsChanged();
		}
		if (m_cBuffer.m_p)
		{
			m_cBuffer.Free();
			m_pWindow->RectangleChanged(NULL);
		}

		return S_FALSE;
	}
	STDMETHOD(PreTranslateMessage)(MSG const* a_pMsg)
	{ return S_FALSE; }

	STDMETHOD(IsDirty)(RECT* a_pImageRect, BOOL* a_pOptimizeImageRect, RECT* a_pSelectionRect)
	{
		if (a_pImageRect)
		{
			if (m_bLightValid)
			{
				a_pImageRect->left = a_pImageRect->top = 0;
				a_pImageRect->right = m_nSizeX;
				a_pImageRect->bottom = m_nSizeY;
			}
			else
			{
				a_pImageRect->left = a_pImageRect->top = LONG_MAX;
				a_pImageRect->right = a_pImageRect->bottom = LONG_MIN;
			}
		}
		return m_bLightValid ? S_OK : S_FALSE;
	}

	STDMETHOD(AdjustCoordinates)(EControlKeysState UNREF(a_eKeysState), TPixelCoords* a_pPos, TPixelCoords const* a_pPointerSize, ULONG const* a_pControlPointIndex)
	{ return S_FALSE; }
	STDMETHOD(ProcessInputEvent)(EControlKeysState a_eKeysState, TPixelCoords const* a_pPos, TPixelCoords const* a_pPointerSize, float a_fNormalPressure, float a_fTangentPressure, float a_fOrientation, float a_fRotation, float a_fZ, DWORD* a_pMaxIdleTime)
	{
		if (a_fNormalPressure < 0.5f || a_pPos == NULL)
			return S_FALSE;
		if (!m_bLightValid || fabsf(a_pPos->fX-m_tLight.fX) > 1e-03f || fabsf(a_pPos->fY-m_tLight.fY) > 1e-03f)
		{
			m_tLight = *a_pPos;
			bool const bValid = m_bLightValid;
			m_bBufferInvalid = true;
			m_bLightValid = true;
			m_pWindow->Size(&m_nSizeX, &m_nSizeY);
			RECT rc = {0, 0, m_nSizeX, m_nSizeY};
			m_pWindow->RectangleChanged(&rc);
			if (bValid)
				m_pWindow->ControlPointChanged(0);
			else
				m_pWindow->ControlPointsChanged();
		}
		return S_OK;
	}

	STDMETHOD(GetCursor)(EControlKeysState a_eKeysState, TPixelCoords const* a_pPos, HCURSOR* a_phCursor)
	{
		static HCURSOR hCustom = ::LoadCursor(_pModule->get_m_hInst(), MAKEINTRESOURCE(IDC_EDITTOOL_LUCE));
		*a_phCursor = hCustom;
		return S_OK;
	}

	STDMETHOD(GetControlPointCount)(ULONG* a_pCount)
	{
		*a_pCount = m_bLightValid ? 1 : 0;
		return S_OK;
	}
	STDMETHOD(GetControlPoint)(ULONG a_nIndex, TPixelCoords* a_pPos, ULONG* a_pClass)
	{
		if (a_nIndex != 0 || !m_bLightValid)
			return E_RW_INDEXOUTOFRANGE;
		*a_pPos = m_tLight;
		if (a_pClass)
			*a_pClass = 61;//39;44
		return S_OK;
	}
	STDMETHOD(SetControlPoint)(ULONG a_nIndex, TPixelCoords const* a_pPos, boolean a_bFinished, float a_fPointSize)
	{
		if (a_nIndex != 0 || !m_bLightValid)
			return E_RW_INDEXOUTOFRANGE;
		if (fabsf(a_pPos->fX-m_tLight.fX) > 1e-3f || fabsf(a_pPos->fY-m_tLight.fY) > 1e-3f)
		{
			m_tLight = *a_pPos;
			m_bBufferInvalid = true;
			RECT rc = {0, 0, m_nSizeX, m_nSizeY};
			m_pWindow->RectangleChanged(&rc);
			m_pWindow->ControlPointChanged(0);
		}
		return S_OK;
	}
	STDMETHOD(GetControlPointDesc)(ULONG a_nIndex, ILocalizedString** a_ppDescription)
	{
		return E_NOTIMPL;
	}
	STDMETHOD(GetControlLines)(IEditToolControlLines* a_pLines, ULONG a_nLineTypes)
	{
		return S_FALSE;
	}
	STDMETHOD(PointTest)(EControlKeysState UNREF(a_eKeysState), TPixelCoords const* a_pPos, BYTE a_bAccurate)
	{
		return S_FALSE;
	}
	STDMETHOD(Transform)(TMatrix3x3f const* a_pMatrix)
	{
		TVector2f t = {m_tLight.fX, m_tLight.fY};
		t = TransformVector2(*a_pMatrix, t);
		m_tLight.fX = t.x;
		m_tLight.fY = t.y;
		return S_OK;
	}

	// IRasterImageEditToolScripting methods
public:
	STDMETHOD(FromText)(BSTR a_bstrParams)
	{
		if (a_bstrParams == NULL)
			return E_INVALIDARG;
		TPixelCoords tLight = m_tLight;
		if (2 != swscanf(a_bstrParams, L"%f, %f", &tLight.fX, &tLight.fY))
			return E_INVALIDARG;
		LPWSTR p = a_bstrParams;
		while (*p && *p != L',') ++p;
		if (*p == L',') ++p;
		while (*p && *p != L',') ++p;
		if (*p == L',') ++p;
		while (*p == L' ') ++p;
		if (*p == L'\0')
			return E_INVALIDARG;
		CEditToolDataLuce cCopy = m_cData;
		cCopy.FromText(p);
		if (!m_bLightValid || fabsf(tLight.fX-m_tLight.fX) > 1e-3f || fabsf(tLight.fY-m_tLight.fY) > 1e-3f || m_cData != cCopy)
		{
			m_cData = cCopy;
			m_tLight = tLight;
			m_bLightValid = true;
			m_bBufferInvalid = true;
			m_pWindow->RectangleChanged(NULL);
			m_pWindow->ControlPointsChanged();
		}
		return S_OK;
	}
	STDMETHOD(ToText)(BSTR* a_pbstrParams)
	{
		*a_pbstrParams = NULL;
		if (!m_bLightValid)
			return S_FALSE;
		CComBSTR bstrData;
		m_cData.ToText(&bstrData);
		wchar_t sz[64];
		_swprintf(sz, L"%g, %g, ", m_tLight.fX, m_tLight.fY);
		*a_pbstrParams = SysAllocStringLen(sz, bstrData.Length()+wcslen(sz));
		wcscpy((*a_pbstrParams)+wcslen(sz), bstrData);
		return S_OK;
	}

private:
	struct TRasterImagePixel16 {WORD wB, wG, wR, wA;};

private:
	CComPtr<IRasterImageEditWindow> m_pWindow;
	ULONG m_nSizeX;
	ULONG m_nSizeY;
	CAutoVectorPtr<TRasterImagePixel16> m_cBuffer;
	TPixelCoords m_tLight;
	bool m_bLightValid;
	bool m_bBufferInvalid;
	EBlendingMode m_eBM;
	CAutoPtr<CGammaTables> m_pGamma;
	CEditToolDataLuce m_cData;
};


