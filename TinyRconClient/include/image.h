// GDIPlusHelper.h: interface for the CGDIPlusHelper class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GDIPLUSHELPER_H__BD5F6266_5686_43E2_B146_5EA1217A56FE__INCLUDED_)
#define AFX_GDIPLUSHELPER_H__BD5F6266_5686_43E2_B146_5EA1217A56FE__INCLUDED_

#pragma once

#define VC_EXTRALEAN// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>// MFC core and standard components
#include <afxext.h>// MFC extensions
#include <afxdisp.h>// MFC Automation classes
#include <afxdtctl.h>// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>// MFC support for Windows Common Controls
#endif// _AFX_NO_AFXCMN_SUPPORT

// SUPPORT FOR THE GDI+ SUBSYSTEM
#pragma comment(lib, "gdiplus.lib")
#include <gdiplus.h>
#include <utility>

using namespace Gdiplus;


class ImageEx : public Image
{
public:
  ImageEx(const char *sResourceType, const char *sResource);
  ImageEx(const WCHAR *filename, BOOL useEmbeddedColorManagement = FALSE);
  ~ImageEx();

public:
  std::pair<UINT, UINT> GetSize();
  bool IsAnimatedGIF() { return m_nFrameCount > 1; }
  void SetPause(bool bPause);
  bool IsPaused() { return m_bPause; }
  bool InitAnimation(HWND hWnd, Point pt);
  void Destroy();

protected:
  bool TestForAnimatedGIF();
  void Initialize();
  bool DrawFrameGIF();

  IStream *m_pStream;

  bool LoadFromBuffer(BYTE *pBuff, int nSize);
  bool GetResource(LPCTSTR lpName, LPCTSTR lpType, void *pResource, int &nBufSize);
  bool Load(CString sResourceType, CString sResource);

  void ThreadAnimation();

  static UINT WINAPI _ThreadAnimationProc(LPVOID pParam);

  HANDLE m_hThread;
  HANDLE m_hPause;
  HANDLE m_hExitEvent;
  HINSTANCE m_hInst;
  HWND m_hWnd;
  UINT m_nFrameCount;
  UINT m_nFramePosition;
  bool m_bIsInitialized;
  bool m_bPause;
  PropertyItem *m_pPropertyItem;
  Point m_pt;
};


#endif// !defined(AFX_GDIPLUSHELPER_H__BD5F6266_5686_43E2_B146_5EA1217A56FE__INCLUDED_)
