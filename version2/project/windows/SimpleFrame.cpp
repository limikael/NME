#include <Graphics.h>
#include <Display.h>
#include <Surface.h>
#include <windows.h>
#include <map>

#include <gl/GL.h>

namespace nme
{

// --- DIB   ------------------------------------------------------------------------

typedef std::map<HWND,class WindowsFrame *> FrameMap;
static FrameMap sgFrameMap;

class DIBSurface : public SimpleSurface
{
public:
   DIBSurface(int inW,int inH) : SimpleSurface( (inW+3) & ~3 ,inH,pfXRGB,4)
   {
      memset(&mInfo,0,sizeof(mInfo));
      BITMAPINFOHEADER &h = mInfo.bmiHeader;
      h.biSize = sizeof(BITMAPINFOHEADER);
      h.biWidth = mWidth;
      h.biHeight = -mHeight;
      h.biPlanes = 1;
      h.biBitCount = 32;
      h.biCompression = BI_RGB;

      memset(mBase, 0, mWidth*mHeight*4);
   }

   void RenderTo(HDC inDC)
   {
       SetDIBitsToDevice(inDC,0,0,mWidth,mHeight,
                        0,0, 0,mHeight, mBase, &mInfo, DIB_RGB_COLORS);
   }

   BITMAPINFO mInfo;

private:
   ~DIBSurface() { }
};


// --- Stage ------------------------------------------------------------------------

enum
{
   PollTimerID = 1,
};

typedef std::vector<Stage *> StageList;
StageList sgAlwaysPollingStages;

class WindowsStage : public Stage
{
public:
   WindowsStage(HWND inHWND,uint32 inFlags)
   {
      mHWND = inHWND;
      mDC = GetDC(mHWND);
      SetICMMode(mDC,ICM_OFF);
      mFlags = inFlags;
      mBMP = 0;
      mHardwareContext = 0;
      mHardwareSurface = 0;
      mOGLCtx = 0;
      HintColourOrder(false);
      mPollingTimer = false;

      mIsHardware = inFlags & wfHardware;
      mPollMethod = pollAlways;
      sgAlwaysPollingStages.push_back(this);

      if (mIsHardware)
      {
         if (!CreateHardware())
            mIsHardware = false;
      }
      if (!mIsHardware)
         CreateBMP();
   }

    void SetPollMethod(PollMethod inMethod)
    {
       if (inMethod!=mPollMethod)
       {
          if (mPollMethod==pollAlways)
          {
             StageList::iterator i  = std::find(sgAlwaysPollingStages.begin(),
                                                sgAlwaysPollingStages.end(), this );
             if (i!=sgAlwaysPollingStages.end())
                sgAlwaysPollingStages.erase(i);
          }

          if (inMethod==pollTimer)
          {
             if (!mPollingTimer)
                SetTimer(mHWND,PollTimerID, 1,0);
             mPollingTimer = true;
          }
          else
          {
             if (mPollingTimer)
                KillTimer(mHWND,PollTimerID);
             mPollingTimer = false;
          }

          mPollMethod = inMethod;

          if (mPollMethod==pollAlways)
             sgAlwaysPollingStages.push_back(this);
       }
    }

   void PollNow()
   {
      Event evt(etPoll);
      HandleEvent(evt);
   }


   ~WindowsStage()
   {
      SetPollMethod(pollNever);
      if (mBMP)
         mBMP->DecRef();
      if (mHardwareContext)
         mHardwareContext->DecRef();
      if (mHardwareSurface)
         mHardwareSurface->DecRef();
      if (mOGLCtx)
         wglDeleteContext( mOGLCtx );
   }

