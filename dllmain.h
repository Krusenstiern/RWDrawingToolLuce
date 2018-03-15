// dllmain.h : Declaration of module class.

class CRWDrawingToolLuceModule : public CAtlDllModuleT< CRWDrawingToolLuceModule >
{
public :
	DECLARE_LIBID(LIBID_RWDrawingToolLuceLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_RWDRAWINGTOOLLUCE, "{E463D0BA-D058-45CC-AF7F-5B07FF9F4610}")
};

extern class CRWDrawingToolLuceModule _AtlModule;
