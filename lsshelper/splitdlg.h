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

class SplitDlg : public wxDialog {
public:
    explicit SplitDlg(wxWindow *parent, int lastWhen = 0, int lastType = 0);
    ~SplitDlg() override;
    int ShowModal() override {
        pendForUpdate();
        return wxDialog::ShowModal();
    }
    void getResult(std::string &when, std::string &type, std::string &name) const;
    void setDefaultFilter(const std::wstring &filer);

private:
    void updateSplitList();
    void pendForUpdate();

private:
    wxChoice *whenChoice_;
    wxChoice *typeChoice_;
    wxTextCtrl *splitFilter_;
    wxListBox *splitDataList_;

    wxButton *okButton_;
    wxButton *cancelButton_;

    wxTimer *updateTimer_;

    const std::vector<EnumData> *enumsList_;
    std::vector<std::pair<const EnumData*, double>> filterList_;
};

}
