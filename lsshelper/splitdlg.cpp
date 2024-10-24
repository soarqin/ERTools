/*
 * Copyright (c) 2024 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "splitdlg.h"

#include <fmt/format.h>

namespace lss_helper {

class SplitList : public wxListBox, public wxComboPopup {
    bool Create(wxWindow *parent) override {
        return wxListBox::Create(parent, wxID_ANY, wxPoint(0, 0), wxDefaultSize, 0, nullptr,wxLB_SINGLE);
    }
    wxWindow *GetControl() override { return this; }
    [[nodiscard]] wxString GetStringValue() const override {
        return GetStringSelection();
    }
};

SplitDlg::SplitDlg(wxWindow *parent, int lastWhen, int lastType) : wxDialog(parent, wxID_ANY, wxT("New Split"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE) {
    auto *sizer = new wxBoxSizer(wxVERTICAL);

    wxArrayString whenChoices = {"Immediate", "OnLoading", "OnBlackscreen"};
    wxArrayString typeChoices = {"Boss", "Grace", "ItemPickup", "Flag"};
    whenChoice_ = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, whenChoices);
    typeChoice_ = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, typeChoices);
    splitList_ = new wxComboCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxCB_READONLY);
    whenChoice_->SetSelection(lastWhen);
    typeChoice_->SetSelection(lastType);
    auto *sl = new SplitList();
    splitListView_ = sl;
    splitList_->SetPopupControl(sl);
    splitList_->SetPopupMinWidth(500);

    auto *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    okButton_ = new wxButton(this, wxID_OK, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0);
    cancelButton_ = new wxButton(this, wxID_CANCEL, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0);
    buttonSizer->Add(okButton_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    buttonSizer->Add(cancelButton_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    sizer->Add(whenChoice_, 0, wxEXPAND | wxALL, 5);
    sizer->Add(typeChoice_, 0, wxEXPAND | wxALL, 5);
    sizer->Add(splitList_, 0, wxEXPAND | wxALL, 5);
    sizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 5);

    wxDialog::SetSizer(sizer);
    wxDialog::SetMinClientSize(wxSize(300, 0));
    wxDialog::Fit();
    wxDialog::Layout();

    enumsList_ = &gEnums[typeChoice_->GetStringSelection().ToStdString()];
    updateSplitList();
}

SplitDlg::~SplitDlg() = default;

void SplitDlg::updateSplitList() {
    splitListView_->Clear();
    for (const auto &item: *enumsList_) {
        splitListView_->Append(wxString::FromUTF8(item.disp));
    }
}

}
