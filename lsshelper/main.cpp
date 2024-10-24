/*
 * Copyright (c) 2024 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "mainwnd.h"

namespace lss_helper {

class LssHelperApp : public wxApp {
public:
    bool OnInit() override {
#if !defined(NDEBUG)
        AllocConsole();
        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
#endif

        // MSWEnableDarkMode(wxApp::DarkMode_Auto);
        wxInitAllImageHandlers();
        (new MainWnd)->Show(true);
        return true;
    }
};

}

IMPLEMENT_APP(lss_helper::LssHelperApp)
