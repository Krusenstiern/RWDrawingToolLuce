// DrawingToolFactoryLuce.h : Declaration of the CDrawingToolFactoryLuce

#pragma once
#include "resource.h"       // main symbols

#include "RWDrawingToolLuce_i.h"
#include <RasterImageEditToolsFactoryImpl.h>
int ProcessorCount();
#include "EditToolLuce.h"


extern __declspec(selectany) SToolSpec const g_tLuceTool =
{
	L"LUCE", L"[0409]Luce[0405]Luce", L"[0409]Add light and shadow rays originating from features of the image.[0405]Pøidat svìtelné a stínové paprsky vycházející z obsahu obrázku.",
	{0x1ce4e54c, 0x19e, 0x4a2d, {0xa2, 0x10, 0xe1, 0xf4, 0x33, 0xb7, 0x0e, 0x24}}, IDI_EDITTOOL_LUCE,
	EBMDrawOver|EBMReplace, 0, 0, ETPSNoPaint, FALSE,
	&CreateTool<CEditToolLuce>, &CreateToolWindow<CEditToolLuceDlg>
};



// CDrawingToolFactoryLuce

class ATL_NO_VTABLE CDrawingToolFactoryLuce :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CDrawingToolFactoryLuce, &CLSID_DrawingToolFactoryLuce>,
	public CRasterImageEditToolsFactoryImpl<&g_tLuceTool, 1>,
	public IAutoEditTools
{
public:
	CDrawingToolFactoryLuce()
	{
	}

DECLARE_NO_REGISTRY()
DECLARE_CLASSFACTORY_SINGLETON(CDrawingToolFactoryLuce)

BEGIN_CATEGORY_MAP(CDrawingToolFactoryLuce)
	IMPLEMENTED_CATEGORY(CATID_RasterImageEditToolsFactory)
	IMPLEMENTED_CATEGORY(CATID_AutoEditTools)
END_CATEGORY_MAP()

BEGIN_COM_MAP(CDrawingToolFactoryLuce)
	COM_INTERFACE_ENTRY(IRasterImageEditToolsFactory)
	COM_INTERFACE_ENTRY(IAutoEditTools)
END_COM_MAP()

	// IAutoEditTools methods
public:
	STDMETHOD(ModifyIDs)(IDocument* a_pDoc, BSTR* a_pbstrIDs, BSTR a_bstrBlocked);
};

OBJECT_ENTRY_AUTO(__uuidof(DrawingToolFactoryLuce), CDrawingToolFactoryLuce)
