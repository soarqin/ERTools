/*
 * Copyright (c) 2024 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "splitdlg.h"

#include <rapidfuzz/fuzz.hpp>
#include <fmt/format.h>

namespace lss_helper {

SplitDlg::SplitDlg(wxWindow *parent, int lastWhen, int lastType) : wxDialog(parent, wxID_ANY, wxT("New Split"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE) {
    auto *sizer = new wxBoxSizer(wxVERTICAL);

    wxArrayString whenChoices = {"Immediate", "OnLoading", "OnBlackscreen"};
    wxArrayString typeChoices = {"Boss", "Grace", "ItemPickup", "Flag"};
    whenChoice_ = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, whenChoices);
    typeChoice_ = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, typeChoices);
    splitFilter_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize);
    splitDataList_ = new wxListBox(this, wxID_ANY, wxPoint(0, 0), wxDefaultSize, 0, nullptr,wxLB_SINGLE);
    whenChoice_->SetSelection(lastWhen);
    typeChoice_->SetSelection(lastType);

    auto *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    okButton_ = new wxButton(this, wxID_OK, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0);
    cancelButton_ = new wxButton(this, wxID_CANCEL, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0);
    buttonSizer->Add(okButton_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    buttonSizer->Add(cancelButton_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    auto *whenTypeSizer = new wxBoxSizer(wxHORIZONTAL);
    whenTypeSizer->Add(whenChoice_, 0, wxEXPAND | wxALL, 5);
    whenTypeSizer->Add(typeChoice_, 0, wxEXPAND | wxALL, 5);
    sizer->Add(whenTypeSizer, 0, wxEXPAND | wxALL, 5);
    sizer->Add(splitFilter_, 0, wxEXPAND | wxALL, 5);
    sizer->Add(splitDataList_, 1, wxEXPAND | wxALL, 5);
    sizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 5);

    wxDialog::SetSizer(sizer);
    wxDialog::SetMinClientSize(wxSize(500, 600));
    wxDialog::Fit();
    wxDialog::Layout();

    typeChoice_->Bind(wxEVT_CHOICE, [&](wxCommandEvent &event) {
        pendForUpdate();
    });
    splitFilter_->Bind(wxEVT_TEXT, [&](wxCommandEvent &event) {
        pendForUpdate();
    });
    splitDataList_->Bind(wxEVT_LISTBOX, [&](wxCommandEvent &event) {
        okButton_->Enable(splitDataList_->GetSelection() >= 0);
    });
    splitDataList_->Bind(wxEVT_LISTBOX_DCLICK, [&](wxCommandEvent &event) {
        if (splitDataList_->GetSelection() >= 0)
            wxDialog::EndModal(wxID_OK);
    });

    updateTimer_ = new wxTimer(this);
    Bind(wxEVT_TIMER, [&](wxTimerEvent &event) {
        updateSplitList();
    });
}

SplitDlg::~SplitDlg() = default;

void SplitDlg::getResult(std::string &when, std::string &type, std::string &name) const {
    when = whenChoice_->GetStringSelection().utf8_string();
    type = typeChoice_->GetStringSelection().utf8_string();
    auto sel = splitDataList_->GetSelection();
    name = sel >= 0 ? filterList_[sel].first->name : "";
}

void SplitDlg::setDefaultFilter(const std::wstring &filer) {
    splitFilter_->SetValue(filer);
}

void SplitDlg::updateSplitList() {
    enumsList_ = &gEnums[typeChoice_->GetStringSelection().utf8_string()];

    filterList_.clear();
    splitDataList_->Clear();
    auto text = splitFilter_->GetValue();
    if (text.IsEmpty()) {
        for (const auto &item: *enumsList_) {
            splitDataList_->Append(item.wdisp);
            filterList_.emplace_back(&item, 100.0);
        }
        return;
    }
    rapidfuzz::fuzz::CachedPartialTokenSortRatio<wchar_t> fuzz(text.ToStdWstring());
    for (const auto &item: *enumsList_) {
        auto ratio = fuzz.similarity(item.wdisp);
        if (ratio < 75) continue;
        filterList_.emplace_back(&item, ratio);
    }
    std::stable_sort(filterList_.begin(), filterList_.end(), [](const auto &a, const auto &b) {
        return a.second > b.second;
    });
    for (const auto &item: filterList_) {
        splitDataList_->Append(item.first->wdisp);
    }
}

void SplitDlg::pendForUpdate() {
    updateTimer_->Start(300, true);
}

}
