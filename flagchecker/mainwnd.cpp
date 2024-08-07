/*
 * Copyright (c) 2022 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "mainwnd.h"
#include "version.h"

#include "game/eldenring.h"
#include "game/flags.h"

#include <wx/stdpaths.h>

namespace flagchecker {

MainWnd::MainWnd() : wxFrame(nullptr, wxID_ANY, wxT("ELDEN RING Flag Checker " VERSION_STR),
                             wxDefaultPosition, wxSize(800, 450)) {
    game::gFlags.loadFromFile("data.json");
    // wxInitAllImageHandlers();
    wxToolTip::Enable(true);
    wxToolTip::SetAutoPop(500);
    Centre();
    timer_ = new wxTimer(this, wxID_ANY);
    Bind(wxEVT_TIMER, [this](wxTimerEvent &event) {
        if (game::gEldenRing.running()) {
            return;
        }
        bool running = game::gEldenRing.searchForGame();
        if (running != gameRunning_) {
            gameRunning_ = running;
            SetStatusText(gameRunning_ ? "Game is running!" : "Game is not running!");
        }
        if (running) {
            game::gFlags.update();
            list_->Clear();
            for (const auto &flag: game::gFlags.flags()) {
                for (const auto &item: flag.flags) {
                    char s[256];
                    snprintf(s, 256, "%s [%s] %s", item.enabled ? "☑": "☐", flag.name.c_str(), item.name.c_str());
                    list_->Append(wxString::FromUTF8(s));
                }
            }
        }
    }, wxID_ANY);
    timer_->Start(1000);
    wxWindow::SetBackgroundColour(wxColour(243, 243, 243));
    wxFont font(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_MEDIUM, false, "Segoe UI");
    wxWindow::SetFont(font);
    auto *mainSizer = new wxBoxSizer(wxVERTICAL);
    list_ = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE);
    mainSizer->Add(list_, 1, wxEXPAND | wxALL, 5);
    SetStatusBar(new wxStatusBar(this, wxID_ANY));
    SetStatusText("Game is not running!");
}

MainWnd::~MainWnd() = default;

}
