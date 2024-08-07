/*
 * Copyright (c) 2022 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/listctrl.h>

namespace flagchecker {

class MainWnd : public wxFrame {
public:
    MainWnd();
    ~MainWnd() override;

private:
    bool gameRunning_ = false;
    wxTimer *timer_;
    wxListBox *list_;
};

}
