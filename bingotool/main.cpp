#include "cell.h"
#include "scorewindow.h"
#include "randomtable.h"
#include "sync.h"
#include "common.h"
#include "config.h"

#define WIN32_LEAN_AND_MEAN
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <windows.h>
#include <commctrl.h>
#include <objbase.h>
#include <shellapi.h>
#include <gdiplus.h>
#if defined(_MSC_VER)
#undef max
#undef min
#endif
#undef WIN32_LEAN_AND_MEAN

#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

#if defined(_MSC_VER)
#pragma comment(linker,"\"/manifestdependency:type='win32' \
    name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
    processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

static int mainLoop();

int wmain(int argc, wchar_t *argv[]) {
    // Unused argc, argv
    (void)argc;
    (void)argv;

    INITCOMMONCONTROLSEX iccex = {sizeof(INITCOMMONCONTROLSEX), ICC_UPDOWN_CLASS | ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&iccex);

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    auto result = mainLoop();

    Gdiplus::GdiplusShutdown(gdiplusToken);

    CoUninitialize();

    return result;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    int argc;
    wchar_t **argv = CommandLineToArgvW(lpCmdLine, &argc);
    return wmain(argc, argv);
}

void sendJudgeSyncState() {
    uint64_t val[2] = {0, 0};
    size_t idx = 0;
    gCells.foreach([&val, &idx](Cell &cell, int x, int y) {
        val[idx / 16] |= (uint64_t)(uint32_t)cell.status << (4 * (idx % 16));
        idx++;
    });
    if (!gConfig.bingoBrawlersMode) {
        val[1] |= (uint64_t)gScoreWindows[0].score << 48;
        val[1] |= (uint64_t)gScoreWindows[1].score << 56;
    }
    syncSendData('S', std::to_string(val[0]) + ',' + std::to_string(val[1]));
}

void sendJudgeSyncData() {
    std::vector<std::string> n;
    gCells.foreach([&n](Cell &cell, int x, int y) {
        n.emplace_back(cell.text);
    });
    n.emplace_back(UnicodeToUtf8(gConfig.playerName[0]));
    n.emplace_back(UnicodeToUtf8(gConfig.playerName[1]));
    syncSendData('T', n);
    sendJudgeSyncState();
}

static void randomCells(const std::string &filename) {
    if (syncGetMode() == 0) return;
    RandomTable rt;
    if (filename.empty()) {
        const auto &lastFilename = RandomTable::lastFilename();
        if (lastFilename.empty()) return;
        rt.load(lastFilename);
    } else {
        rt.load(filename);
    }
    std::vector<std::string> result;
    rt.generate(result);
    std::ofstream ofs("data/squares.txt");
    for (auto &s: result) {
        ofs << s << std::endl;
    }
    ofs.close();
    size_t idx = 0;
    gCells.foreach([&result, &idx](Cell &cell, int x, int y) {
        cell.status = 0;
        cell.text = result[idx++];
    });
    gCells.updateTextures();
    gScoreWindows[0].reset();
    gScoreWindows[1].reset();
    sendJudgeSyncData();
}

static void saveState() {
    std::ofstream ofs("state.txt");
    std::vector<std::string> n;
    for (auto &w: gScoreWindows) {
        n.emplace_back(std::to_string(w.cleared ? 1 : 0));
        n.emplace_back(std::to_string(w.clearCount));
    }
    gCells.foreach([&n](Cell &cell, int x, int y) {
        n.emplace_back(std::to_string(cell.status));
    });
    int x, y;
    SDL_GetWindowPosition(gCells.window(), &x, &y);
    n.emplace_back(std::to_string(x));
    n.emplace_back(std::to_string(y));
    for (int i = 0; i < 2; i++) {
        for (auto &win: gScoreWindows) {
            win.updateXYValue();
            n.emplace_back(std::to_string(win.tx[i]));
            n.emplace_back(std::to_string(win.ty[i]));
        }
    }
    ofs << mergeString(n, ',') << std::endl;
}

