#include "mainwnd.h"

namespace flagchecker {

class FlagCheckerApp : public wxApp {
public:
    bool OnInit() override {
        AllocConsole();
        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);

        MSWEnableDarkMode(wxApp::DarkMode_Always);
        wxInitAllImageHandlers();
        (new MainWnd)->Show(true);
        return true;
    }
};

}

IMPLEMENT_APP(flagchecker::FlagCheckerApp)
