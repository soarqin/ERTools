#pragma once

#include <string>

std::wstring selectFile(const std::wstring &title, const std::wstring &defaultFolder, const std::wstring &filters, const std::wstring &defaultExt, bool folderOnly, bool openMode = true);
