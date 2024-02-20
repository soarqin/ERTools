#define WIN32_LEAN_AND_MEAN
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <windows.h>
#include <shellapi.h>
#undef WIN32_LEAN_AND_MEAN

#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include <utility>
#include <random>
#include <algorithm>

static int gCellSize = 150;
static int gCellSpacing = 2;
static int gAlpha = 128;
static std::string gFontFile = "data/font.ttf";
static int gFontSize = 24;
static std::string gScoreFontFile = "data/font.ttf";
static int gScoreFontSize = 24;
static int gScoreAlpha = 0;
static SDL_Color gTextColor = {255, 255, 255, 255};
static SDL_FColor gColors[3] = {
    {0, 0, 0, 0},
    {1, 0, 0, 0},
    {0, 0, 1, 0},
};
static int gBingoBrawlersMode = 0;
static int gScores[5] = {2, 4, 6, 8, 10};
static int gNFScores[5] = {1, 2, 3, 4, 5};
static int gLineScore = 3;
static int gMaxPerRow = 3;
static int gClearScore = 0;
static int gClearQuestMultiplier = 2;
static std::wstring gPlayerName[2] = {L"红方", L"蓝方"};

static SDL_Window *gWindow = nullptr;
static SDL_Renderer *gRenderer = nullptr;
static TTF_Font *gFont = nullptr;
static TTF_Font *gScoreFont = nullptr;

struct Cell {
    std::string text;
    SDL_Texture *texture = nullptr;
    uint32_t status = 0;
    void updateTexture() {
        auto *font = gFont;
        auto fontSize = gFontSize;
        while (true) {
            auto *surface =
                TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), gTextColor, gCellSize * 9 / 10);
            if (surface->h <= gCellSize * 9 / 10) {
                texture = SDL_CreateTextureFromSurface(gRenderer, surface);
                SDL_DestroySurface(surface);
                break;
            }
            fontSize--;
            font = TTF_OpenFont(gFontFile.c_str(), fontSize);
            TTF_SetFontWrappedAlign(font, TTF_WRAPPED_ALIGN_CENTER);
        }
        if (font != gFont) {
            TTF_CloseFont(font);
        }
    }
};

static Cell gCells[5][5];

struct ScoreWindow {
    static SDL_HitTestResult ScoreWindowHitTestCallback(SDL_Window *window, const SDL_Point *pt, void *data) {
        (void)data;
        (void)window;
        (void)pt;
        return SDL_HITTEST_DRAGGABLE;
    }

    void reset() {
        score = 0;
        extraScore = 0;
        hasExtraScore = false;
        updateTexture();
    }