static void loadState() {
    std::ifstream ifs("state.txt");
    if (!ifs.is_open()) return;
    std::string line;
    std::getline(ifs, line);
    auto sl = splitString(line, ',');
    if (sl.size() < 35) return;
    int i = 0;
    for (auto &w: gScoreWindows) {
        w.cleared = sl[i++] == "1";
        w.clearCount = std::stoi(sl[i++]);
    }
    gCells.foreach([&sl, &i](Cell &cell, int x, int y) {
        cell.status = std::stoi(sl[i++]);
    });
    int x = std::stoi(sl[i++]);
    int y = std::stoi(sl[i++]);
    SDL_SetWindowPosition(gCells.window(), x, y);
    for (int j = 0; j < 2; j++) {
        for (auto &win: gScoreWindows) {
            x = std::stoi(sl[i++]);
            y = std::stoi(sl[i++]);
            win.tx[j] = x;
            win.ty[j] = y;
            if (!gConfig.simpleMode)
                SDL_SetWindowPosition(win.window[j], x, y);
        }
    }
}

static void loadSquares() {
    std::ifstream ifs("data/squares.txt");
    std::string line;
    gCells.foreach([&ifs, &line](Cell &cell, int x, int y) {
        std::getline(ifs, line);
        cell.text = line;
    });
}

void startSync() {
    syncOpen([]{
        syncSetChannel([](char t, const std::string &s) {
            switch (t) {
                case 'T': {
                    if (syncGetMode() != 0) break;
                    auto sl = splitString(s, '\n');
                    size_t idx = 0;
                    bool dirty = false;
                    for (int i = 0; i < 5; i++) {
                        auto *row = gCells[i];
                        for (int j = 0; j < 5; j++) {
                            auto &cell = row[j];
                            if (idx >= sl.size()) break;
                            const auto &text = sl[idx++];
                            if (cell.text == text) continue;
                            cell.text = text;
                            cell.updateTexture();
                            dirty = true;
                        }
                    }
                    for (int i = 0; i < 2; i++) {
                        if (idx < sl.size()) {
                            auto pname = sl[idx++];
                            gConfig.playerName[i] = Utf8ToUnicode(pname);
                            if (gScoreWindows[i].playerName != pname) {
                                gScoreWindows[i].playerName = pname;
                                gScoreWindows[i].updateTexture();
                            }
                        }
                    }
                    if (dirty) {
                        std::ofstream ofs("data/squares.txt");
                        ofs << s << std::endl;
                        ofs.close();
                    }
                    break;
                }
                case 'S': {
                    if (syncGetMode() != 0) break;
                    auto sl = splitString(s, ',');
                    uint64_t val[2];
                    size_t idx = 0;
                    for (const auto &ss: sl) {
                        val[idx++] = std::stoull(ss);
                    }
                    idx = 0;
                    bool dirty = false;
                    for (int i = 0; i < 5; i++) {
                        auto *row = gCells[i];
                        for (int j = 0; j < 5; j++) {
                            auto &cell = row[j];
                            auto state = (int)((val[idx / 16] >> (4 * (idx % 16))) & 0x0FULL);
                            idx++;
                            if (cell.status == state) continue;
                            cell.status = state;
                            cell.updateTexture();
                            dirty = true;
                        }
                    }
                    if (!gConfig.bingoBrawlersMode) {
                        int es[2] = {
                            (int)(uint32_t)((val[1] >> 48) & 0xFFULL),
                            (int)(uint32_t)((val[1] >> 56) & 0xFFULL),
                        };
                        for (int i = 0; i < 2; i++) {
                            if (es[i] != gScoreWindows[i].clearCount) {
                                gScoreWindows[i].cleared = es[i];
                                gScoreWindows[i].clearCount = es[i] != 0;
                                dirty = true;
                            }
                        }
                    }
                    if (dirty) {
                        updateScores();
                        saveState();
                    }
                    break;
                }
                default:
                    break;
            }
        });
        if (syncGetMode() != 0) {
            sendJudgeSyncData();
        } else {
            syncSendData('T', "");
            syncSendData('S', "");
        }
    });
}

void reloadAll() {
    gConfig.load();
    syncClose();

    gConfig.postLoad();
    loadSquares();
    gCells.init();
    gScoreWindows[0].create(0, UnicodeToUtf8(gConfig.playerName[0]));
    gScoreWindows[1].create(1, UnicodeToUtf8(gConfig.playerName[1]));
    loadState();
    updateScores();

    startSync();
}

