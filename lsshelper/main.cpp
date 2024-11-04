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

#include "mainwnd.h"
#include "enums.h"

namespace lss_helper {

class LssHelperApp : public wxApp {
public:
    bool OnInit() override {
#if !defined(NDEBUG)
        AllocConsole();
        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
#endif

        int lang = wxLocale::GetSystemLanguage();
        wxLocale::AddCatalogLookupPathPrefix(wxT("data"));
        locale = new wxLocale();
        locale->Init(lang);
        locale->AddCatalog(wxT("lsshelper"));
        if (!locale->IsOk()) {
            lang = wxLANGUAGE_ENGLISH_US;
            locale->Init(lang);
            locale->AddCatalog(wxT("lsshelper"));
        }
        Enums::setLanguagePrefix(wxLocale::GetLanguageCanonicalName(lang).ToStdWstring());
        MSWEnableDarkMode(wxApp::DarkMode_Auto);
        (new MainWnd)->Show(true);
        return true;
    }

    int OnExit() override {
        delete locale;
        return wxApp::OnExit();
    }

private:
    wxLocale *locale;
};

}

IMPLEMENT_APP(lss_helper::LssHelperApp)