    void create(int idx) {
        index = idx;
        char name[256];
        WideCharToMultiByte(CP_UTF8, 0, gPlayerName[idx].c_str(), -1, name, 256, nullptr, nullptr);
        playerName = name;
        window = SDL_CreateWindow(name, 200, 200, SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT | SDL_WINDOW_ALWAYS_ON_TOP);
        renderer = SDL_CreateRenderer(window, "opengl", SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        SDL_SetWindowHitTest(window, ScoreWindowHitTestCallback, nullptr);
        int x, y, w, h;
        SDL_GetWindowPosition(gWindow, &x, &y);
        SDL_GetWindowSize(gWindow, &w, &h);
        SDL_SetWindowPosition(window, x, y + h + 8 + idx * 50);
    }

    void destroy() {
        if (texture) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
        if (renderer) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }
        if (window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
    }

    void updateTexture() {
        if (texture) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
        auto clrf = gColors[index + 1];
        SDL_Color clr = { (uint8_t)roundf(clrf.r * 255), (uint8_t)roundf(clrf.g * 255), (uint8_t)roundf(clrf.b * 255), 255 };
        auto surface =
            gBingoBrawlersMode
            ? TTF_RenderUTF8_Blended(gScoreFont, (playerName + (score >= 100 ? "达成Bingo" : ("完成：" + std::to_string(score) + (score >= 13 ? " 完成数获胜" : "")))).c_str(), clr)
            : TTF_RenderUTF8_Blended(gScoreFont, (playerName + "积分：" + std::to_string(score)).c_str(), clr);
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
        int w, h;
        SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
        SDL_SetWindowSize(window, w, h);
    }

    inline void setExtraScore(int idx) {
        int count[2] = {0, 0};
        for (auto &r: gCells) {
            for (auto &c: r) {
                switch (c.status) {
                    case 1:
                        count[0]++;
                        break;
                    case 2:
                        count[1]++;
                        break;
                }
            }
        }
        extraScore = gClearScore + gClearQuestMultiplier * (count[idx] - count[1 - idx]);
        hasExtraScore = true;
    }

    inline void unsetExtraScore() {
        extraScore = 0;
        hasExtraScore = false;
    }

    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *texture = nullptr;
    std::string playerName;
    int index = -1;
    int score = -1;
    bool hasExtraScore = false;
    int extraScore = 0;
};

static ScoreWindow gScoreWindows[2];

static void updateScores() {
    if (gBingoBrawlersMode) {
        int score[2] = {0, 0};
        for (auto & row : gCells) {
            for (auto & cell : row) {
                switch (cell.status) {
                    case 1:
                        score[0]++;
                        break;
                    case 2:
                        score[1]++;
                        break;
                    default:
                        break;
                }
            }
        }
        auto n = gCells[0][0].status;
        if (n) {
            auto match = true;
            for (int i = 1; i < 5; i++) {
                if (gCells[i][i].status != n) {
                    match = false;
                    break;
                }
            }
            if (match) {
                score[n - 1] = 100;
            }
        }
        n = gCells[4][0].status;
        if (n) {
            auto match = true;
            for (int i = 1; i < 5; i++) {
                if (gCells[4 - i][i].status != n) {
                    match = false;
                    break;
                }
            }
            if (match) {
                score[n - 1] = 100;
            }
        }
        for (int j = 0; j < 5; j++) {
            n = gCells[j][0].status;
            if (n) {
                auto match = true;
                for (int i = 1; i < 5; i++) {
                    if (gCells[j][i].status != n) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    score[n - 1] = 100;
                }
            }
            n = gCells[0][j].status;
            if (n) {
                auto match = true;
                for (int i = 1; i < 5; i++) {
                    if (gCells[i][j].status != n) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    score[n - 1] = 100;
                }
            }
        }
        if (score[0] != gScoreWindows[0].score) {
            gScoreWindows[0].score = score[0];
            gScoreWindows[0].updateTexture();
        }
        if (score[1] != gScoreWindows[1].score) {
            gScoreWindows[1].score = score[1];
            gScoreWindows[1].updateTexture();
        }
        return;
    }
    int score[2] = {0, 0};
    int line[5] = {0};
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 5; ++j) {
            auto &cell = gCells[i][j];
            switch (cell.status) {
                case 1:
                    score[0] += gScores[i];
                    if (i == 0) line[j] = 0;
                    else if (line[j] != 0) line[j] = -1;
                    break;
                case 2:
                    score[1] += gScores[i];
                    if (i == 0) line[j] = 1;
                    else if (line[j] != 1) line[j] = -1;
                    break;
                case 3:
                    score[0] += gScores[i];
                    score[1] += gNFScores[i];
                    if (i == 0) line[j] = 0;
                    else if (line[j] != 0) line[j] = -1;
                    break;
                case 4:
                    score[0] += gNFScores[i];
                    score[1] += gScores[i];
                    if (i == 0) line[j] = 1;
                    else if (line[j] != 1) line[j] = -1;
                    break;
                default:
                    line[j] = -1;
                    break;
            }
        }
    }
    int cross[2] = {0, 0};
    for (int i = 0; i < 5; ++i) {
        switch (gCells[i][i].status) {
            case 1:
            case 3:
                if (i == 0) cross[0] = 0;
                else if (cross[0] != 0) cross[0] = -1;
                break;
            case 2:
            case 4:
                if (i == 0) cross[0] = 1;
                else if (cross[0] != 1) cross[0] = -1;
                break;
            default:
                cross[0] = -1;
                break;
        }
        switch (gCells[i][4 - i].status) {
            case 1:
            case 3:
                if (i == 0) cross[1] = 0;
                else if (cross[1] != 0) cross[1] = -1;
                break;
            case 2:
            case 4:
                if (i == 0) cross[1] = 1;
                else if (cross[1] != 1) cross[1] = -1;
                break;
            default:
                cross[1] = -1;
                break;
        }
    }
    for (int i: line) {
        if (i < 0) continue;
        score[i] += gLineScore;
    }
    for (int i: cross) {
        if (i < 0) continue;
        score[i] += gLineScore;
    }
    if (gScoreWindows[0].hasExtraScore) {
        score[0] += gScoreWindows[0].extraScore;
    }
    if (gScoreWindows[1].hasExtraScore) {
        score[1] += gScoreWindows[1].extraScore;
    }
    if (score[0] != gScoreWindows[0].score) {
        gScoreWindows[0].score = score[0];
        gScoreWindows[0].updateTexture();
    }
    if (score[1] != gScoreWindows[1].score) {
        gScoreWindows[1].score = score[1];
        gScoreWindows[1].updateTexture();
    }
}

