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

    auto *sizer = new wxBoxSizer(wxHORIZONTAL);
    wxFrame::SetSizer(sizer);
    segList_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_NO_SORT_HEADER | wxLC_SINGLE_SEL);
    segList_->InsertColumn(0, wxT("Name"));
    segList_->InsertColumn(1, wxT("Assigned Split"));
    segList_->SetColumnWidth(0, 120);
    segList_->SetColumnWidth(1, 150);
    sizer->Add(segList_, 1, wxEXPAND);

    splitList_ = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    sizer->Add(splitList_, 1, wxEXPAND);

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
    lss_ = std::make_unique<Lss>();
    auto res = lss_->open(path.wc_str());
    switch (res) {
        case -1: {
            wxMessageBox(wxT("Invalid lss file"), wxT("Error"), wxICON_ERROR);
            break;
        }
        case -2: {
            wxMessageBox(wxT("Unsupported game"), wxT("Error"), wxICON_ERROR);
            break;
        }
        default: break;
    }

    segList_->DeleteAllItems();
    const auto &segs = lss_->segs();
    size_t sz = segs.size();
    bool hasSplit = false;
    for (size_t i = 0; i < sz; i++) {
        const auto &seg = segs[i];
        auto index = segList_->InsertItem(segList_->GetItemCount(), wxString::FromUTF8(seg.seg.child("Name").text().get()));
        if (seg.split) {
            segList_->SetItem(index, 1, wxString::FromUTF8(seg.split.text().get()));
            hasSplit = true;
        }
        segList_->SetItemData(index, (long)i);
    }
    segList_->SetColumnWidth(0, sz > 0 ? wxLIST_AUTOSIZE : 120);
    segList_->SetColumnWidth(1, hasSplit ? wxLIST_AUTOSIZE : 150);

    splitList_->Clear();
    for (const auto &split: lss_->splits()) {
        const auto &timingType = split.type;
        for (const auto &stype: split.splitTypes) {
            const auto &splitType = stype.type;
            for (const auto &node: stype.nodes) {
                splitList_->Append(wxString::Format(wxT("[%s/%s] %s"), wxString::FromUTF8(timingType.c_str()), wxString::FromUTF8(splitType.c_str()), wxString::FromUTF8(node.text().get())));
            }
        }
    }
}

}
