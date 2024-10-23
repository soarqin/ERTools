/*
 * Copyright (c) 2024 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#pragma once

#include "lss.h"
#include "enums.h"

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

private:
    Lss lss_;
    Enums enums_;

    wxListCtrl *segList_;
    wxListCtrl *splitList_;
};

}