static std::vector<std::string> splitString(const std::string &str, char sep) {
    std::vector<std::string> result;
    std::string::size_type pos1 = 0, pos2 = str.find(sep);
    while (pos2 != std::string::npos) {
        result.push_back(str.substr(pos1, pos2 - pos1));
        pos1 = pos2 + 1;
        pos2 = str.find(sep, pos1);
    }
    if (pos1 < str.length()) {
        result.push_back(str.substr(pos1));
    }
    return result;
}

static void randomCells() {
    std::ifstream ifs("data/randomtable.txt");
    std::string line;
    std::vector<std::string> fixed;
    struct Group {
        uint64_t ratio = 0;
        uint64_t total = 0;
        std::vector<std::pair<uint64_t, std::string>> strings;
    };
    std::vector<Group> groups;
    std::vector<std::pair<uint64_t, std::string>> strings;
    uint64_t total = 0;
    while (!ifs.eof()) {
        std::getline(ifs, line);
        if (line.empty() || line[0] == ';') continue;
        if (line[0] == 'F') {
            auto sl = splitString(line, ',');
            fixed.emplace_back(sl[1]);
            continue;
        }
        if (line[0] == 'G') {
            auto sl = splitString(line, ',');
            auto id = std::stoi(sl[1]);
            auto ratio = std::stoul(sl[2]);
            if (id >= (int)groups.size()) {
                groups.resize(id + 1);
            }
            groups[id].ratio = ratio;
            total += ratio;
            continue;
        }
        if (line[0] < '0' || line[0] > '9') continue;
        auto sl = splitString(line, ',');
        auto groupId = std::stoi(sl[0]);
        auto ratio = std::stoul(sl[1]);
        if (groupId == 0) {
            strings.emplace_back(ratio, sl[2]);
            total += ratio;
        } else {
            if (groupId >= (int)groups.size()) {
                groups.resize(groupId + 1);
            }
            auto &group = groups[groupId];
            group.strings.emplace_back(ratio, sl[2]);
            group.total += ratio;
        }
    }
    int idx = 0;
    std::string result[25];
    for (auto &s: fixed) {
        result[idx] = s;
        if (++idx >= 25) break;
    }
    static std::mt19937_64 rand((std::random_device())());
    for (; idx < 25; idx++) {
        uint64_t r = rand() % total;
        for (auto ite = strings.begin(); ite != strings.end(); ite++) {
            if (r < ite->first) {
                result[idx] = ite->second;
                total -= ite->first;
                strings.erase(ite);
                break;
            }
            r -= ite->first;
        }
        for (auto ite = groups.begin(); ite != groups.end(); ite++) {
            if (r < ite->ratio) {
                auto r2 = rand() % total;
                for (auto &s: ite->strings) {
                    if (r2 < s.first) {
                        result[idx] = s.second;
                        break;
                    }
                    r2 -= s.first;
                }
                break;
            }
            r -= ite->total;
            groups.erase(ite);
        }
    }
    std::shuffle(result, result + 25, rand);
    std::ofstream ofs("data/squares.txt");
    for (auto &r: gCells) {
        for (auto &c: r) {
            c.status = 0;
            c.text = result[--idx];
            c.updateTexture();
            ofs << c.text << std::endl;
        }
    }
    ofs.close();
    gScoreWindows[0].reset();
    gScoreWindows[1].reset();
}

