/*
 * LSS Helper - LiveSplit lss file helper
 * Copyright (C) 2024  Soar Qin<soarchin@gmail.com>

 * This file is part of LSS Helper.

 * LSS Helper is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.

 * LSS Helper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with LSS Helper.  If not, see <http://www.gnu.org/licenses/>.
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
