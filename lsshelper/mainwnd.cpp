/*
 * Copyright (c) 2024 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "mainwnd.h"

#include "editsegmentdlg.h"

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
    moveUp_ = new wxButton(middlePanel, wxID_ANY, wxT("Move Up"), wxDefaultPosition, wxSize(100, 30));
    moveDown_ = new wxButton(middlePanel, wxID_ANY, wxT("Move Down"), wxDefaultPosition, wxSize(100, 30));
    assign_ = new wxButton(middlePanel, wxID_ANY, wxT("Assign"), wxDefaultPosition, wxSize(100, 30));
    unsign_ = new wxButton(middlePanel, wxID_ANY, wxT("Unsign"), wxDefaultPosition, wxSize(100, 30));
    editSegment_ = new wxButton(middlePanel, wxID_ANY, wxT("Edit"), wxDefaultPosition, wxSize(100, 30));
    insertAbove_ = new wxButton(middlePanel, wxID_ANY, wxT("Insert Above"), wxDefaultPosition, wxSize(100, 30));
    insertBelow_ = new wxButton(middlePanel, wxID_ANY, wxT("Insert Below"), wxDefaultPosition, wxSize(100, 30));
    deleteSegment_ = new wxButton(middlePanel, wxID_ANY, wxT("Delete Segment"), wxDefaultPosition, wxSize(100, 30));
    deleteSplit_ = new wxButton(middlePanel, wxID_ANY, wxT("Delete Split"), wxDefaultPosition, wxSize(100, 30));

    middleSizer->Add(moveUp_, 0, wxALIGN_CENTER_HORIZONTAL);
    middleSizer->AddSpacer(10);
    middleSizer->Add(moveDown_, 0, wxALIGN_CENTER_HORIZONTAL);
    middleSizer->AddSpacer(40);
    middleSizer->Add(assign_, 0, wxALIGN_CENTER_HORIZONTAL);
    middleSizer->AddSpacer(10);
    middleSizer->Add(unsign_, 0, wxALIGN_CENTER_HORIZONTAL);
    middleSizer->AddSpacer(40);
    middleSizer->Add(editSegment_, 0, wxALIGN_CENTER_HORIZONTAL);
    middleSizer->AddSpacer(10);
    middleSizer->Add(insertAbove_, 0, wxALIGN_CENTER_HORIZONTAL);
    middleSizer->AddSpacer(10);
    middleSizer->Add(insertBelow_, 0, wxALIGN_CENTER_HORIZONTAL);
    middleSizer->AddSpacer(40);
    middleSizer->Add(deleteSegment_, 0, wxALIGN_CENTER_HORIZONTAL);
    middleSizer->AddSpacer(10);
    middleSizer->Add(deleteSplit_, 0, wxALIGN_CENTER_HORIZONTAL);
    moveUp_->Enable(false);
    moveDown_->Enable(false);
    assign_->Enable(false);
    unsign_->Enable(false);
    editSegment_->Enable(false);
    insertAbove_->Enable(false);
    insertBelow_->Enable(false);
    deleteSegment_->Enable(false);
    deleteSplit_->Enable(false);

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

    Bind(wxEVT_CLOSE_WINDOW, [&](wxCloseEvent &event) {
        if (lss_.changed()) {
            if (wxMessageBox(wxT("Save changes?"), wxT("Save"), wxYES_NO) != wxYES) {
                event.Veto();
                return;
            }
            lss_.save();
        }
        event.Skip();
    });
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
        moveUp_->Enable(index > 0);
        moveDown_->Enable(enable && index < segList_->GetItemCount() - 1);
        assign_->Enable(enable && splitList_->GetFirstSelected() >= 0);
        auto &seg = lss_.segs()[index];
        unsign_->Enable(seg.split);
        editSegment_->Enable(enable);
        insertAbove_->Enable(enable);
        insertBelow_->Enable(enable);
        deleteSegment_->Enable(enable);
    });
    segList_->Bind(wxEVT_LIST_ITEM_DESELECTED, [&](wxListEvent &event) {
        moveUp_->Enable(false);
        moveDown_->Enable(false);
        assign_->Enable(false);
        unsign_->Enable(false);
        editSegment_->Enable(false);
        insertAbove_->Enable(false);
        insertBelow_->Enable(false);
        deleteSegment_->Enable(false);
    });
    segList_->Bind(wxEVT_LIST_ITEM_ACTIVATED, [&](wxListEvent &event) {
        auto index = segList_->GetFirstSelected();
        if (index < 0) return;
        auto &seg = lss_.segs()[index];
        editSegmentButtonClicked();
    });
    splitList_->Bind(wxEVT_LIST_ITEM_SELECTED, [&](wxListEvent &event) {
        auto index = event.GetIndex();
        auto enable = index >= 0;
        assign_->Enable(enable && segList_->GetFirstSelected() >= 0);
        deleteSplit_->Enable(enable);
    });
    splitList_->Bind(wxEVT_LIST_ITEM_DESELECTED, [&](wxListEvent &event) {
        assign_->Enable(false);
        deleteSplit_->Enable(false);
    });
    splitList_->Bind(wxEVT_LIST_ITEM_ACTIVATED, [&](wxListEvent &event) {
        assignButtonClicked();
    });
    moveUp_->Bind(wxEVT_BUTTON, [&](wxCommandEvent &event) { moveUpButtonClicked(); });
    moveDown_->Bind(wxEVT_BUTTON, [&](wxCommandEvent &event) { moveDownButtonClicked(); });
    assign_->Bind(wxEVT_BUTTON, [&](wxCommandEvent &event) { assignButtonClicked(); });
    unsign_->Bind(wxEVT_BUTTON, [&](wxCommandEvent &event) { unsignButtonClicked(); });
    editSegment_->Bind(wxEVT_BUTTON, [&](wxCommandEvent &event) { editSegmentButtonClicked(); });
    insertAbove_->Bind(wxEVT_BUTTON, [&](wxCommandEvent &event) { editSegmentButtonClicked(true, false); });
    insertBelow_->Bind(wxEVT_BUTTON, [&](wxCommandEvent &event) { editSegmentButtonClicked(true, true); });
    deleteSegment_->Bind(wxEVT_BUTTON, [&](wxCommandEvent &event) { deleteSegmentButtonClicked(); });
    deleteSplit_->Bind(wxEVT_BUTTON, [&](wxCommandEvent &event) { deleteSplitButtonClicked(); });
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
        segList_->InsertItem(long(i), wxString::FromUTF8(seg.seg.child("Name").text().get()));
        if (!seg.split.empty()) segList_->SetItem(long(i), 1, seg.splitName);
    }
    segList_->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
    segList_->Select(0);
    segList_->EnsureVisible(0);
    int selIdx = 0;
    auto segCnt = segList_->GetItemCount();
    while (selIdx < segCnt) {
        if (segs[selIdx].split.empty()) {
            segList_->Select(selIdx);
            segList_->EnsureVisible(selIdx);
            break;
        }
        selIdx++;
    }

    splitList_->DeleteAllItems();
    const auto &splits = lss_.splits();
    auto sz1 = splits.size();
    for (size_t i = 0; i < sz1; i++) {
        const auto &split = splits[i];
        auto index = splitList_->InsertItem(splitList_->GetItemCount(), wxString::FromUTF8(split.when));
        splitList_->SetItem(index, 1, wxString::FromUTF8(split.type));
        splitList_->SetItem(index, 2, split.displayName);
        splitList_->SetItemData(index, long(i));
        splitList_->SetItemTextColour(index, split.seg ? wxColour(96, 96, 96) : splitList_->GetTextColour());
    }
    splitList_->SetColumnWidth(2, wxLIST_AUTOSIZE_USEHEADER);
}

void MainWnd::moveUpButtonClicked() {
    auto index = segList_->GetFirstSelected();
    if (index <= 0) return;
    lss_.moveSegmentDown(index - 1);
    auto &segs = lss_.segs();
    segList_->SetItem(index, 0, wxString::FromUTF8(segs[index].seg.child("Name").text().get()));
    segList_->SetItem(index, 1, segs[index].splitName);
    segList_->SetItem(index - 1, 0, wxString::FromUTF8(segs[index - 1].seg.child("Name").text().get()));
    segList_->SetItem(index - 1, 1, segs[index - 1].splitName);
    segList_->Select(index - 1);
    segList_->EnsureVisible(index - 1);
}

void MainWnd::moveDownButtonClicked() {
    auto index = segList_->GetFirstSelected();
    if (index < 0) return;
    auto &segs = lss_.segs();
    if (index >= segs.size() - 1) return;
    lss_.moveSegmentDown(index);
    segList_->SetItem(index, 0, wxString::FromUTF8(segs[index].seg.child("Name").text().get()));
    segList_->SetItem(index, 1, segs[index].splitName);
    segList_->SetItem(index + 1, 0, wxString::FromUTF8(segs[index + 1].seg.child("Name").text().get()));
    segList_->SetItem(index + 1, 1, segs[index + 1].splitName);
    segList_->Select(index + 1);
    segList_->EnsureVisible(index + 1);
}

void MainWnd::assignButtonClicked() {
    auto toIndex = segList_->GetFirstSelected();
    if (toIndex == -1) return;
    auto fromIndex = splitList_->GetFirstSelected();
    if (fromIndex == -1) return;
    auto sindex = splitList_->GetItemData(fromIndex);
    const auto &segs = lss_.segs();
    auto &segData = segs[toIndex];
    const auto &splits = lss_.splits();
    auto &splitData = splits[sindex];
    if (segData.split == splitData.split) return;
    doUnsign(toIndex, segData);
    doUnsign(fromIndex, splitData);

    assignSplitToSegment(toIndex, fromIndex, segData, splitData);
}

void MainWnd::unsignButtonClicked() {
    auto toIndex = segList_->GetFirstSelected();
    if (toIndex == -1) return;
    doUnsign(toIndex, lss_.segs()[toIndex]);
}

void MainWnd::editSegmentButtonClicked(bool newSeg, bool insertBelow) {
    long toIndex;
    const SegNode *segData = nullptr;
    if (newSeg) {
        toIndex = segList_->GetFirstSelected();
        if (toIndex == -1) {
            return;
        }
        if (insertBelow) toIndex++;
    } else {
        toIndex = segList_->GetFirstSelected();
        if (toIndex == -1) return;
        segData = &lss_.segs()[toIndex];
    }

    static EditSegmentDlg *dlg = nullptr;
    if (dlg == nullptr)
        dlg = new EditSegmentDlg(this);
    std::string oldName = newSeg ? "" : segData->seg.child("Name").text().get();
    dlg->setSegmentName(oldName);
    if (!newSeg && segData->split) {
        auto &splits = lss_.splits();
        for (const auto &split: splits) {
            if (split.split == segData->split) {
                dlg->setValue(split.when, split.type, split.identifier);
                break;
            }
        }
    } else {
        dlg->setDefaultFilter(newSeg ? L"" : segList_->GetItemText(toIndex).ToStdWstring());
    }
    if (dlg->ShowModal() != wxID_OK) return;
    std::string segmentName, when, type, identifier;
    dlg->getResult(segmentName, when, type, identifier);
    if (segmentName.empty() || identifier.empty()) {
        wxMessageBox(wxT("Name cannot be empty"), wxT("Error"), wxICON_ERROR);
        return;
    }
    bool wasAppend;
    const auto *snode = lss_.findOrAppendSplit(when, type, identifier, wasAppend);
    if (snode == nullptr) {
        wxMessageBox(wxT("Wrong Split data!"), wxT("Error"), wxICON_ERROR);
        return;
    }
    if (newSeg) {
        segData = &lss_.insertNewSegment(segmentName, toIndex);
        segList_->InsertItem(toIndex, wxString::FromUTF8(segmentName));
    } else {
        if (segmentName != oldName) {
            segData->seg.child("Name").text().set(segmentName.c_str());
            segList_->SetItem(toIndex, 0, wxString::FromUTF8(segmentName));
            lss_.setChanged();
        }
    }
    if (wasAppend) {
        auto sindex = splitList_->InsertItem(splitList_->GetItemCount(), wxString::FromUTF8(snode->when));
        splitList_->SetItem(sindex, 1, wxString::FromUTF8(snode->type));
        splitList_->SetItem(sindex, 2, snode->displayName);
        splitList_->SetItemData(sindex, long(lss_.splits().size() - 1));
        assignSplitToSegment(toIndex, sindex, *segData, *snode);
    } else if (segData->split != snode->split) {
        const auto &splits = lss_.splits();
        auto sz = splitList_->GetItemCount();
        for (int i = 0; i < sz; i++) {
            const auto &splitNode = splits[splitList_->GetItemData(i)];
            if (splitNode.split == snode->split) {
                doUnsign(i, splitNode);
                doUnsign(toIndex, *segData);
                assignSplitToSegment(toIndex, i, *segData, *snode);
                break;
            }
        }
    }
}

void MainWnd::deleteSegmentButtonClicked() {
    auto index = segList_->GetFirstSelected();
    if (index < 0) return;
    auto &seg = lss_.segs()[index];
    doUnsign(index, seg);
    segList_->DeleteItem(index);
    lss_.deleteSegment(index);
}

void MainWnd::deleteSplitButtonClicked() {
    auto index = splitList_->GetFirstSelected();
    if (index < 0) return;
    auto sindex = splitList_->GetItemData(index);
    auto &split = lss_.splits()[sindex];
    doUnsign(index, split);
    splitList_->DeleteItem(index);
    lss_.deleteSplit(index);
}

void MainWnd::doUnsign(int index, const SegNode &seg) {
    if (!seg.split) return;
    const auto &splits = lss_.splits();
    auto cnt = splitList_->GetItemCount();
    for (long i = 0; i < cnt; i++) {
        auto data = splitList_->GetItemData(i);
        const auto &split = splits[data];
        if (split.split == seg.split) {
            split.assign(pugi::xml_node());
            splitList_->SetItemTextColour(i, splitList_->GetTextColour());
            break;
        }
    }
    seg.assign(pugi::xml_node(), L"");
    segList_->SetItem(index, 1, wxEmptyString);
}

void MainWnd::doUnsign(int index, const SplitNode &split) {
    if (!split.seg) return;
    auto sz = segList_->GetItemCount();
    const auto &segs = lss_.segs();
    for (int i = 0; i < sz; i++) {
        const auto &seg = segs[i];
        if (seg.seg == split.seg) {
            seg.assign(pugi::xml_node(), L"");
            segList_->SetItem(i, 1, wxEmptyString);
            break;
        }
    }
    split.assign(pugi::xml_node());
    splitList_->SetItemTextColour(index, splitList_->GetTextColour());
}

void MainWnd::assignSplitToSegment(int toIndex, int fromIndex, const SegNode &seg, const SplitNode &split) {
    seg.assign(split.split, split.fullDisplayName);
    split.assign(seg.seg);
    segList_->SetItem(toIndex, 1, split.fullDisplayName);
    splitList_->SetItemTextColour(fromIndex, wxColour(96, 96, 96));
    const auto &segs = lss_.segs();
    auto segCnt = segList_->GetItemCount();
    while (++toIndex < segCnt) {
        if (segs[toIndex].split.empty()) {
            segList_->Select(toIndex);
            segList_->EnsureVisible(toIndex);
            break;
        }
    }
}

}
