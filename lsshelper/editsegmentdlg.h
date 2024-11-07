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

#pragma once

#include "enums.h"

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/combo.h>

namespace lss_helper {

class EditSegmentDlg : public wxDialog {
public:
    explicit EditSegmentDlg(wxWindow *parent);
    ~EditSegmentDlg() override;
    void prepare(bool isNewSplit, const std::string &gameName);
    void getResult(std::string &segmentName, std::string &when, std::string &type, std::string &identifier) const;
    void setSegmentName(const std::string &name);
    void setDefaultFilter(const std::wstring &filer);
    void setValue(const std::string &when, const std::string &type, const std::string &value);

private:
    void updateSplitList();
    void pendForUpdate();
    void fitOnTypeChanged();

private:
    bool isER_ = false;

    wxTextCtrl *segmentName_;
    wxChoice *whenChoice_;
    wxChoice *typeChoice_;
    wxTextCtrl *splitFilter_;
    wxListBox *splitDataList_;

    wxTextCtrl *flagId_;
    wxPanel *positionPanel_;
    wxStaticText *areaText_;
    wxStaticText *blockText_;
    wxStaticText *regionText_;
    wxTextCtrl *positionDescription_;
    wxTextCtrl *positionArea_;
    wxTextCtrl *positionBlock_;
    wxTextCtrl *positionRegion_;
    wxTextCtrl *positionSize_;
    wxTextCtrl *positionX_;
    wxTextCtrl *positionY_;
    wxTextCtrl *positionZ_;

    wxButton *okButton_;
    wxButton *cancelButton_;

    wxTimer *updateTimer_;

    const std::vector<EnumData> *enumsList_;
    std::vector<std::pair<const EnumData*, double>> filterList_;
    std::vector<std::string> whenStrings_, typeStrings_;
};

}
