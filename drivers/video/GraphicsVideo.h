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

class GraphicsVideo : protected KernelUtil::TimerTask {
  private:
    GraphicsVideo(const framebuffer_info_t&);

  public:
    static void Create();
    static GraphicsVideo& Instance();
    unsigned FlatLFBAddress() const { return _flatLFBAddress; }
    void MappedLFBAddress(unsigned a)
    {
      _mappedLFBAddress = a;
      _zBuffer = a;
    }
    unsigned LFBSize() const { return _lfbSize; }
    uint32_t LFBPageCount() const { return _lfbPageCount; }

    void FillRect(unsigned sx, unsigned sy, unsigned width, unsigned height, unsigned color);
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

    uint32_t allocateFrameBuffer();

    void DebugPrint();

    private:
    typedef struct {
      bool _processChanged;
      bool _mouseChanged;
    } RedrawInfo;

    RedrawInfo isDirty();
    bool TimerTrigger() override;
    void NeedRefresh();
    void DrawUSFNChar(byte ch, unsigned x, unsigned y, unsigned fg, unsigned bg);
    void CopyArea(const uint32_t destX, const uint32_t destY,
                  const uint32_t srcX, const uint32_t srcY,
                  const uint32_t srcBufferWidth,
                  const uint32_t drawWidth, const uint32_t drawHeight,
                  const uint32_t* src, const bool checkAlpha);
    void DrawMouseCursor();

    static GraphicsVideo* _instance;
    unsigned _flatLFBAddress;
    unsigned _mappedLFBAddress;
    unsigned _zBuffer;
    unsigned _pitch;
    unsigned _width;
    unsigned _height;
    unsigned _lfbSize;
    uint32_t _lfbPageCount;
    upan::atomic::integral<bool> _needRefresh;
    byte     _bpp;
    byte     _bytesPerPixel;

    upanui::usfn::Context* _ssfnContext;
    bool     _initialized;
    uint32_t _xCharScale;
    uint32_t _yCharScale;
    upan::list<int> _fgProcesses;
    upan::list<int> _guiBaseStack;
    int _inputEventFGProcess;
    upan::mutex _fgProcessMutex;

    upan::uniq_ptr<upanui::MouseCursor> _mouseCursor;
    int _mousePrevX;
    int _mousePrevY;
    upan::atomic::integral<bool> _mouseChange;
};