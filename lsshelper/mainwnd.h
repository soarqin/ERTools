/*
 * Copyright (c) 2024 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#pragma once

#include "lss.h"

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/listctrl.h>

namespace lss_helper {

class MainWnd : public wxFrame {
public:
    MainWnd();
    ~MainWnd() override;

private:
    void onLoad();
    void moveUpButtonClicked();
    void moveDownButtonClicked();
    void assignButtonClicked();
    void unsignButtonClicked();
    void editSegmentButtonClicked(bool newSeg = false, bool insertBelow = false);
    void deleteSegmentButtonClicked();
    void deleteSplitButtonClicked();

    void doUnsign(int index, const SegNode &seg);
    void doUnsign(int index, const SplitNode &split);
    void assignSplitToSegment(int toIndex, int fromIndex, const SegNode &seg, const SplitNode &split);

private:
    Lss lss_;

    wxListView *segList_;
    wxListView *splitList_;
    wxButton *moveUp_, *moveDown_, *assign_, *unsign_, *editSegment_, *insertAbove_, *insertBelow_, *deleteSegment_, *deleteSplit_;
};

}
