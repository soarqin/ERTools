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
    void assignButtonClicked();
    void removeButtonClicked();
    void assignNewSplit();

    void removeAssigned(int index, const SegNode &seg);
    void removeAssigned(int index, const SplitNode &split);
    void assignSplitToSeg(int toIndex, int fromIndex, const SegNode &seg, const SplitNode &split);

private:
    Lss lss_;

    wxListView *segList_;
    wxListView *splitList_;
    wxButton *toLeft_, *toRight_, *newSplit_;
};

}
