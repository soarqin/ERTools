/*
 * Copyright (c) 2024 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
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
    void getResult(std::string &segmentName, std::string &when, std::string &type, std::string &identifier) const;
    void setSegmentName(const std::string &name);
    void setDefaultFilter(const std::wstring &filer);
    void setValue(const std::string &when, const std::string &type, const std::string &value);

private:
    void updateSplitList();
    void pendForUpdate();
    void fitOnTypeChanged();

private:
    wxTextCtrl *segmentName_;
    wxChoice *whenChoice_;
    wxChoice *typeChoice_;
    wxTextCtrl *splitFilter_;
    wxListBox *splitDataList_;

    wxTextCtrl *flagId_;
    wxPanel *positionPanel_;
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
};

}