static std::string mergeString(const std::vector<std::string> &strs, char sep) {
    std::string result;
    for (auto &s: strs) {
        if (!result.empty()) result += sep;
        result += s;
    }
    return result;
}

static void load() {
    std::ifstream ifs("data/config.txt");
    std::string line;
    while (!ifs.eof()) {
        std::getline(ifs, line);
        if (line.empty() || line[0] == ';') continue;
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        auto key = line.substr(0, pos);
        auto value = line.substr(pos + 1);
        if (key == "CellSize") {
            gCellSize = std::stoi(value);
        } else if (key == "CellSpacing") {
            gCellSpacing = std::stoi(value);
        } else if (key == "FontFile") {
            gFontFile = value;
        } else if (key == "FontSize") {
            gFontSize = std::stoi(value);
        } else if (key == "Alpha") {
            gAlpha = std::stoi(value);
        } else if (key == "TextColor") {
            auto sl = splitString(value, ',');
            if (sl.size() != 3) continue;
            gTextColor.r = std::stoi(sl[0]);
            gTextColor.g = std::stoi(sl[1]);
            gTextColor.b = std::stoi(sl[2]);
        } else if (key == "Color1") {
            auto sl = splitString(value, ',');
            if (sl.size() != 3) continue;
            gColors[1].r = std::stof(sl[0]) / 255.f;
            gColors[1].g = std::stof(sl[1]) / 255.f;
            gColors[1].b = std::stof(sl[2]) / 255.f;
        } else if (key == "Color2") {
            auto sl = splitString(value, ',');
            if (sl.size() != 3) continue;
            gColors[2].r = std::stof(sl[0]) / 255.f;
            gColors[2].g = std::stof(sl[1]) / 255.f;
            gColors[2].b = std::stof(sl[2]) / 255.f;
        } else if (key == "BingoBrawlersMode") {
            gBingoBrawlersMode = std::stoi(value) != 0 ? 1 : 0;
        } else if (key == "ScoreAlpha") {
            gScoreAlpha = std::stoi(value);
        } else if (key == "ScoreFontFile") {
            gScoreFontFile = value;
        } else if (key == "ScoreFontSize") {
            gScoreFontSize = std::stoi(value);
        } else if (key == "FirstScores") {
            auto sl = splitString(value, ',');
            int sz = (int)sl.size();
            if (sz > 5) sz = 5;
            int i;
            for (i = 0; i < sz; i++) {
                gScores[i] = std::stoi(sl[i]);
            }
            if (i == 0) {
                gScores[0] = 2;
                i = 1;
            }
            for (; i < 5; i++) {
                gScores[i] = gScores[i - 1];
            }
        } else if (key == "NoneFirstScores") {
            auto sl = splitString(value, ',');
            int sz = (int)sl.size();
            if (sz > 5) sz = 5;
            int i;
            for (i = 0; i < sz; i++) {
                gNFScores[i] = std::stoi(sl[i]);
            }
            if (i == 0) {
                gNFScores[0] = 1;
                i = 1;
            }
            for (; i < 5; i++) {
                gNFScores[i] = gNFScores[i - 1];
            }
        } else if (key == "LineScore") {
            gLineScore = std::stoi(value);
        } else if (key == "MaxPerRow") {
            gMaxPerRow = std::stoi(value);
        } else if (key == "ClearScore") {
            gClearScore = std::stoi(value);
        } else if (key == "ClearQuestMultiplier") {
            gClearQuestMultiplier = std::stoi(value);
        } else if (key == "Player1") {
            wchar_t name[256];
            MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, name, 256);
            gPlayerName[0] = name;
        } else if (key == "Player2") {
            wchar_t name[256];
            MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, name, 256);
            gPlayerName[1] = name;
        }
    }
    for (auto &c: gColors) {
        c.a = (float)gAlpha / 255.f;
    }
    if (gBingoBrawlersMode) {
        gMaxPerRow = 5;
        gClearQuestMultiplier = 0;
        for (int i = 0; i < 5; i++) {
            gScores[i] = 1;
            gNFScores[i] = 0;
        }
    }
}