static int mainLoop() {
    syncLoadConfig();
    const TASKDIALOG_BUTTON buttons[] = {
        {IDYES, L"裁判模式"},
        {IDNO, L"解说模式"},
        {IDRETRY, L"选手模式"},
    };
    TASKDIALOGCONFIG taskDialogConfig = {sizeof(TASKDIALOGCONFIG)};
    taskDialogConfig.hInstance = GetModuleHandleW(nullptr);
    taskDialogConfig.pszWindowTitle = L"运行模式";
    taskDialogConfig.pszMainIcon = MAKEINTRESOURCEW(1);
    taskDialogConfig.pszContent = L"请选择运行模式：\n裁判和解说模式为多窗口，选手模式为单窗口。\n裁判模式可操作表格，其他模式只能查看和被动同步。";
    taskDialogConfig.cButtons = ARRAYSIZE(buttons);
    taskDialogConfig.pButtons = buttons;
    int sel;
    if (TaskDialogIndirect(&taskDialogConfig, &sel, nullptr, nullptr) != S_OK
        || sel == IDCANCEL) return -1;
    gConfig.simpleMode = sel == IDRETRY;
    syncSetMode(sel == IDYES ? 1 : 0);
    if (sel != IDYES) {
        DialogBoxW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(130), nullptr, [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> INT_PTR {
            switch (msg) {
                case WM_INITDIALOG: {
                    // Center on desktop
                    RECT rc;
                    GetWindowRect(GetDesktopWindow(), &rc);
                    int x = (rc.right - rc.left) / 2;
                    int y = (rc.bottom - rc.top) / 2;
                    GetWindowRect(hwnd, &rc);
                    SetWindowPos(hwnd, nullptr, x - (rc.right - rc.left) / 2, y - (rc.bottom - rc.top) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

                    SetWindowTextW(GetDlgItem(hwnd, 1001), Utf8ToUnicode(syncGetChannelPassword()).c_str());
                    return TRUE;
                }
                case WM_COMMAND: {
                    switch (LOWORD(wParam)) {
                        case IDOK: {
                            HWND hwndEdit = GetDlgItem(hwnd, 1001);
                            wchar_t text[256];
                            GetWindowTextW(hwndEdit, text, 256);
                            syncSetChannelPassword(UnicodeToUtf8(text));
                            EndDialog(hwnd, IDOK);
                            return TRUE;
                        }
                        case IDCANCEL: {
                            EndDialog(hwnd, IDCANCEL);
                            return TRUE;
                        }
                    }
                    break;
                }
            }
            return FALSE;
        });
    }

    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) < 0) {
        printf("SDL could not be initialized!\n"
               "SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    TTF_Init();
    reloadAll();

    while (true) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_EVENT_QUIT:
                    goto QUIT;
                case SDL_EVENT_KEY_UP:
                    if (e.key.keysym.sym == SDLK_r && (e.key.keysym.mod & SDL_KMOD_CTRL) != 0) {
                        randomCells("");
                    }
                    break;
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
                    if (e.window.windowID == SDL_GetWindowID(gCells.window())) goto QUIT;
                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_UP: {
                    if (e.button.windowID == SDL_GetWindowID(gCells.window()) && e.button.button != SDL_BUTTON_RIGHT) break;
                    if (syncGetMode() == 0) {
                        auto menu = CreatePopupMenu();
                        AppendMenuW(menu, MF_STRING | ((SDL_GetWindowFlags(gCells.window()) & SDL_WINDOW_ALWAYS_ON_TOP) ? MF_CHECKED : 0), 9, L"窗口置顶");
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING, 7, L"设置");
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING, 8, L"退出");
                        POINT pt;
                        GetCursorPos(&pt);
                        auto hwnd = (HWND)SDL_GetProperty(SDL_GetWindowProperties(gCells.window()), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
                        auto cmd = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, nullptr);
                        switch (cmd) {
                            case 7:
                                gCells.showSettingsWindow();
                                break;
                            case 8:
                                goto QUIT;
                            case 9:
                                if (SDL_GetWindowFlags(gCells.window()) & SDL_WINDOW_ALWAYS_ON_TOP) {
                                    SDL_SetWindowAlwaysOnTop(gCells.window(), SDL_FALSE);
                                } else {
                                    SDL_SetWindowAlwaysOnTop(gCells.window(), SDL_TRUE);
                                }
                                break;
                            default:
                                break;
                        }
                        break;
                    }
                    float fx, fy;
                    SDL_GetMouseState(&fx, &fy);
                    int x = (int)fx, y = (int)fy;
                    int dx = (x - gConfig.cellBorder) / (gConfig.cellSize[0] + gConfig.cellSpacing);
                    int dy = (y - gConfig.cellBorder) / (gConfig.cellSize[1] + gConfig.cellSpacing);
                    if (dx >= gConfig.cellSize[0] || dy >= gConfig.cellSize[1]) break;
                    auto &cell = gCells[dy][dx];
                    auto menu = CreatePopupMenu();
                    int count[2] = {0, 0};
                    for (int j = 0; j < 5; ++j) {
                        switch (gCells[dy][j].status) {
                            case 1:
                                count[0]++;
                                break;
                            case 2:
                                count[1]++;
                                break;
                            case 3:
                            case 4:
                                count[0]++;
                                count[1]++;
                                break;
                        }
                    }
                    HMENU syncMenu = nullptr;
                    const auto &password = syncGetChannelPasswordForC();
                    if (!password.empty()) {
                        syncMenu = CreatePopupMenu();
                        AppendMenuW(syncMenu, MF_STRING | MF_DISABLED, 0, Utf8ToUnicode(password).c_str());
                        AppendMenuW(syncMenu, MF_STRING, 10, L"复制到剪贴板");
                        AppendMenuW(syncMenu, MF_STRING, 11, L"重新生成同步码");
                    }
                    if (gConfig.bingoBrawlersMode) {
                        auto tables = CreatePopupMenu();
                        AppendMenuW(tables, MF_STRING | (RandomTable::lastFilename().empty() ?  MF_DISABLED : 0), 99, L"使用上次的随机表格\tCtrl+R");
                        if (std::filesystem::exists("data/randomtable.txt")) {
                            AppendMenuW(tables, MF_STRING, 6, L"data/randomtable.txt");
                        }
                        std::vector<std::string> tableFilenames;
                        if (std::filesystem::exists("randomtables")) {
                            UINT_PTR idx = 100;
                            std::filesystem::directory_iterator it("randomtables");
                            for (const auto &entry: it) {
                                if (!entry.is_regular_file()) continue;
                                const auto &path = entry.path();
                                if (path.extension() != ".txt") continue;
                                AppendMenuW(tables, MF_STRING, idx++, path.stem().c_str());
                                tableFilenames.emplace_back(path.filename().string());
                            }
                        }
                        AppendMenuW(menu,
                                    (cell.status != 0 ? MF_DISABLED : 0)
                                        | MF_STRING,
                                    1, (gConfig.playerName[0] + L"完成").c_str());
                        AppendMenuW(menu,
                                    (cell.status != 0 ? MF_DISABLED : 0)
                                        | MF_STRING,
                                    2, (gConfig.playerName[1] + L"完成").c_str());
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, (cell.status == 0 ? MF_DISABLED : 0) | MF_STRING, 3, L"重置");
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING, 5, L"重新开始");
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING | MF_POPUP, (UINT_PTR)tables, L"重新随机表格");
                        if (syncMenu != nullptr) {
                            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                            AppendMenuW(menu, MF_STRING | MF_POPUP, (UINT_PTR)syncMenu, L"选手同步码");
                        }
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING | ((SDL_GetWindowFlags(gCells.window()) & SDL_WINDOW_ALWAYS_ON_TOP) ? MF_CHECKED : 0), 9, L"窗口置顶");
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING, 7, L"设置");
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING, 8, L"退出");
                        POINT pt;
                        GetCursorPos(&pt);
                        auto hwnd = (HWND)SDL_GetProperty(SDL_GetWindowProperties(gCells.window()), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
                        auto cmd = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, nullptr);
                        if (syncMenu != nullptr) DestroyMenu(syncMenu);
                        DestroyMenu(tables);
                        switch (cmd) {
                            case 1: {
                                cell.status = 1;
                                updateScores();
                                sendJudgeSyncState();
                                break;
                            }
                            case 2: {
                                cell.status = 2;
                                updateScores();
                                sendJudgeSyncState();
                                break;
                            }
                            case 3: {
                                cell.status = 0;
                                updateScores();
                                sendJudgeSyncState();
                                break;
                            }
                            case 5: {
                                gCells.foreach([](Cell &cell, int x, int y) {
                                    cell.status = 0;
                                });
                                gScoreWindows[0].reset();
                                gScoreWindows[1].reset();
                                sendJudgeSyncState();
                                break;
                            }
                            case 6:
                                randomCells("data/randomtable.txt");
                                break;
                            case 7:
                                gCells.showSettingsWindow();
                                break;
                            case 8:
                                DestroyMenu(menu);
                                goto QUIT;
                            case 9:
                                if (SDL_GetWindowFlags(gCells.window()) & SDL_WINDOW_ALWAYS_ON_TOP) {
                                    SDL_SetWindowAlwaysOnTop(gCells.window(), SDL_FALSE);
                                    for (auto &w: gScoreWindows) {
                                        SDL_SetWindowAlwaysOnTop(w.window[0], SDL_FALSE);
                                        SDL_SetWindowAlwaysOnTop(w.window[1], SDL_FALSE);
                                    }
                                } else {
                                    SDL_SetWindowAlwaysOnTop(gCells.window(), SDL_TRUE);
                                    for (auto &w: gScoreWindows) {
                                        SDL_SetWindowAlwaysOnTop(w.window[0], SDL_TRUE);
                                        SDL_SetWindowAlwaysOnTop(w.window[1], SDL_TRUE);
                                    }
                                }
                                break;
                            case 10:
                                if (OpenClipboard(nullptr)) {
                                    EmptyClipboard();
                                    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (password.size() + 1) * sizeof(wchar_t));
                                    if (hMem != nullptr) {
                                        auto *pMem = (wchar_t *)GlobalLock(hMem);
                                        if (pMem != nullptr) {
                                            lstrcpyW(pMem, Utf8ToUnicode(password).c_str());
                                            GlobalUnlock(hMem);
                                            SetClipboardData(CF_UNICODETEXT, hMem);
                                        }
                                    }
                                    CloseClipboard();
                                }
                                break;
                            case 11:
                                syncSetChannelPassword("");
                                syncSetChannelPasswordForC("");
                                syncClose();
                                startSync();
                                break;
                            case 99:
                                randomCells("");
                                break;
                            default:
                                if (cmd >= 100 && cmd < 100 + tableFilenames.size()) {
                                    randomCells("randomtables/" + tableFilenames[cmd - 100]);
                                }
                                break;
                        }
                    } else {
                        AppendMenuW(menu,
                                    (cell.status == 1 || cell.status == 3 || cell.status == 4 || count[0] >= gConfig.maxPerRow ? MF_DISABLED : 0)
                                        | MF_STRING,
                                    1, (gConfig.playerName[0] + L"完成").c_str());
                        AppendMenuW(menu,
                                    (cell.status == 2 || cell.status == 3 || cell.status == 4 || count[1] >= gConfig.maxPerRow ? MF_DISABLED : 0)
                                        | MF_STRING,
                                    2, (gConfig.playerName[1] + L"完成").c_str());
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, (cell.status == 0 ? MF_DISABLED : 0) | MF_STRING, 3, L"重置");
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING | (gScoreWindows[1].cleared ? MF_DISABLED : 0), 4,
                                    (gConfig.playerName[0] + (gScoreWindows[0].cleared ? L"重置为未通关状态" : L"通关")).c_str());
                        AppendMenuW(menu, MF_STRING | (gScoreWindows[0].cleared ? MF_DISABLED : 0), 5,
                                    (gConfig.playerName[1] + (gScoreWindows[1].cleared ? L"重置为未通关状态" : L"通关")).c_str());
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING, 6, L"重新开始");
                        if (syncMenu != nullptr) {
                            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                            AppendMenuW(menu, MF_STRING | MF_POPUP, (UINT_PTR)syncMenu, L"选手同步码");
                        }
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING | ((SDL_GetWindowFlags(gCells.window()) & SDL_WINDOW_ALWAYS_ON_TOP) ? MF_CHECKED : 0), 9, L"窗口置顶");
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING, 7, L"设置");
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING, 8, L"退出");
                        POINT pt;
                        GetCursorPos(&pt);
                        auto hwnd = (HWND)SDL_GetProperty(SDL_GetWindowProperties(gCells.window()), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
                        auto cmd = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, nullptr);
                        DestroyMenu(menu);
                        switch (cmd) {
                            case 1: {
                                if (cell.status == 0) {
                                    cell.status = 1;
                                } else {
                                    cell.status = 4;
                                }
                                updateScores();
                                sendJudgeSyncState();
                                break;
                            }
                            case 2: {
                                if (cell.status == 0) {
                                    cell.status = 2;
                                } else {
                                    cell.status = 3;
                                }
                                updateScores();
                                sendJudgeSyncState();
                                break;
                            }
                            case 3: {
                                cell.status = 0;
                                updateScores();
                                sendJudgeSyncState();
                                break;
                            }
                            case 4: {
                                gScoreWindows[0].setClear(!gScoreWindows[0].cleared);
                                updateScores();
                                sendJudgeSyncState();
                                break;
                            }
                            case 5: {
                                gScoreWindows[1].setClear(!gScoreWindows[1].cleared);
                                updateScores();
                                sendJudgeSyncState();
                                break;
                            }
                            case 6: {
                                gCells.foreach([](Cell &cell, int x, int y) {
                                    cell.status = 0;
                                });
                                gScoreWindows[0].reset();
                                gScoreWindows[1].reset();
                                sendJudgeSyncState();
                                break;
                            }
                            case 7:
                                gCells.showSettingsWindow();
                                break;
                            case 8:
                                DestroyMenu(menu);
                                goto QUIT;
                            case 9:
                                if (SDL_GetWindowFlags(gCells.window()) & SDL_WINDOW_ALWAYS_ON_TOP) {
                                    SDL_SetWindowAlwaysOnTop(gCells.window(), SDL_FALSE);
                                    for (auto &w: gScoreWindows) {
                                        SDL_SetWindowAlwaysOnTop(w.window[0], SDL_FALSE);
                                        SDL_SetWindowAlwaysOnTop(w.window[1], SDL_FALSE);
                                    }
                                } else {
                                    SDL_SetWindowAlwaysOnTop(gCells.window(), SDL_TRUE);
                                    for (auto &w: gScoreWindows) {
                                        SDL_SetWindowAlwaysOnTop(w.window[0], SDL_TRUE);
                                        SDL_SetWindowAlwaysOnTop(w.window[1], SDL_TRUE);
                                    }
                                }
                                break;
                            case 10:
                                if (OpenClipboard(nullptr)) {
                                    EmptyClipboard();
                                    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (password.size() + 1) * sizeof(wchar_t));
                                    if (hMem != nullptr) {
                                        auto *pMem = (wchar_t *)GlobalLock(hMem);
                                        if (pMem != nullptr) {
                                            lstrcpyW(pMem, Utf8ToUnicode(password).c_str());
                                            GlobalUnlock(hMem);
                                            SetClipboardData(CF_UNICODETEXT, hMem);
                                        }
                                    }
                                    CloseClipboard();
                                }
                                break;
                            case 11:
                                syncSetChannelPassword("");
                                syncSetChannelPasswordForC("");
                                syncClose();
                                startSync();
                                break;
                            default:
                                break;
                        }
                    }
                    DestroyMenu(menu);
                    break;
                }
            }
        }
        gCells.render();
        for (auto &sw : gScoreWindows) {
            sw.render();
        }
        syncProcess();
    }
    QUIT:
    saveState();
    TTF_CloseFont(gConfig.font);
    for (auto &w: gScoreWindows) {
        w.destroy();
    }
    gCells.foreach([](Cell &cell, int x, int y) {
        SDL_DestroyTexture(cell.texture);
        cell.texture = nullptr;
    });
    gCells.deinit();

    TTF_Quit();
    SDL_Quit();
    return 0;
}