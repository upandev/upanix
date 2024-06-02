/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
 *
 *  I am making my contributions/submissions to this project solely in
 *  my personal capacity and am not conveying any rights to any
 *  intellectual property of any third parties.
 *                                                                          
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *                                                                          
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *                                                                          
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/
 */
#pragma once

#include <MultiBoot.h>
#include <KernelUtil.h>
#include <usfncontext.h>
#include <atomicop.h>
#include <list.h>
#include <vector.h>
#include <mutex.h>
#include <MouseCursor.h>
#include <MouseData.h>

class GraphicsVideo : protected KernelUtil::TimerTask {
  private:
    GraphicsVideo(const FrameBufferInfo&);

  public:
    static void Create();
    static GraphicsVideo& Instance();
    uint64_t FlatLFBAddress() const { return _flatLFBAddress; }
    void MappedLFBAddress(uint64_t a)
    {
      _mappedLFBAddress = a;
      _zBuffer = a;
    }
    unsigned LFBSize() const { return _lfbSize; }
    uint32_t LFBPageCount() const { return _lfbPageCount; }

    void FillRect(int sx, int sy, int width, int height, unsigned color);
    void CreateRefreshTask();
    void Initialize();

    void SetMouseCursorPos(int x, int y);
    upan::option<int> getFGProcessUnderMouseCursor();
    void addGuiBase(int pid);
    void removeGuiBase(int pid);
    void switchFGProcess(int pid);

    void addFGProcess(int pid);
    void removeFGProcess(int pid);
    upan::option<int> getDisplayFGProcess();
    int getInputEventFGProcess() {
      return _inputEventFGProcess;
    }

    uint64_t allocateFrameBuffer();

    void DebugPrint();

    private:
    typedef struct {
      bool _processChanged;
      bool _mouseChanged;
    } RedrawInfo;

    RedrawInfo isDirty();
    bool TimerTrigger() override;
    void NeedRefresh();
    void CopyArea(int destX, int destY,
                  int srcX, int srcY,
                  int srcBufferWidth,
                  int drawWidth, int drawHeight,
                  const uint32_t* src, const bool checkAlpha);
    void DrawMouseCursor();

    static GraphicsVideo* _instance;
    uint64_t _flatLFBAddress;
    uint64_t _mappedLFBAddress;
    uint64_t _zBuffer;
    int _pitch;
    int _width;
    int _height;
    int _lfbSize;
    uint32_t _lfbPageCount;
    upan::atomic::integral<bool> _needRefresh;
    byte     _bpp;
    byte     _bytesPerPixel;

    upanui::usfn::Context* _ssfnContext;
    bool     _initialized;
    int _xCharScale;
    int _yCharScale;
    upan::list<int> _fgProcesses;
    upan::list<int> _guiBaseStack;
    int _inputEventFGProcess;
    upan::mutex _fgProcessMutex;

    upan::uniq_ptr<upanui::MouseCursor> _mouseCursor;
    upan::uniq_ptr<upanui::Image> _mousePointerImage;
    upan::uniq_ptr<upanui::Image> _mouseResizerImage;

    int _mousePrevX;
    int _mousePrevY;
    int _mousePrevWidth;
    int _mousePrevHeight;
    upan::atomic::integral<bool> _mouseChange;
};