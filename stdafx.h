// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef STRICT
#define STRICT
#endif

#define ISOLATION_AWARE_ENABLED 1

#include "targetver.h"

#define _ATL_FREE_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#include "resource.h"
#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>
#include <atlwin.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <atlframe.h>
#include <atldlgs.h>

using namespace ATL;

#include <RWErrorCodes.h>
#include <CodingStyle.h>
