/*
 * Copyright (c) 2024 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "mainwnd.h"

#include "lss.h"

namespace lss_helper {

MainWnd::MainWnd() : wxFrame(nullptr, wxID_ANY, wxT("SoulSplitter lss helper"),
                             wxDefaultPosition, wxSize(800, 450)) {
    auto *bar = new wxMenuBar();
    auto *file = new wxMenu();
    file->Append(wxID_OPEN, wxT("&Open...\tCtrl-O"), wxT("Open a file"));
    file->Append(wxID_EXIT, wxT("E&xit\tAlt-X"), wxT("Quit this program"));
    bar->Append(file, wxT("&File"));

    auto *help = new wxMenu();
    help->Append(wxID_ABOUT, wxT("&About...\tF1"), wxT("Show about dialog"));
    bar->Append(help, wxT("&Help"));

    wxFrame::SetMenuBar(bar);

    Bind(wxEVT_MENU, [&](wxCommandEvent &event) {
        switch (event.GetId()) {
            case wxID_OPEN: {
                onLoad();
                break;
            }
            case wxID_EXIT:
                Close(true);
                break;
            case wxID_ABOUT: {
                wxMessageBox(wxT("SoulSplitter lss helper\n\n"
                                 "A tool to help you manage lss segments/splits\n\n"
                                 "Author: Soar Qin <soarchin@gmail.com>"), wxT("About"));
                break;
            }
            default:
                break;
        }
    });
}

MainWnd::~MainWnd() = default;

void MainWnd::onLoad() {
    wxFileDialog dialog(this, wxT("Open file"), wxEmptyString, wxEmptyString,
                        wxT("lss files (*.lss)|*.lss"), wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dialog.ShowModal() == wxID_CANCEL) {
        return;
    }
    auto path = dialog.GetPath();
    if (path.empty()) {
        return;
    }
    Lss lss(path);
    if (!lss.loaded()) {
        wxMessageBox(wxT("Invalid lss file"), wxT("Error"), wxICON_ERROR);
        return;
    }
}

}
