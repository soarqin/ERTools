/*
 * LSS Helper - LiveSplit lss file helper
 * Copyright (C) 2024  Soar Qin<soarchin@gmail.com>

 * This file is part of LSS Helper.

 * LSS Helper is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.

 * LSS Helper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with LSS Helper.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "editsegmentdlg.h"

#include "util.h"

#include <wx/valnum.h>
#include <rapidfuzz/fuzz.hpp>
#include <fmt/format.h>

namespace lss_helper {

EditSegmentDlg::EditSegmentDlg(wxWindow *parent) : wxDialog(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE) {
    auto *sizer = new wxBoxSizer(wxVERTICAL);

    /* for gettext use */
#if 0
    _("Immediate");
    _("OnLoading");
    _("OnBlackscreen");
    _("Boss");
    _("Grace");
    _("ItemPickup");
    _("Flag");
    _("Position");
    _("Bonfire");
    _("Attribute");
#endif

    segmentName_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize);
    whenChoice_ = new wxChoice(this, wxID_ANY);
    typeChoice_ = new wxChoice(this, wxID_ANY);
    splitFilter_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize);
    splitDataList_ = new wxListBox(this, wxID_ANY, wxPoint(0, 0), wxDefaultSize, 0, nullptr, wxLB_SINGLE);
    flagSizer_ = new wxBoxSizer(wxHORIZONTAL);
    flagIdText_ = new wxStaticText(this, wxID_ANY, _("Flag ID:"));
    flagId_ = new wxTextCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize);
    flagDescriptionText_ = new wxStaticText(this, wxID_ANY, _("Description:"));
    flagDescription_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize);
    positionSizer_ = new wxGridBagSizer();
    positionDescription_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize);
    positionAreaText_ = new wxStaticText(this, wxID_ANY, _("Area:"));
    positionBlockText_ = new wxStaticText(this, wxID_ANY, _("Block:"));
    positionRegionText_ = new wxStaticText(this, wxID_ANY, _("Region:"));
    positionArea_ = new wxTextCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize);
    positionBlock_ = new wxTextCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize);
    positionRegion_ = new wxTextCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize);
    positionSize_ = new wxTextCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize);
    positionX_ = new wxTextCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize);
    positionY_ = new wxTextCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize);
    positionZ_ = new wxTextCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize);
    attributeSizer_ = new wxBoxSizer(wxHORIZONTAL);
    attributeType_ = new wxChoice(this, wxID_ANY);
    attributeLevel_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize);

    segmentName_->SetHint(_("Segment Name"));
    whenChoice_->SetSelection(0);
    typeChoice_->SetSelection(0);
    splitFilter_->SetHint(_("Filter"));

    flagSizer_->Add(flagIdText_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    flagSizer_->Add(flagId_, 1, wxEXPAND | wxALL, 5);
    flagSizer_->Add(flagDescriptionText_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    flagSizer_->Add(flagDescription_, 1, wxEXPAND | wxALL, 5);
    wxIntegerValidator<uint32_t> intValidator;
    flagId_->SetValidator(intValidator);
    flagId_->Hide();
    positionArea_->SetValidator(intValidator);
    positionBlock_->SetValidator(intValidator);
    positionRegion_->SetValidator(intValidator);
    positionSize_->SetValidator(intValidator);
    wxFloatingPointValidator<float> posValidator(6, nullptr,wxNUM_VAL_NO_TRAILING_ZEROES);
    positionX_->SetHint("X");
    positionX_->SetValidator(posValidator);
    positionY_->SetHint("Y");
    positionY_->SetValidator(posValidator);
    positionZ_->SetHint("Z");
    positionZ_->SetValidator(posValidator);

    positionSizer_->SetCols(6);
    positionSizer_->SetRows(4);
    positionSizer_->AddGrowableCol(1, 1);
    positionSizer_->AddGrowableCol(3, 1);
    positionSizer_->AddGrowableCol(5, 1);
    positionSizer_->Add(positionAreaText_, wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    positionSizer_->Add(positionArea_, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND | wxALL, 5);
    positionSizer_->Add(positionBlockText_, wxGBPosition(0, 2), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    positionSizer_->Add(positionBlock_, wxGBPosition(0, 3), wxDefaultSpan, wxEXPAND | wxALL, 5);
    positionSizer_->Add(positionRegionText_, wxGBPosition(0, 4), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    positionSizer_->Add(positionRegion_, wxGBPosition(0, 5), wxDefaultSpan, wxEXPAND | wxALL, 5);
    positionSizer_->Add(new wxStaticText(this, wxID_ANY, _("Size:")), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    positionSizer_->Add(positionSize_, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND | wxALL, 5);
    positionSizer_->Add(new wxStaticText(this, wxID_ANY, wxT("X:")), wxGBPosition(2, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    positionSizer_->Add(positionX_, wxGBPosition(2, 1), wxDefaultSpan, wxEXPAND | wxALL, 5);
    positionSizer_->Add(new wxStaticText(this, wxID_ANY, wxT("Y:")), wxGBPosition(2, 2), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    positionSizer_->Add(positionY_, wxGBPosition(2, 3), wxDefaultSpan, wxEXPAND | wxALL, 5);
    positionSizer_->Add(new wxStaticText(this, wxID_ANY, wxT("Z:")), wxGBPosition(2, 4), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    positionSizer_->Add(positionZ_, wxGBPosition(2, 5), wxDefaultSpan, wxEXPAND | wxALL, 5);
    positionSizer_->Add(new wxStaticText(this, wxID_ANY, _("Description:")), wxGBPosition(3, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    positionSizer_->Add(positionDescription_, wxGBPosition(3, 1), wxGBSpan(1, 5), wxEXPAND | wxALL, 5);
    positionSizer_->Show(false);

    attributeSizer_->Add(new wxStaticText(this, wxID_ANY, _("Type:")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    attributeSizer_->Add(attributeType_, 1, wxEXPAND | wxALL, 5);
    attributeSizer_->Add(new wxStaticText(this, wxID_ANY, _("Level:")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    attributeSizer_->Add(attributeLevel_, 1, wxEXPAND | wxALL, 5);
    attributeSizer_->Show(false);

    auto *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    okButton_ = new wxButton(this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0);
    cancelButton_ = new wxButton(this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0);
    okButton_->Enable(false);
    buttonSizer->Add(okButton_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    buttonSizer->Add(cancelButton_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    auto *whenTypeSizer = new wxBoxSizer(wxHORIZONTAL);
    whenTypeSizer->Add(new wxStaticText(this, wxID_ANY, _("When:")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    whenTypeSizer->Add(whenChoice_, 0, wxEXPAND | wxALL, 5);
    whenTypeSizer->Add(new wxStaticText(this, wxID_ANY, _("Type:")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    whenTypeSizer->Add(typeChoice_, 0, wxEXPAND | wxALL, 5);
    sizer->Add(segmentName_, 0, wxEXPAND | wxALL, 5);
    sizer->Add(whenTypeSizer, 0, wxEXPAND | wxALL, 5);
    sizer->Add(splitFilter_, 0, wxEXPAND | wxALL, 5);
    sizer->Add(splitDataList_, 1, wxEXPAND | wxALL, 5);
    sizer->Add(flagSizer_, 0, wxEXPAND | wxALL, 5);
    sizer->Add(positionSizer_, 0, wxEXPAND | wxALL, 5);
    sizer->Add(attributeSizer_, 0, wxEXPAND | wxALL, 5);
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

void EditSegmentDlg::prepare(bool isNewSplit, const std::string &gameName) {
    isER_ = gameName == "EldenRing";
    this->SetTitle(isNewSplit ? _("New Segment") : _("Edit Segment"));
    if (isER_) {
        whenStrings_ = {"Immediate", "OnLoading", "OnBlackscreen"};
        typeStrings_ = {"Boss", "Grace", "ItemPickup", "Flag", "Position"};
        flagDescriptionText_->Show(false);
        flagDescription_->Show(false);
        positionAreaText_->Show();
        positionBlockText_->Show();
        positionRegionText_->Show();
        positionArea_->Show();
        positionBlock_->Show();
        positionRegion_->Show();
        positionDescription_->Show(false);
    } else {
        whenStrings_ = {"Immediate", "OnLoading"};
        typeStrings_ = {"Boss", "Bonfire", "ItemPickup", "Flag", "Position", "Attribute"};
        attrTypeStrings_ = {"Vigor", "Attunement", "Endurance", "Vitality", "Strength", "Dexterity", "Intelligence", "Faith", "Luck", "SoulLevel", "Humanity"};
        flagDescriptionText_->Show();
        flagDescription_->Show();
        positionAreaText_->Show(false);
        positionBlockText_->Show(false);
        positionRegionText_->Show(false);
        positionArea_->Show(false);
        positionBlock_->Show(false);
        positionRegion_->Show(false);
        positionDescription_->Show();
    }
    wxArrayString whenChoices;
    std::transform(whenStrings_.begin(), whenStrings_.end(), std::back_inserter(whenChoices), [](const std::string &str) { return _(str); });
    wxArrayString typeChoices;
    std::transform(typeStrings_.begin(), typeStrings_.end(), std::back_inserter(typeChoices), [](const std::string &str) { return _(str); });
    wxArrayString attrTypeChoices;
    std::transform(attrTypeStrings_.begin(), attrTypeStrings_.end(), std::back_inserter(attrTypeChoices), [](const std::string &str) { return _(str); });
    whenChoice_->Set(whenChoices);
    typeChoice_->Set(typeChoices);
    attributeType_->Set(attrTypeChoices);
}

void EditSegmentDlg::getResult(std::string &segmentName, std::string &when, std::string &type, std::string &identifier) const {
    segmentName = segmentName_->GetValue().utf8_string();
    when = whenStrings_[whenChoice_->GetSelection()];
    auto typeIdx = typeChoice_->GetSelection();
    type = typeStrings_[typeIdx];
    switch (typeIdx) {
        case 3:
            if (isER_) {
                identifier = fmt::format("{}", flagId_->GetValue().utf8_string());
            } else {
                identifier = fmt::format("{},{}", flagId_->GetValue().utf8_string(), flagDescription_->GetValue().utf8_string());
            }
            break;
        case 4:
            if (isER_) {
                identifier = fmt::format("{},{},{},{},{},{},{}", positionArea_->GetValue().utf8_string(),
                                         positionBlock_->GetValue().utf8_string(), positionRegion_->GetValue().utf8_string(),
                                         positionSize_->GetValue().utf8_string(), positionX_->GetValue().utf8_string(),
                                         positionY_->GetValue().utf8_string(), positionZ_->GetValue().utf8_string());
            } else {
                identifier = fmt::format("{},{},{},{},{}", positionDescription_->GetValue().utf8_string(),
                                         positionX_->GetValue().utf8_string(), positionY_->GetValue().utf8_string(),
                                         positionZ_->GetValue().utf8_string(), positionSize_->GetValue().utf8_string());
            }
            break;
        case 5:
            identifier = fmt::format("{},{}", attrTypeStrings_[attributeType_->GetSelection()], attributeLevel_->GetValue().utf8_string());
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
    whenChoice_->SetStringSelection(_(when.c_str()));
    typeChoice_->SetStringSelection(_(type.c_str()));
    fitOnTypeChanged();
    if (type == _("Flag")) {
        if (isER_) {
            flagId_->SetValue(wxString::FromUTF8(value));
        } else {
            auto res = splitString(value, ',');
            if (res.size() < 2) return;
            flagId_->SetValue(wxString::FromUTF8(res[0]));
            flagDescription_->SetValue(wxString::FromUTF8(res[1]));
        }
    } else if (type == _("Position")) {
        auto res = splitString(value, ',');
        if (isER_) {
            if (res.size() < 7) return;
            positionArea_->SetValue(wxString::FromUTF8(res[0]));
            positionBlock_->SetValue(wxString::FromUTF8(res[1]));
            positionRegion_->SetValue(wxString::FromUTF8(res[2]));
            positionSize_->SetValue(wxString::FromUTF8(res[3]));
            positionX_->SetValue(wxString::FromUTF8(res[4]));
            positionY_->SetValue(wxString::FromUTF8(res[5]));
            positionZ_->SetValue(wxString::FromUTF8(res[6]));
        } else {
            if (res.size() < 5) return;
            positionDescription_->SetValue(wxString::FromUTF8(res[0]));
            positionX_->SetValue(wxString::FromUTF8(res[1]));
            positionY_->SetValue(wxString::FromUTF8(res[2]));
            positionZ_->SetValue(wxString::FromUTF8(res[3]));
            positionSize_->SetValue(wxString::FromUTF8(res[4]));
        }
    } else if (type == _("Attribute")) {
        auto res = splitString(value, ',');
        if (res.size() < 2) return;
        auto ite = std::find(attrTypeStrings_.begin(), attrTypeStrings_.end(), res[0]);
        attributeType_->SetSelection(ite == attrTypeStrings_.end() ? 0 : int(std::distance(attrTypeStrings_.begin(), ite)));
        attributeLevel_->SetValue(wxString::FromUTF8(res[1]));
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
    enumsList_ = &gEnums[typeStrings_[typeChoice_->GetSelection()]];

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
            splitFilter_->Show(false);
            splitDataList_->Show(false);
            flagSizer_->Show(true);
            positionSizer_->Show(false);
            attributeSizer_->Show(false);
            wxDialog::SetMinClientSize(wxSize(500, 0));
            break;
        case 4:
            splitFilter_->Show(false);
            splitDataList_->Show(false);
            flagSizer_->Show(false);
            positionSizer_->Show(true);
            attributeSizer_->Show(false);
            wxDialog::SetMinClientSize(wxSize(500, 0));
            break;
        case 5:
            splitFilter_->Show(false);
            splitDataList_->Show(false);
            flagSizer_->Show(false);
            positionSizer_->Show(false);
            attributeSizer_->Show(true);
            wxDialog::SetMinClientSize(wxSize(500, 0));
            break;
        default:
            splitFilter_->Show(true);
            splitDataList_->Show(true);
            flagSizer_->Show(false);
            positionSizer_->Show(false);
            attributeSizer_->Show(false);
            wxDialog::SetMinClientSize(wxSize(500, 600));
            break;
    }
    wxDialog::Layout();
    wxDialog::Fit();
}

}
