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

namespace lss_helper {

class MainWnd : public wxFrame {
public:
    MainWnd();
    ~MainWnd() override;

private:
    void onLoad();

    std::unique_ptr<lss::Lss> lss_;
};

}
