/*
 * Copyright (c) 2024 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "editsegmentdlg.h"

#include "util.h"

#include <wx/valnum.h>
#include <rapidfuzz/fuzz.hpp>
#include <fmt/format.h>

namespace lss_helper {

EditSegmentDlg::EditSegmentDlg(wxWindow *parent) : wxDialog(parent, wxID_ANY, wxT("New Split"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE) {
    auto *sizer = new wxBoxSizer(wxVERTICAL);

    wxArrayString whenChoices = {"Immediate", "OnLoading", "OnBlackscreen"};
    wxArrayString typeChoices = {"Boss", "Grace", "ItemPickup", "Flag", "Position"};
    segmentName_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize);
    whenChoice_ = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, whenChoices);
    typeChoice_ = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, typeChoices);
    splitFilter_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize);
    splitDataList_ = new wxListBox(this, wxID_ANY, wxPoint(0, 0), wxDefaultSize, 0, nullptr, wxLB_SINGLE);
    flagId_ = new wxTextCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize);
    positionPanel_ = new wxPanel(this, wxID_ANY);
    positionArea_ = new wxTextCtrl(positionPanel_, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize);
    positionBlock_ = new wxTextCtrl(positionPanel_, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize);
    positionRegion_ = new wxTextCtrl(positionPanel_, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize);
    positionSize_ = new wxTextCtrl(positionPanel_, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize);
    positionX_ = new wxTextCtrl(positionPanel_, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize);
    positionY_ = new wxTextCtrl(positionPanel_, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize);
    positionZ_ = new wxTextCtrl(positionPanel_, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize);

    segmentName_->SetHint("Segment Name");
    whenChoice_->SetSelection(0);
    typeChoice_->SetSelection(0);
    splitFilter_->SetHint("Filter");
    flagId_->SetHint("Flag ID");
    wxIntegerValidator<uint32_t> intValidator;
    flagId_->SetValidator(intValidator);
    flagId_->Hide();
    positionArea_->SetHint("Area");
    positionArea_->SetValidator(intValidator);
    positionBlock_->SetHint("Block");
    positionBlock_->SetValidator(intValidator);
    positionRegion_->SetHint("Region");
    positionRegion_->SetValidator(intValidator);
    positionSize_->SetHint("Size");
    positionSize_->SetValidator(intValidator);
    wxFloatingPointValidator<float> posValidator(6, nullptr,wxNUM_VAL_NO_TRAILING_ZEROES);
    positionX_->SetHint("X");
    positionX_->SetValidator(posValidator);
    positionY_->SetHint("Y");
    positionY_->SetValidator(posValidator);
    positionZ_->SetHint("Z");
    positionZ_->SetValidator(posValidator);

    auto *positionSizer = new wxFlexGridSizer(6, 0, 0);
    positionPanel_->SetSizer(positionSizer);
    positionSizer->AddGrowableCol(1, 1);
    positionSizer->AddGrowableCol(3, 1);
    positionSizer->AddGrowableCol(5, 1);
    positionSizer->Add(new wxStaticText(positionPanel_, wxID_ANY, wxT("Area:")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    positionSizer->Add(positionArea_, 0, wxEXPAND | wxALL, 5);
    positionSizer->Add(new wxStaticText(positionPanel_, wxID_ANY, wxT("Block:")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    positionSizer->Add(positionBlock_, 0, wxEXPAND | wxALL, 5);
    positionSizer->Add(new wxStaticText(positionPanel_, wxID_ANY, wxT("Region:")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    positionSizer->Add(positionRegion_, 0, wxEXPAND | wxALL, 5);
    positionSizer->Add(new wxStaticText(positionPanel_, wxID_ANY, wxT("Size:")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    positionSizer->Add(positionSize_, 0, wxEXPAND | wxALL, 5);
    positionSizer->AddSpacer(0);
    positionSizer->AddSpacer(0);
    positionSizer->AddSpacer(0);
    positionSizer->AddSpacer(0);
    positionSizer->Add(new wxStaticText(positionPanel_, wxID_ANY, wxT("X:")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    positionSizer->Add(positionX_, 0, wxEXPAND | wxALL, 5);
    positionSizer->Add(new wxStaticText(positionPanel_, wxID_ANY, wxT("Y:")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    positionSizer->Add(positionY_, 0, wxEXPAND | wxALL, 5);
    positionSizer->Add(new wxStaticText(positionPanel_, wxID_ANY, wxT("Z:")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    positionSizer->Add(positionZ_, 0, wxEXPAND | wxALL, 5);
    positionPanel_->Hide();

    auto *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    okButton_ = new wxButton(this, wxID_OK, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0);
    cancelButton_ = new wxButton(this, wxID_CANCEL, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0);
    okButton_->Enable(false);
    buttonSizer->Add(okButton_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    buttonSizer->Add(cancelButton_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    auto *whenTypeSizer = new wxBoxSizer(wxHORIZONTAL);
    whenTypeSizer->Add(new wxStaticText(this, wxID_ANY, wxT("When:")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    whenTypeSizer->Add(whenChoice_, 0, wxEXPAND | wxALL, 5);
    whenTypeSizer->Add(new wxStaticText(this, wxID_ANY, wxT("Type:")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    whenTypeSizer->Add(typeChoice_, 0, wxEXPAND | wxALL, 5);
    sizer->Add(segmentName_, 0, wxEXPAND | wxALL, 5);
    sizer->Add(whenTypeSizer, 0, wxEXPAND | wxALL, 5);
    sizer->Add(splitFilter_, 0, wxEXPAND | wxALL, 5);
    sizer->Add(splitDataList_, 1, wxEXPAND | wxALL, 5);
    sizer->Add(flagId_, 0, wxEXPAND | wxALL, 5);
    sizer->Add(positionPanel_, 0, wxEXPAND | wxALL, 5);
    sizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 5);

    wxDialog::SetSizer(sizer);
    wxDialog::SetMinClientSize(wxSize(500, 600));
    wxDialog::Fit();
    wxDialog::Layout();

    typeChoice_->Bind(wxEVT_CHOICE, [&](wxCommandEvent &event) {
        pendForUpdate();
        fitOnTypeChanged();
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

    segmentName_->Bind(wxEVT_TEXT, [&](wxCommandEvent &event) {
        okButton_->Enable(!segmentName_->GetValue().IsEmpty());
    });

    updateTimer_ = new wxTimer(this);
    Bind(wxEVT_TIMER, [&](wxTimerEvent &event) {
        updateSplitList();
    });
}

EditSegmentDlg::~EditSegmentDlg() = default;

void EditSegmentDlg::getResult(std::string &segmentName, std::string &when, std::string &type, std::string &identifier) const {
    segmentName = segmentName_->GetValue().utf8_string();
    when = whenChoice_->GetStringSelection().utf8_string();
    type = typeChoice_->GetStringSelection().utf8_string();
    switch (typeChoice_->GetSelection()) {
        case 3:
            identifier = fmt::format("{}", flagId_->GetValue().utf8_string());
            break;
        case 4:
            identifier = fmt::format("{},{},{},{},{},{},{}", positionArea_->GetValue().utf8_string(),
                                     positionBlock_->GetValue().utf8_string(), positionRegion_->GetValue().utf8_string(),
                                     positionSize_->GetValue().utf8_string(), positionX_->GetValue().utf8_string(),
                                     positionY_->GetValue().utf8_string(), positionZ_->GetValue().utf8_string());
            break;
        default: {
            auto sel = splitDataList_->GetSelection();
            identifier = sel >= 0 ? filterList_[sel].first->name : "";
            break;
        }
    }
}

void EditSegmentDlg::setSegmentName(const std::string &name) {
    segmentName_->SetValue(wxString::FromUTF8(name));
}

void EditSegmentDlg::setDefaultFilter(const std::wstring &filer) {
    splitFilter_->SetValue(filer);
}

void EditSegmentDlg::setValue(const std::string &when, const std::string &type, const std::string &value) {
    whenChoice_->SetStringSelection(wxString::FromUTF8(when));
    typeChoice_->SetStringSelection(wxString::FromUTF8(type));
    fitOnTypeChanged();
    if (type == "Flag") {
        flagId_->SetValue(wxString::FromUTF8(value));
    } else if (type == "Position") {
        auto res = splitString(value, ',');
        if (res.size() < 7) return;
        positionArea_->SetValue(wxString::FromUTF8(res[0]));
        positionBlock_->SetValue(wxString::FromUTF8(res[1]));
        positionRegion_->SetValue(wxString::FromUTF8(res[2]));
        positionSize_->SetValue(wxString::FromUTF8(res[3]));
        positionX_->SetValue(wxString::FromUTF8(res[4]));
        positionY_->SetValue(wxString::FromUTF8(res[5]));
        positionZ_->SetValue(wxString::FromUTF8(res[6]));
    } else {
        splitFilter_->SetValue(wxEmptyString);
        updateTimer_->Stop();
        updateSplitList();
        auto &list = *enumsList_;
        auto it = std::find_if(list.begin(), list.end(), [&](const auto &item) {
            return item.name == value;
        });
        if (it != list.end()) {
            auto idx = int(std::distance(list.begin(), it));
            splitDataList_->SetSelection(idx);
            splitDataList_->EnsureVisible(idx);
        }
    }
}

void EditSegmentDlg::updateSplitList() {
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

void EditSegmentDlg::pendForUpdate() {
    updateTimer_->Start(300, true);
}

void EditSegmentDlg::fitOnTypeChanged() {
    switch (typeChoice_->GetSelection()) {
        case 3:
            splitFilter_->Hide();
            splitDataList_->Hide();
            positionPanel_->Hide();
            flagId_->Show();
            wxDialog::SetMinClientSize(wxSize(500, 0));
            break;
        case 4:
            splitFilter_->Hide();
            splitDataList_->Hide();
            flagId_->Hide();
            positionPanel_->Show();
            wxDialog::SetMinClientSize(wxSize(500, 0));
            break;
        default:
            flagId_->Hide();
            positionPanel_->Hide();
            splitFilter_->Show();
            splitDataList_->Show();
            wxDialog::SetMinClientSize(wxSize(500, 600));
            break;
    }
    wxDialog::Layout();
    wxDialog::Fit();
}

}
