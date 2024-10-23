/*
 * Copyright (c) 2024 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "mainwnd.h"

#include "lss.h"
#include "enums.h"

#include <fmt/format.h>

namespace lss_helper {

MainWnd::MainWnd() : wxFrame(nullptr, wxID_ANY, wxT("SoulSplitter lss helper"),
                             wxDefaultPosition, wxSize(1200, 800)) {
    auto *bar = new wxMenuBar();
    auto *file = new wxMenu();
    file->Append(wxID_OPEN, wxT("&Open...\tCtrl-O"), wxT("Open a file"));
    file->Append(wxID_SAVE, wxT("&Apply Changes\tCtrl-S"), wxT("Apply changes to current lss file, backup will be created"));
    file->Append(wxID_EXIT, wxT("E&xit\tAlt-X"), wxT("Quit this program"));
    bar->Append(file, wxT("&File"));

    auto *help = new wxMenu();
    help->Append(wxID_ABOUT, wxT("&About...\tF1"), wxT("Show about dialog"));
    bar->Append(help, wxT("&Help"));

    wxFrame::SetMenuBar(bar);

    auto *sizer = new wxBoxSizer(wxHORIZONTAL);
    wxFrame::SetSizer(sizer);

    /* Left */
    segList_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);

    /* Middle */
    auto *middlePanel = new wxPanel(this, wxID_ANY);
    auto *middleSizer = new wxBoxSizer(wxVERTICAL);
    middlePanel->SetSizer(middleSizer);
    auto *toLeft = new wxButton(middlePanel, wxID_ANY, wxT(" << "), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    auto *toRight = new wxButton(middlePanel, wxID_ANY, wxT(" >> "), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    middleSizer->Add(toLeft, 0, wxALIGN_CENTER_HORIZONTAL);
    middleSizer->AddSpacer(10);
    middleSizer->Add(toRight, 0, wxALIGN_CENTER_HORIZONTAL);

    /* Right */
    splitList_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);

    sizer->Add(segList_, 1, wxEXPAND);
    sizer->Add(middlePanel, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(splitList_, 1, wxEXPAND);

    segList_->AppendColumn(wxT("Segment Name"));
    segList_->AppendColumn(wxT("Assigned Split"));
    segList_->SetColumnWidth(0, 150);
    segList_->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);

    splitList_->AppendColumn(wxT("When"));
    splitList_->AppendColumn(wxT("Type"));
    splitList_->AppendColumn(wxT("Split"));
    splitList_->SetColumnWidth(0, 150);
    splitList_->SetColumnWidth(1, 100);
    splitList_->SetColumnWidth(2, wxLIST_AUTOSIZE_USEHEADER);

    Bind(wxEVT_MENU, [&](wxCommandEvent &event) {
        switch (event.GetId()) {
            case wxID_OPEN: {
                onLoad();
                break;
            }
            case wxID_SAVE: {
                lss_.save();
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
    toLeft->Bind(wxEVT_BUTTON, [&](wxCommandEvent &event) {
        assignButtonClicked();
    });
    toRight->Bind(wxEVT_BUTTON, [&](wxCommandEvent &event) {
        removeButtonClicked();
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
    auto res = lss_.open(path.wc_str());
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

    const auto &segs = lss_.segs();
    segList_->DeleteAllItems();
    size_t sz = segs.size();
    for (size_t i = 0; i < sz; i++) {
        const auto &seg = segs[i];
        auto index = segList_->InsertItem(long(i), wxString::FromUTF8(seg.seg.child("Name").text().get()));
        if (!seg.split.empty()) segList_->SetItem(index, 1, wxString::FromUTF8(seg.splitName));
        segList_->SetItemData(index, long(i));
    }
    segList_->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
    segList_->SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);

    splitList_->DeleteAllItems();
    const auto &splits = lss_.splits();
    auto sz1 = splits.size();
    for (size_t i = 0; i < sz1; i++) {
        const auto &split = splits[i];
        auto index = splitList_->InsertItem(splitList_->GetItemCount(), wxString::FromUTF8(split.when));
        splitList_->SetItem(index, 1, wxString::FromUTF8(split.type));
        splitList_->SetItem(index, 2, wxString::FromUTF8(split.displayName));
        splitList_->SetItemData(index, long(i));
        splitList_->SetItemTextColour(index, split.seg ? wxColour(96, 96, 96) : splitList_->GetTextColour());
    }
    splitList_->SetColumnWidth(2, wxLIST_AUTOSIZE_USEHEADER);
}

void MainWnd::assignButtonClicked() {
    auto toIndex = segList_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (toIndex == -1) return;
    auto fromIndex = splitList_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (fromIndex == -1) return;
    auto index = segList_->GetItemData(toIndex);
    auto sindex = splitList_->GetItemData(fromIndex);
    auto &segData = lss_.segs()[index];
    auto &splitData = lss_.splits()[sindex];
    segData.assign(splitData.split, splitData.fullDisplayName);
    splitData.assign(segData.seg);
    segList_->SetItem(toIndex, 1, wxString::FromUTF8(splitData.fullDisplayName));
    splitList_->SetItemTextColour(fromIndex, wxColour(96, 96, 96));
    while (++toIndex < segList_->GetItemCount()) {
        if (lss_.segs()[segList_->GetItemData(toIndex)].split.empty()) {
            segList_->SetItemState(toIndex, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
            break;
        }
    }
}

void MainWnd::removeButtonClicked() {
    auto toIndex = segList_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (toIndex == -1) return;
    auto index = segList_->GetItemData(toIndex);
    auto &segData = lss_.segs()[index];
    segList_->SetItem(toIndex, 1, wxEmptyString);
    const auto &splits = lss_.splits();
    auto cnt = splitList_->GetItemCount();
    for (long i = 0; i < cnt; i++) {
        auto data = splitList_->GetItemData(i);
        const auto &split = splits[data];
        if (split.split == segData.split) {
            split.assign(pugi::xml_node());
            splitList_->SetItemTextColour(i, splitList_->GetTextColour());
            break;
        }
    }
    segData.assign(pugi::xml_node(), "");
}

}