static void postLoad() {
    gFont = TTF_OpenFont(gFontFile.c_str(), gFontSize);
    TTF_SetFontWrappedAlign(gFont, TTF_WRAPPED_ALIGN_CENTER);
    gScoreFont = TTF_OpenFont(gScoreFontFile.c_str(), gScoreFontSize);
    std::ifstream ifs("data/squares.txt");
    std::string line;
    for (auto & row : gCells) {
        for (auto & cell : row) {
            std::getline(ifs, line);
            cell.text = line;
            cell.updateTexture();
        }
    }
}

static void saveState() {
    std::ofstream ofs("state.txt");
    std::vector<std::string> n;
    for (auto &w: gScoreWindows) {
        n.emplace_back(std::to_string(w.hasExtraScore ? 1 : 0));
        n.emplace_back(std::to_string(w.extraScore));
    }
    for (auto &r: gCells) {
        for (auto &c: r) {
            n.emplace_back(std::to_string(c.status));
        }
    }
    int x, y;
    SDL_GetWindowPosition(gWindow, &x, &y);
    n.emplace_back(std::to_string(x));
    n.emplace_back(std::to_string(y));
    for (auto &win: gScoreWindows) {
        SDL_GetWindowPosition(win.window, &x, &y);
        n.emplace_back(std::to_string(x));
        n.emplace_back(std::to_string(y));
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
        w.hasExtraScore = sl[i++] == "1";
        w.extraScore = std::stoi(sl[i++]);
    }
    for (auto &r: gCells) {
        for (auto &c: r) {
            c.status = std::stoi(sl[i++]);
        }
    }
    int x = std::stoi(sl[i++]);
    int y = std::stoi(sl[i++]);
    SDL_SetWindowPosition(gWindow, x, y);
    for (auto &win: gScoreWindows) {
        x = std::stoi(sl[i++]);
        y = std::stoi(sl[i++]);
        SDL_SetWindowPosition(win.window, x, y);
    }
}

SDL_HitTestResult HitTestCallback(SDL_Window *window, const SDL_Point *pt, void *data) {
    (void)data;
    (void)window;
    if (pt->y < 30)
        return SDL_HITTEST_DRAGGABLE;
    return SDL_HITTEST_NORMAL;
}

