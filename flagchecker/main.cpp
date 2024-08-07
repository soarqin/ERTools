#include "mainwnd.h"

#include "game/eldenring.h"

namespace flagchecker {

class FlagCheckerApp : public wxApp {
public:
    bool OnInit() override {
        AllocConsole();
        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);

//        if (game::gEldenRing.searchForGame()) {
//            uintptr_t addr = game::gEldenRing.scanWithJump("48 8B 3D ?? ?? ?? ?? 48 85 FF ?? ?? 32 C0 E9", 3, 7);
//            fprintf(stdout, "0x%p\n", addr);
//        }

        (new MainWnd)->Show(true);
        return true;
    }
};

}

IMPLEMENT_APP(flagchecker::FlagCheckerApp)
