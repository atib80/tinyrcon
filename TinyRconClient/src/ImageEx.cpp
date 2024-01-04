// GDIPlusHelper.cpp: implementation of the CGDIPlusHelper class.
//
//////////////////////////////////////////////////////////////////////

#include "ImageEx.h"
#include <process.h>
#include "tiny_rcon_client_application.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

extern tiny_rcon_client_application main_app;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:	ImageEx::ImageEx
//
// DESCRIPTION:	Constructor for constructing images from a resource
//
// RETURNS:
//
// NOTES:
//
// MODIFICATIONS:
//
// Name				Date		Version		Comments
// N T ALMOND       29012002	1.0			Origin
//
////////////////////////////////////////////////////////////////////////////////
ImageEx::ImageEx(const char *sResourceType, const char *sResource)
{
  Initialize();

  if (Load(sResourceType, sResource)) {

    nativeImage = nullptr;

    lastResult = DllExports::GdipLoadImageFromStreamICM(m_pStream, &nativeImage);

    TestForAnimatedGIF();
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:	ImageEx::ImageEx
//
// DESCRIPTION:	Constructor for constructing images from a file
//
// RETURNS:
//
// NOTES:
//
// MODIFICATIONS:
//
// Name				Date		Version		Comments
// N T ALMOND       29012002	1.0			Origin
//
////////////////////////////////////////////////////////////////////////////////
ImageEx::ImageEx(const WCHAR *filename, BOOL useEmbeddedColorManagement) : Image(filename, useEmbeddedColorManagement)
{
  Initialize();

  m_bIsInitialized = true;

  TestForAnimatedGIF();
}


////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:	ImageEx::~ImageEx
//
// DESCRIPTION:	Free up fresources
//
// RETURNS:
//
// NOTES:
//
// MODIFICATIONS:
//
// Name				Date		Version		Comments
// N T ALMOND       29012002	1.0			Origin
//
////////////////////////////////////////////////////////////////////////////////
ImageEx::~ImageEx()
{
  Destroy();
}

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:	InitAnimation
//
// DESCRIPTION:	Prepare animated GIF
//
// RETURNS:
//
// NOTES:
//
// MODIFICATIONS:
//
// Name				Date		Version		Comments
// N T ALMOND       29012002	1.0			Origin
//
////////////////////////////////////////////////////////////////////////////////
bool ImageEx::InitAnimation(HWND hWnd, Point pt)
{

  m_hWnd = hWnd;
  m_pt = pt;

  if (!m_bIsInitialized) {
    print_colored_text(app_handles.hwnd_re_messages_data, "^3GIF not initialized!\n");
    return false;
  };

  if (IsAnimatedGIF()) {
    if (m_hThread == nullptr) {

      unsigned int nTID = 0;

      m_hThread = (HANDLE)_beginthreadex(nullptr, 0, _ThreadAnimationProc, this, CREATE_SUSPENDED, &nTID);

      if (!m_hThread) {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3Could not launch a GIF animation thread!\n");
        return true;
      }

      ResumeThread(m_hThread);
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:	LoadFromBuffer
//
// DESCRIPTION:	Helper function to copy phyical memory from buffer a IStream
//
// RETURNS:
//
// NOTES:
//
// MODIFICATIONS:
//
// Name				Date		Version		Comments
// N T ALMOND       29012002	1.0			Origin
//
////////////////////////////////////////////////////////////////////////////////
bool ImageEx::LoadFromBuffer(BYTE *pBuff, int nSize)
{
  bool bResult = false;

  HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, nSize);
  if (hGlobal) {
    void *pData = GlobalLock(hGlobal);
    if (pData)
      memcpy(pData, pBuff, nSize);

    GlobalUnlock(hGlobal);

    if (CreateStreamOnHGlobal(hGlobal, TRUE, &m_pStream) == S_OK)
      bResult = true;
  }


  return bResult;
}

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:	GetResource
//
// DESCRIPTION:	Helper function to lock down resource
//
// RETURNS:
//
// NOTES:
//
// MODIFICATIONS:
//
// Name				Date		Version		Comments
// N T ALMOND       29012002	1.0			Origin
//
////////////////////////////////////////////////////////////////////////////////
bool ImageEx::GetResource(LPCTSTR lpName, LPCTSTR lpType, void *pResource, int &nBufSize)
{
  HRSRC hResInfo;
  HANDLE hRes;
  LPSTR lpRes = nullptr;
  // int nLen = 0;
  bool bResult = false;

  // Find the resource

  hResInfo = FindResource(m_hInst, lpName, lpType);
  if (hResInfo == nullptr) {
    // DWORD dwErr = GetLastError();
    return false;
  }

  // Load the resource
  hRes = LoadResource(m_hInst, hResInfo);

  if (hRes == nullptr)
    return false;

  // Lock the resource
  lpRes = (char *)LockResource(hRes);

  if (lpRes != nullptr) {
    if (pResource == nullptr) {
      nBufSize = SizeofResource(m_hInst, hResInfo);
      bResult = true;
    } else {
      if (nBufSize >= (int)SizeofResource(m_hInst, hResInfo)) {
        memcpy(pResource, lpRes, nBufSize);
        bResult = true;
      }
    }

    UnlockResource(hRes);
  }

  // Free the resource
  FreeResource(hRes);

  return bResult;
}


////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:	Load
//
// DESCRIPTION:	Helper function to load resource from memory
//
// RETURNS:
//
// NOTES:
//
// MODIFICATIONS:
//
// Name				Date		Version		Comments
// N T ALMOND       29012002	1.0			Origin
//
////////////////////////////////////////////////////////////////////////////////
bool ImageEx::Load(CString sResourceType, CString sResource)
{
  bool bResult = false;


  BYTE *pBuff = nullptr;
  int nSize = 0;
  if (GetResource(sResource.GetBuffer(0), sResourceType.GetBuffer(0), pBuff, nSize)) {
    if (nSize > 0) {
      pBuff = new BYTE[nSize];

      if (GetResource(sResource, sResourceType.GetBuffer(0), pBuff, nSize)) {
        if (LoadFromBuffer(pBuff, nSize)) {

          bResult = true;
        }
      }

      delete[] pBuff;
    }
  }


  m_bIsInitialized = bResult;

  return bResult;
}


////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:	GetSize
//
// DESCRIPTION:	Returns Width and Height object
//
// RETURNS:
//
// NOTES:
//
// MODIFICATIONS:
//
// Name				Date		Version		Comments
// N T ALMOND       29012002	1.0			Origin
//
////////////////////////////////////////////////////////////////////////////////
std::pair<UINT, UINT> ImageEx::GetSize()
{
  return std::make_pair(GetWidth(), GetHeight());
}


////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:	TestForAnimatedGIF
//
// DESCRIPTION:	Check GIF/Image for avialability of animation
//
// RETURNS:
//
// NOTES:
//
// MODIFICATIONS:
//
// Name				Date		Version		Comments
// N T ALMOND       29012002	1.0			Origin
//
////////////////////////////////////////////////////////////////////////////////
bool ImageEx::TestForAnimatedGIF()
{
  UINT count = 0;
  count = GetFrameDimensionsCount();
  auto *pDimensionIDs = new GUID[count];

  // Get the list of frame dimensions from the Image object.
  GetFrameDimensionsList(pDimensionIDs, count);

  // Get the number of frames in the first dimension.
  m_nFrameCount = GetFrameCount(&pDimensionIDs[0]);

  // Assume that the image has a property item of type PropertyItemEquipMake.
  // Get the size of that property item.
  int nSize = GetPropertyItemSize(PropertyTagFrameDelay);

  // Allocate a buffer to receive the property item.
  m_pPropertyItem = (PropertyItem *)malloc(nSize);

  GetPropertyItem(PropertyTagFrameDelay, nSize, m_pPropertyItem);

  delete pDimensionIDs;

  return m_nFrameCount > 1;
}


////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:	ImageEx::Initialize
//
// DESCRIPTION:	Common function called from Constructors
//
// RETURNS:
//
// NOTES:
//
// MODIFICATIONS:
//
// Name				Date		Version		Comments
// N T ALMOND       29012002	1.0			Origin
//
////////////////////////////////////////////////////////////////////////////////
void ImageEx::Initialize()
{
  m_pStream = nullptr;
  m_nFramePosition = 0;
  m_nFrameCount = 0;
  m_pStream = nullptr;
  lastResult = InvalidParameter;
  m_hThread = nullptr;
  m_bIsInitialized = false;
  m_pPropertyItem = nullptr;

#ifdef INDIGO_CTRL_PROJECT
  m_hInst = _Module.GetResourceInstance();
#else
  m_hInst = AfxGetResourceHandle();
#endif


  m_bPause = false;

  m_hExitEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

  m_hPause = CreateEvent(nullptr, TRUE, TRUE, nullptr);
}


////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:	ImageEx::_ThreadAnimationProc
//
// DESCRIPTION:	Thread to draw animated gifs
//
// RETURNS:
//
// NOTES:
//
// MODIFICATIONS:
//
// Name				Date		Version		Comments
// N T ALMOND       29012002	1.0			Origin
//
////////////////////////////////////////////////////////////////////////////////
UINT WINAPI ImageEx::_ThreadAnimationProc(LPVOID pParam)
{
  ASSERT(pParam);
  auto *pImage = reinterpret_cast<ImageEx *>(pParam);
  pImage->ThreadAnimation();

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:	ImageEx::ThreadAnimation
//
// DESCRIPTION:	Helper function
//
// RETURNS:
//
// NOTES:
//
// MODIFICATIONS:
//
// Name				Date		Version		Comments
// N T ALMOND       29012002	1.0			Origin
//
////////////////////////////////////////////////////////////////////////////////
void ImageEx::ThreadAnimation()
{
  m_nFramePosition = 0;

  bool bExit = false;
  while (bExit == false) {
    bExit = DrawFrameGIF();
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:	ImageEx::DrawFrameGIF
//
// DESCRIPTION:
//
// RETURNS:
//
// NOTES:
//
// MODIFICATIONS:
//
// Name				Date		Version		Comments
// N T ALMOND       29012002	1.0			Origin
//
////////////////////////////////////////////////////////////////////////////////
extern const HBRUSH black_brush;

bool ImageEx::DrawFrameGIF()
{

  ::WaitForSingleObject(m_hPause, INFINITE);

  GUID pageGuid = FrameDimensionTime;

  long hmWidth = GetWidth();
  long hmHeight = GetHeight();

  HDC hDC = GetDC(m_hWnd);
  if (hDC) {
    Graphics graphics(hDC);
    RECT imageRect{ m_pt.X, m_pt.Y, m_pt.X + hmWidth, m_pt.Y + hmHeight };
    FillRect(hDC, &imageRect, black_brush);
    graphics.DrawImage(this, m_pt.X, m_pt.Y, hmWidth, hmHeight);
    ReleaseDC(m_hWnd, hDC);
  }

  SelectActiveFrame(&pageGuid, m_nFramePosition++);

  if (m_nFramePosition == m_nFrameCount)
    m_nFramePosition = 0;


  long lPause = ((long *)m_pPropertyItem->value)[m_nFramePosition] * 10;

  DWORD dwErr = WaitForSingleObject(m_hExitEvent, lPause);

  return dwErr == WAIT_OBJECT_0;
}


////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:	ImageEx::SetPause
//
// DESCRIPTION:	Toggle Pause state of GIF
//
// RETURNS:
//
// NOTES:
//
// MODIFICATIONS:
//
// Name				Date		Version		Comments
// N T ALMOND       29012002	1.0			Origin
//
////////////////////////////////////////////////////////////////////////////////
void ImageEx::SetPause(bool bPause)
{
  if (!IsAnimatedGIF())
    return;

  if (bPause && !m_bPause) {
    ::ResetEvent(m_hPause);
  } else {

    if (m_bPause && !bPause) {
      ::SetEvent(m_hPause);
    }
  }

  m_bPause = bPause;
}


void ImageEx::Destroy()
{

  if (m_hThread) {
    // If pause unpause
    SetPause(false);

    SetEvent(m_hExitEvent);
    WaitForSingleObject(m_hThread, INFINITE);
  }

  CloseHandle(m_hThread);
  CloseHandle(m_hExitEvent);
  CloseHandle(m_hPause);

  free(m_pPropertyItem);

  m_pPropertyItem = nullptr;
  m_hThread = nullptr;
  m_hExitEvent = nullptr;
  m_hPause = nullptr;

  if (m_pStream)
    m_pStream->Release();
}