   bool CreateHardware()
   {
      PIXELFORMATDESCRIPTOR pfd;
      ZeroMemory( &pfd, sizeof( pfd ) );
      pfd.nSize = sizeof( pfd );
      pfd.nVersion = 1;
      pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
                    PFD_DOUBLEBUFFER;
      pfd.iPixelType = PFD_TYPE_RGBA;
      pfd.cColorBits = 24;
      pfd.cDepthBits = 16;
      pfd.iLayerType = PFD_MAIN_PLANE;
      int fmt = ChoosePixelFormat( mDC, &pfd );
      if (!fmt)
         return false;
      if (!SetPixelFormat( mDC, fmt, &pfd ))
         return false;

      mOGLCtx = wglCreateContext( mDC );
      if (!mOGLCtx)
         return false;

      mHardwareContext = HardwareContext::CreateOpenGL(mDC,mOGLCtx);
      mHardwareContext->IncRef();
      mHardwareSurface = new HardwareSurface(mHardwareContext);
      mHardwareSurface->IncRef();
      UpdateHardware();
      return true;
   }


   void UpdateHardware()
   {
      WINDOWINFO info;
      info.cbSize = sizeof(WINDOWINFO);

      if (GetWindowInfo(mHWND,&info))
      {
         int w =  info.rcClient.right - info.rcClient.left;
         int h =  info.rcClient.bottom - info.rcClient.top;
         mHardwareContext->SetWindowSize(w,h);
      }
   }

   void CreateBMP()
   {
      if (mBMP)
      {
         mBMP->DecRef();
         mBMP = 0;
      }

      WINDOWINFO info;
      info.cbSize = sizeof(WINDOWINFO);

      if (GetWindowInfo(mHWND,&info))
      {
         int w =  info.rcClient.right - info.rcClient.left;
         int h =  info.rcClient.bottom - info.rcClient.top;
         mBMP = new DIBSurface(w,h);
         mBMP->IncRef();
      }
   }

   void Flip()
   {
      if (mHardwareContext)
         mHardwareContext->Flip();
      else if (mBMP)
         mBMP->RenderTo(mDC);
   }
   void GetMouse()
   {
   }

   Surface *GetPrimarySurface()
   {
      if (mHardwareSurface)
         return mHardwareSurface;
      return mBMP;
   }

   void HandleEvent(Event &inEvent)
   {
      switch(inEvent.type)
      {
         case etRedraw:
            Flip();
            break;
         case etResize:
            if (mHardwareContext)
               UpdateHardware();
            else
               CreateBMP();
            break;
      }

      Stage::HandleEvent(inEvent);
   }


   // --- IRenderTarget Interface ------------------------------------------
   int Width()
   {
      WINDOWINFO info;
      info.cbSize = sizeof(WINDOWINFO);

      if (!GetWindowInfo(mHWND,&info))
         return 0;
      return info.rcClient.right - info.rcClient.left;
   }

   int Height()
   {
      WINDOWINFO info;
      info.cbSize = sizeof(WINDOWINFO);

      if (!GetWindowInfo(mHWND,&info))
         return 0;

      return info.rcClient.bottom - info.rcClient.top;
   }


   HWND         mHWND;
   HDC          mDC;
   HGLRC        mOGLCtx;
   uint32       mFlags;
   double       mFrameRate;
   DIBSurface   *mBMP;
   HardwareSurface *mHardwareSurface;
   HardwareContext *mHardwareContext;
   bool         mIsHardware;
   bool         mPollingTimer;
   PollMethod   mPollMethod;
};


// --- Frame ------------------------------------------------------------------------


class WindowsFrame : public Frame
{
public:
   WindowsFrame(HWND inHandle, uint32 inFlags)
   {
      mFlags = inFlags;
      mHandle = inHandle;
      sgFrameMap[mHandle] = this;
      mOldProc = (WNDPROC)SetWindowLongPtr(mHandle,GWL_WNDPROC,(LONG)StaticCallback);
      mStage = new WindowsStage(inHandle,mFlags);
      ShowWindow(mHandle,true);
   }
   ~WindowsFrame()
   {
      SetWindowLongPtr(mHandle,GWL_WNDPROC,(LONG)mOldProc);
      sgFrameMap.erase(mHandle);
   }

