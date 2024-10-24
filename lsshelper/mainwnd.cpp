/*
 * Copyright (c) 2024 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "mainwnd.h"

#include "splitdlg.h"

#include "lss.h"
#include "enums.h"

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
    segList_ = new wxListView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);

    /* Middle */
    auto *middlePanel = new wxPanel(this, wxID_ANY);
    auto *middleSizer = new wxBoxSizer(wxVERTICAL);
    middlePanel->SetSizer(middleSizer);
    toLeft_ = new wxButton(middlePanel, wxID_ANY, wxT(" << "), wxDefaultPosition, wxSize(50, 30));
    toRight_ = new wxButton(middlePanel, wxID_ANY, wxT(" >> "), wxDefaultPosition, wxSize(50, 30));
    newSplit_ = new wxButton(middlePanel, wxID_ANY, wxT(" ++ "), wxDefaultPosition, wxSize(50, 30));
    middleSizer->Add(toLeft_, 0, wxALIGN_CENTER_HORIZONTAL);
    middleSizer->AddSpacer(30);
    middleSizer->Add(toRight_, 0, wxALIGN_CENTER_HORIZONTAL);
    middleSizer->AddSpacer(30);
    middleSizer->Add(newSplit_, 0, wxALIGN_CENTER_HORIZONTAL);
    toLeft_->Enable(false);
    toRight_->Enable(false);
    newSplit_->Enable(false);

    /* Right */
    splitList_ = new wxListView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);

    sizer->Add(segList_, 1, wxEXPAND);
    sizer->Add(middlePanel, 0, wxEXPAND);
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
    segList_->Bind(wxEVT_LIST_ITEM_SELECTED, [&](wxListEvent &event) {
        auto index = event.GetIndex();
        auto enable = index >= 0;
        toLeft_->Enable(enable && splitList_->GetFirstSelected() >= 0);
        auto data = segList_->GetItemData(index);
        auto &seg = lss_.segs()[data];
        toRight_->Enable(seg.split);
        newSplit_->Enable(enable);
    });
    segList_->Bind(wxEVT_LIST_ITEM_DESELECTED, [&](wxListEvent &event) {
        toLeft_->Enable(false);
        toRight_->Enable(false);
        newSplit_->Enable(false);
    });
    splitList_->Bind(wxEVT_LIST_ITEM_SELECTED, [&](wxListEvent &event) {
        auto index = event.GetIndex();
        auto enable = index >= 0;
        toLeft_->Enable(enable && segList_->GetFirstSelected() >= 0);
    });
    splitList_->Bind(wxEVT_LIST_ITEM_DESELECTED, [&](wxListEvent &event) {
        toLeft_->Enable(false);
    });
    toLeft_->Bind(wxEVT_BUTTON, [&](wxCommandEvent &event) {
        assignButtonClicked();
    });
    toRight_->Bind(wxEVT_BUTTON, [&](wxCommandEvent &event) {
        removeButtonClicked();
    });
    newSplit_->Bind(wxEVT_BUTTON, [&](wxCommandEvent &event) {
        auto *dlg = new SplitDlg(this);
        dlg->ShowModal();
        dlg->Destroy();
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
    segList_->Select(0);

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
    auto toIndex = segList_->GetFirstSelected();
    if (toIndex == -1) return;
    auto fromIndex = splitList_->GetFirstSelected();
    if (fromIndex == -1) return;
    auto index = segList_->GetItemData(toIndex);
    auto sindex = splitList_->GetItemData(fromIndex);
    const auto &segs = lss_.segs();
    auto &segData = segs[index];
    const auto &splits = lss_.splits();
    auto &splitData = splits[sindex];
    if (splitData.seg) {
        auto sz = segList_->GetItemCount();
        for (int i = 0; i < sz; i++) {
            auto data = segList_->GetItemData(i);
            const auto &seg = segs[data];
            if (seg.seg == splitData.seg) {
                seg.assign(pugi::xml_node(), "");
                segList_->SetItem(i, 1, wxEmptyString);
                break;
            }
        }
        splitList_->SetItemTextColour(fromIndex, splitList_->GetTextColour());
    }
    segData.assign(splitData.split, splitData.fullDisplayName);
    splitData.assign(segData.seg);
    segList_->SetItem(toIndex, 1, wxString::FromUTF8(splitData.fullDisplayName));
    splitList_->SetItemTextColour(fromIndex, wxColour(96, 96, 96));
    auto segCnt = segList_->GetItemCount();
    while (++toIndex < segCnt) {
        if (segs[segList_->GetItemData(toIndex)].split.empty()) {
            segList_->Select(toIndex);
            break;
        }
    }
}

void MainWnd::removeButtonClicked() {
    auto toIndex = segList_->GetFirstSelected();
    if (toIndex == -1) return;
    auto index = segList_->GetItemData(toIndex);
    auto &segData = lss_.segs()[index];
    if (!segData.split) return;
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