int wmain(int argc, wchar_t *argv[]) {
    // Unused argc, argv
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) < 0) {
        printf("SDL could not be initialized!\n"
               "SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    TTF_Init();
    load();

    SDL_SetHint("SDL_BORDERLESS_RESIZABLE_STYLE", "1");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
    gWindow = SDL_CreateWindow("BingoTool",
                               gCellSize * 5 + gCellSpacing * 4,
                               gCellSize * 5 + gCellSpacing * 4,
                               SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT | SDL_WINDOW_ALWAYS_ON_TOP);
    SDL_SetWindowPosition(gWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    gRenderer = SDL_CreateRenderer(gWindow, "opengl", SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetWindowHitTest(gWindow, HitTestCallback, nullptr);
    SDL_SetRenderDrawBlendMode(gRenderer,
                               SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_SRC_COLOR, SDL_BLENDFACTOR_ZERO,
                                                          SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_SRC_ALPHA,
                                                          SDL_BLENDFACTOR_ZERO, SDL_BLENDOPERATION_ADD));

    postLoad();
    gScoreWindows[0].create(0);
    gScoreWindows[1].create(1);
    loadState();
    updateScores();

    while (true) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_EVENT_QUIT:
                    goto QUIT;
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
                    if (e.window.windowID == SDL_GetWindowID(gWindow)) goto QUIT;
                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_UP: {
                    if (e.button.windowID == SDL_GetWindowID(gWindow) && e.button.button != SDL_BUTTON_RIGHT) break;
                    float fx, fy;
                    SDL_GetMouseState(&fx, &fy);
                    int x = (int)fx, y = (int)fy;
                    int dx = x / (gCellSize + gCellSpacing);
                    int dy = y / (gCellSize + gCellSpacing);
                    if (dx >= gCellSize || dy >= gCellSize) break;
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
                    if (gBingoBrawlersMode) {
                        AppendMenuW(menu,
                                    (cell.status != 0 ? MF_DISABLED : 0)
                                        | MF_STRING,
                                    1, (gPlayerName[0] + L"完成").c_str());
                        AppendMenuW(menu,
                                    (cell.status != 0 ? MF_DISABLED : 0)
                                        | MF_STRING,
                                    2, (gPlayerName[1] + L"完成").c_str());
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, (cell.status == 0 ? MF_DISABLED : 0) | MF_STRING, 3, L"重置");
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING, 5, L"重新开始");
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING, 6, L"重新随机表格");
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING, 7, L"退出");
                        POINT pt;
                        GetCursorPos(&pt);
                        auto hwnd = (HWND)SDL_GetProperty(SDL_GetWindowProperties(gWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
                        auto cmd = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, nullptr);
                        switch (cmd) {
                            case 1: {
                                cell.status = 1;
                                updateScores();
                                break;
                            }
                            case 2: {
                                cell.status = 2;
                                updateScores();
                                break;
                            }
                            case 3: {
                                cell.status = 0;
                                updateScores();
                                break;
                            }
                            case 5: {
                                for (auto &r: gCells) {
                                    for (auto &c: r) {
                                        c.status = 0;
                                    }
                                }
                                gScoreWindows[0].reset();
                                gScoreWindows[1].reset();
                                break;
                            }
                            case 6:
                                randomCells();
                                break;
                            case 7:
                                goto QUIT;
                            default:
                                break;
                        }
                    } else {
                        AppendMenuW(menu,
                                    (cell.status == 1 || cell.status == 3 || cell.status == 4 || count[0] >= gMaxPerRow ? MF_DISABLED : 0)
                                        | MF_STRING,
                                    1, (gPlayerName[0] + L"完成").c_str());
                        AppendMenuW(menu,
                                    (cell.status == 2 || cell.status == 3 || cell.status == 4 || count[1] >= gMaxPerRow ? MF_DISABLED : 0)
                                        | MF_STRING,
                                    2, (gPlayerName[1] + L"完成").c_str());
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, (cell.status == 0 ? MF_DISABLED : 0) | MF_STRING, 3, L"重置");
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING | (gScoreWindows[1].hasExtraScore ? MF_DISABLED : 0), 4,
                                    (gPlayerName[0] + (gScoreWindows[0].hasExtraScore ? L"重置为未通关状态" : L"通关")).c_str());
                        AppendMenuW(menu, MF_STRING | (gScoreWindows[0].hasExtraScore ? MF_DISABLED : 0), 5,
                                    (gPlayerName[1] + (gScoreWindows[1].hasExtraScore ? L"重置为未通关状态" : L"通关")).c_str());
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING, 6, L"重新开始");
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING, 7, L"退出");
                        POINT pt;
                        GetCursorPos(&pt);
                        auto hwnd = (HWND)SDL_GetProperty(SDL_GetWindowProperties(gWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
                        auto cmd = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, nullptr);
                        switch (cmd) {
                            case 1: {
                                if (cell.status == 0) {
                                    cell.status = 1;
                                } else {
                                    cell.status = 4;
                                }
                                updateScores();
                                break;
                            }
                            case 2: {
                                if (cell.status == 0) {
                                    cell.status = 2;
                                } else {
                                    cell.status = 3;
                                }
                                updateScores();
                                break;
                            }
                            case 3: {
                                cell.status = 0;
                                updateScores();
                                break;
                            }
                            case 4: {
                                if (gScoreWindows[0].hasExtraScore) {
                                    gScoreWindows[0].unsetExtraScore();
                                } else {
                                    gScoreWindows[0].setExtraScore(0);
                                }
                                updateScores();
                                break;
                            }
                            case 5: {
                                if (gScoreWindows[1].hasExtraScore) {
                                    gScoreWindows[1].unsetExtraScore();
                                } else {
                                    gScoreWindows[1].setExtraScore(1);
                                }
                                updateScores();
                                break;
                            }
                            case 6: {
                                for (auto &r: gCells) {
                                    for (auto &c: r) {
                                        c.status = 0;
                                    }
                                }
                                gScoreWindows[0].reset();
                                gScoreWindows[1].reset();
                                break;
                            }
                            case 7:
                                goto QUIT;
                            default:
                                break;
                        }
                    }
                    break;
                }
            }
        }
        SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0);
        SDL_RenderClear(gRenderer);
        auto cs = (float)gCellSize;
        for (int i = 0; i < 5; i++) {
            auto y = (float)(i * (gCellSize + gCellSpacing));
            for (int j = 0; j < 5; j++) {
                auto x = (float)(j * (gCellSize + gCellSpacing));
                auto &cell = gCells[i][j];
                bool dbl = cell.status > 2;
                auto &col = gColors[dbl ? (cell.status - 2) : cell.status];
                SDL_SetRenderDrawColorFloat(gRenderer, col.r, col.g, col.b, col.a);
                SDL_FRect rect = {x, y, cs, cs};
                SDL_RenderFillRect(gRenderer, &rect);
                if (dbl) {
                    auto &color = gColors[5 - cell.status];
                    const SDL_Vertex verts[] = {
                        {SDL_FPoint{x, y + cs}, color, SDL_FPoint{0},},
                        {SDL_FPoint{x, y + cs * .7f}, color, SDL_FPoint{0},},
                        {SDL_FPoint{x + cs * .3f, y + cs}, color, SDL_FPoint{0},},
                    };
                    SDL_RenderGeometry(gRenderer, nullptr, verts, 3, nullptr, 0);
                }
                if (cell.texture) {
                    int w, h;
                    SDL_QueryTexture(cell.texture, nullptr, nullptr, &w, &h);
                    auto ww = (float)w, hh = (float)h;
                    auto l = (float)(gCellSize - w) / 2, t = (float)(gCellSize - h) / 2;
                    rect = {x + l, y + t, ww, hh};
                    SDL_RenderTexture(gRenderer, cell.texture, nullptr, &rect);
                }
            }
        }
        for (auto &sw : gScoreWindows) {
            SDL_SetRenderDrawColor(sw.renderer, 0, 0, 0, gScoreAlpha);
            SDL_RenderClear(sw.renderer);
            SDL_RenderTexture(sw.renderer, sw.texture, nullptr, nullptr);
            SDL_RenderPresent(sw.renderer);
        }
        SDL_RenderPresent(gRenderer);
    }
    QUIT:
    saveState();
    TTF_CloseFont(gScoreFont);
    TTF_CloseFont(gFont);
    for (auto &w: gScoreWindows) {
        w.destroy();
    }
    for (auto &c: gCells) {
        for (auto &cc: c) {
            SDL_DestroyTexture(cc.texture);
            cc.texture = nullptr;
        }
    }
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    TTF_Quit();
    SDL_Quit();
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    int argc;
    wchar_t **argv = CommandLineToArgvW(lpCmdLine, &argc);
    return wmain(argc, argv);
}