   LRESULT Callback(UINT uMsg, WPARAM wParam, LPARAM lParam)
   {
      switch (uMsg)
      {
         case WM_CLOSE:
            TerminateMainLoop();
            break;
         case WM_PAINT:
            {
            PAINTSTRUCT ps;
            HDC dc;
            BeginPaint(mHandle,&ps);
            Event evt(etRedraw);
            mStage->HandleEvent(evt);
            EndPaint(mHandle,&ps);
            }
            break;
         case WM_SIZE:
            {
            Event evt(etResize);
            mStage->HandleEvent(evt);
            }
            break;
         case WM_MOUSEMOVE:
            {
            Event evt(etMouseMove, LOWORD(lParam), HIWORD(lParam));
            mStage->HandleEvent(evt);
            }
            break;
			case WM_LBUTTONDOWN:
            {
            Event evt(etMouseDown, LOWORD(lParam), HIWORD(lParam));
            mStage->HandleEvent(evt);
            }
            break;
			case WM_LBUTTONUP:
            {
            Event evt(etMouseUp, LOWORD(lParam), HIWORD(lParam));
            mStage->HandleEvent(evt);
				// TODO: based on timer/motion?
            Event click(etMouseClick, LOWORD(lParam), HIWORD(lParam));
            mStage->HandleEvent(click);
            }
            break;
			// TODO : Create click event


         case WM_TIMER:
            {
            Event evt(etPoll);
            evt.id = wParam;
            mStage->HandleEvent(evt);
            }
            break;
      }

      return mOldProc(mHandle, uMsg, wParam, lParam);
   }

   static LRESULT CALLBACK StaticCallback( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
   {
      FrameMap::iterator i = sgFrameMap.find(hwnd);
      if (i!=sgFrameMap.end())
         return i->second->Callback(uMsg,wParam,lParam);
      return DefWindowProc(hwnd, uMsg, wParam, lParam);
   }

   // --- Frame Interface ----------------------------------------------------

   void SetTitle()
   {
   }
   void SetIcon()
   {
   }
   Stage *GetStage()
   {
      return mStage;
   }


   WindowsStage *mStage;

   uint32 mFlags;
   HWND mHandle;
   WNDPROC mOldProc;
};


// --- When using the simple window class -----------------------------------------------

Frame *CreateMainFrame(int inWidth,int inHeight,unsigned int inFlags,
                       const char *inTitle,const char *inIcon)
{
   Rect r(100,100,inWidth,inHeight);

   WNDCLASSEX wc;
   memset(&wc,0,sizeof(wc));
   wc.cbSize = sizeof(wc);
   wc.style = CS_OWNDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
   wc.hbrBackground = 0; //(HBRUSH)GetStockObject(WHITE_BRUSH);
   wc.lpfnWndProc =  DefWindowProc;
   wc.lpszClassName = "NME";

   RegisterClassEx(&wc);

   DWORD ex_style = WS_EX_ACCEPTFILES;
   DWORD style = 0;
   if (inFlags & wfResizable)
      style |= WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_MAXIMIZEBOX;
   if (inFlags & wfBorderless)
      style |= WS_POPUP;
   else
      style |= WS_OVERLAPPEDWINDOW;

   HWND win = CreateWindowEx(ex_style, "NME", inTitle,
                              style,
                              r.x, r.y, r.w, r.h,
                              0,
                              0,
                              0,
                              0);

   Frame *frame = new WindowsFrame(win,inFlags);
   SetCursor(LoadCursor(0, IDC_ARROW));
   return frame;
}


bool sgDead = false;

void TerminateMainLoop()
{
   sgDead =true;
}

void MainLoop()
{
   MSG msg;
   while( !sgDead )
   {
      while(!sgDead && !sgAlwaysPollingStages.empty() &&
          PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)==0)
      {
         for(int i=0;i<sgAlwaysPollingStages.size();i++)
            sgAlwaysPollingStages[i]->PollNow();
      }

      if (GetMessage(&msg, NULL, 0, 0)<=0)
         break;

      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }
}

} // end namespace nme
