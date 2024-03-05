#include "config.h"

#include "common.h"

#include <fstream>

Config gConfig;

inline int setFontStyle(const std::string &style) {
    int res = TTF_STYLE_NORMAL;
    for (auto &c: style) {
        switch (c) {
            case 'B':
                res |= TTF_STYLE_BOLD;
                break;
            case 'I':
                res |= TTF_STYLE_ITALIC;
                break;
            default:
                break;
        }
    }
    return res;
}

void Config::load() {
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
            auto sl = splitString(value, ',');
            switch (sl.size()) {
                case 1:
                    cellSize[0] = cellSize[1] = std::stoi(sl[0]);
                    break;
                case 2:
                    cellSize[0] = std::stoi(sl[0]);
                    cellSize[1] = std::stoi(sl[1]);
                    break;
                default:
                    break;
            }
        } else if (key == "CellRoundCorner") {
            cellRoundCorner = std::stoi(value);
        } else if (key == "CellSpacing") {
            cellSpacing = std::stoi(value);
        } else if (key == "CellBorder") {
            cellBorder = std::stoi(value);
        } else if (key == "FontFile") {
            auto sl = splitString(value, '|');
            if (sl.size() == 2) {
                fontFile = sl[0];
                fontStyle = setFontStyle(sl[1]);
            } else {
                fontFile = value;
            }
        } else if (key == "CellColor") {
            auto sl = splitString(value, ',');
            if (sl.size() != 4) continue;
            colorsInt[0].r = std::stoi(sl[0]);
            colorsInt[0].g = std::stoi(sl[1]);
            colorsInt[0].b = std::stoi(sl[2]);
            colorsInt[0].a = std::stoi(sl[3]);
        } else if (key == "CellSpacincolor") {
            auto sl = splitString(value, ',');
            if (sl.size() != 4) continue;
            cellSpacingColor.r = std::stoi(sl[0]);
            cellSpacingColor.g = std::stoi(sl[1]);
            cellSpacingColor.b = std::stoi(sl[2]);
            cellSpacingColor.a = std::stoi(sl[3]);
        } else if (key == "CellBorderColor") {
            auto sl = splitString(value, ',');
            if (sl.size() != 4) continue;
            cellBorderColor.r = std::stoi(sl[0]);
            cellBorderColor.g = std::stoi(sl[1]);
            cellBorderColor.b = std::stoi(sl[2]);
            cellBorderColor.a = std::stoi(sl[3]);
        } else if (key == "FontSize") {
            fontSize = std::stoi(value);
        } else if (key == "TextColor") {
            auto sl = splitString(value, ',');
            if (sl.size() != 4) continue;
            textColor.r = std::stoi(sl[0]);
            textColor.g = std::stoi(sl[1]);
            textColor.b = std::stoi(sl[2]);
            textColor.a = std::stoi(sl[3]);
        } else if (key == "TextShadow") {
            textShadow = std::stoi(value);
        } else if (key == "TextShadowColor") {
            auto sl = splitString(value, ',');
            if (sl.size() != 4) continue;
            textShadowColor.r = std::stoi(sl[0]);
            textShadowColor.g = std::stoi(sl[1]);
            textShadowColor.b = std::stoi(sl[2]);
            textShadowColor.a = std::stoi(sl[3]);
        } else if (key == "TextShadowOffset") {
            auto sl = splitString(value, ',');
            if (sl.size() != 2) continue;
            textShadowOffset[0] = std::stoi(sl[0]);
            textShadowOffset[1] = std::stoi(sl[1]);
        } else if (key == "Color1") {
            auto sl = splitString(value, ',');
            if (sl.size() == 3) {
                colorsInt[1].r = std::stoi(sl[0]);
                colorsInt[1].g = std::stoi(sl[1]);
                colorsInt[1].b = std::stoi(sl[2]);
                colorTextureFile[0].clear();
                colorTexture[0] = nullptr;
            } else if (sl.size() == 1) {
                colorTextureFile[0] = sl[0].c_str();
            }
        } else if (key == "Color2") {
            auto sl = splitString(value, ',');
            if (sl.size() == 3) {
                colorsInt[2].r = std::stoi(sl[0]);
                colorsInt[2].g = std::stoi(sl[1]);
                colorsInt[2].b = std::stoi(sl[2]);
                colorTextureFile[1].clear();
                colorTexture[1] = nullptr;
            } else if (sl.size() == 1) {
                colorTextureFile[1] = sl[0].c_str();
            }
        } else if (key == "BingoBrawlersMode") {
            bingoBrawlersMode = std::stoi(value) != 0 ? 1 : 0;
        } else if (key == "ScoreBackgroundColor") {
            auto sl = splitString(value, ',');
            if (sl.size() != 4) continue;
            scoreBackgroundColor.r = std::stoi(sl[0]);
            scoreBackgroundColor.g = std::stoi(sl[1]);
            scoreBackgroundColor.b = std::stoi(sl[2]);
            scoreBackgroundColor.a = std::stoi(sl[3]);
        } else if (key == "ScorePadding") {
            scorePadding = std::stoi(value);
        } else if (key == "ScoreRoundCorner") {
            scoreRoundCorner = std::stoi(value);
        } else if (key == "ScoreFontFile") {
            auto sl = splitString(value, '|');
            if (sl.size() == 2) {
                scoreFontFile = sl[0];
                scoreFontStyle = setFontStyle(sl[1]);
            } else {
                scoreFontFile = value;
            }
        } else if (key == "ScoreFontSize") {
            scoreFontSize = std::stoi(value);
        } else if (key == "ScoreTextShadow") {
            scoreTextShadow = std::stoi(value);
        } else if (key == "ScoreTextShadowColor") {
            auto sl = splitString(value, ',');
            if (sl.size() != 4) continue;
            scoreTextShadowColor.r = std::stoi(sl[0]);
            scoreTextShadowColor.g = std::stoi(sl[1]);
            scoreTextShadowColor.b = std::stoi(sl[2]);
            scoreTextShadowColor.a = std::stoi(sl[3]);
        } else if (key == "ScoreTextShadowOffset") {
            auto sl = splitString(value, ',');
            if (sl.size() != 2) continue;
            scoreTextShadowOffset[0] = std::stoi(sl[0]);
            scoreTextShadowOffset[1] = std::stoi(sl[1]);
        } else if (key == "ScoreNameFontFile") {
            auto sl = splitString(value, '|');
            if (sl.size() == 2) {
                scoreNameFontFile = sl[0];
                scoreNameFontStyle = setFontStyle(sl[1]);
            } else {
                scoreNameFontFile = value;
            }
        } else if (key == "ScoreNameFontSize") {
            scoreNameFontSize = std::stoi(value);
        } else if (key == "ScoreNameTextShadow") {
            scoreNameTextShadow = std::stoi(value);
        } else if (key == "ScoreNameTextShadowColor") {
            auto sl = splitString(value, ',');
            if (sl.size() != 4) continue;
            scoreNameTextShadowColor.r = std::stoi(sl[0]);
            scoreNameTextShadowColor.g = std::stoi(sl[1]);
            scoreNameTextShadowColor.b = std::stoi(sl[2]);
            scoreNameTextShadowColor.a = std::stoi(sl[3]);
        } else if (key == "ScoreNameTextShadowOffset") {
            auto sl = splitString(value, ',');
            if (sl.size() != 2) continue;
            scoreNameTextShadowOffset[0] = std::stoi(sl[0]);
            scoreNameTextShadowOffset[1] = std::stoi(sl[1]);
        } else if (key == "ScoreWinText") {
            scoreWinText = value;
            unescape(scoreWinText);
        } else if (key == "ScoreBingoText") {
            scoreBingoText = value;
            unescape(scoreBingoText);
        } else if (key == "ScoreNameWinText") {
            scoreNameWinText = value;
            unescape(scoreNameWinText);
        } else if (key == "ScoreNameBingoText") {
            scoreNameBingoText = value;
            unescape(scoreNameBingoText);
        } else if (key == "FirstScores") {
            auto sl = splitString(value, ',');
            int sz = (int)sl.size();
            if (sz > 5) sz = 5;
            int i;
            for (i = 0; i < sz; i++) {
                scores[i] = std::stoi(sl[i]);
            }
            if (i == 0) {
                scores[0] = 2;
                i = 1;
            }
            for (; i < 5; i++) {
                scores[i] = scores[i - 1];
            }
        } else if (key == "NoneFirstScores") {
            auto sl = splitString(value, ',');
            int sz = (int)sl.size();
            if (sz > 5) sz = 5;
            int i;
            for (i = 0; i < sz; i++) {
                nFScores[i] = std::stoi(sl[i]);
            }
            if (i == 0) {
                nFScores[0] = 1;
                i = 1;
            }
            for (; i < 5; i++) {
                nFScores[i] = nFScores[i - 1];
            }
        } else if (key == "LineScore") {
            lineScore = std::stoi(value);
        } else if (key == "MaxPerRow") {
            maxPerRow = std::stoi(value);
        } else if (key == "ClearScore") {
            clearScore = std::stoi(value);
        } else if (key == "ClearQuestMultiplier") {
            clearQuestMultiplier = std::stoi(value);
        } else if (key == "Player1") {
            playerName[0] = Utf8ToUnicode(value);
        } else if (key == "Player2") {
            playerName[1] = Utf8ToUnicode(value);
        }
    }
    for (int i = 0; i < 3; i++) {
        auto &c = colors[i];
        c.r = (float)colorsInt[i].r / 255.f;
        c.g = (float)colorsInt[i].g / 255.f;
        c.b = (float)colorsInt[i].b / 255.f;
        if (i > 0)
            colorsInt[i].a = colorsInt[0].a;
        c.a = (float)colorsInt[i].a / 255.f;
    }
    if (bingoBrawlersMode) {
        maxPerRow = 5;
        clearQuestMultiplier = 0;
        for (int i = 0; i < 5; i++) {
            scores[i] = 1;
            nFScores[i] = 0;
        }
    }
}

void Config::postLoad() {
    if (font) {
        TTF_CloseFont(font);
        font = nullptr;
    }
    font = TTF_OpenFont(fontFile.c_str(), fontSize);
    TTF_SetFontStyle(font, fontStyle);
    TTF_SetFontWrappedAlign(font, TTF_WRAPPED_ALIGN_CENTER);
    if (scoreFont) {
        TTF_CloseFont(scoreFont);
        scoreFont = nullptr;
    }
    scoreFont = TTF_OpenFont(scoreFontFile.c_str(), scoreFontSize);
    TTF_SetFontStyle(scoreFont, scoreFontStyle);
    TTF_SetFontWrappedAlign(scoreFont, TTF_WRAPPED_ALIGN_CENTER);
    if (scoreNameFont) {
        TTF_CloseFont(scoreNameFont);
        scoreNameFont = nullptr;
    }
    scoreNameFont = TTF_OpenFont(scoreNameFontFile.c_str(), scoreNameFontSize);
    TTF_SetFontStyle(scoreNameFont, scoreNameFontStyle);
    TTF_SetFontWrappedAlign(scoreNameFont, TTF_WRAPPED_ALIGN_CENTER);
}
