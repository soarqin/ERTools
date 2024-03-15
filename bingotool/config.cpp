#include "config.h"

#include "common.h"

#include <toml.hpp>

#include <fstream>
#include <algorithm>

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
    oldLoad();

    auto stringToColor = [](const std::string &s, SDL_Color &c) {
        auto sl = splitString(s, ',');
        if (sl.size() == 4) {
            c.r = std::stoi(sl[0]);
            c.g = std::stoi(sl[1]);
            c.b = std::stoi(sl[2]);
            c.a = std::stoi(sl[3]);
        }
    };
    auto extractColor = [&stringToColor](const toml::value &parent, const char *key, SDL_Color &c) {
        auto s = toml::find_or(parent, key, "");
        stringToColor(s, c);
    };
    auto extractOffset = [](const toml::value &parent, const char *key, int &x, int &y) {
        const auto &v = toml::find_or(parent, key, toml::value());
        if (v.is_array() && v.size() >= 2) {
            x = toml::get_or(v.at(0), x);
            y = toml::get_or(v.at(1), y);
        }
    };

    toml::value data;
    try {
        data = toml::parse("config.toml");
    } catch (...) {
        return;
    }
    const auto cells = toml::find_or(data, "cells_window", toml::value());
    if (cells.is_table()) {
        originCellSizeX = cellSize[0] = toml::find_or(cells, "width", originCellSizeX);
        cellSize[1] = toml::find_or(cells, "height", originCellSizeX);
        cellSpacing = toml::find_or(cells, "spacing", cellSpacing);
        cellBorder = toml::find_or(cells, "border", cellBorder);
        extractColor(cells, "spacing_color", cellSpacingColor);
        extractColor(cells, "border_color", cellBorderColor);
        cellAutoFit = toml::find_or(cells, "auto_fit", cellAutoFit);
        extractColor(cells, "text_color", textColor);
        textShadow = toml::find_or(cells, "text_shadow", textShadow);
        extractColor(cells, "text_shadow_color", textShadowColor);
        extractOffset(cells, "text_shadow_offset", textShadowOffset[0], textShadowOffset[1]);
        fontFile = toml::find_or(cells, "font_file", fontFile);
        auto style = toml::find_or(cells, "font_style", "-");
        if (style != "-") fontStyle = setFontStyle(style);
        originalFontSize = fontSize = toml::find_or(cells, "font_size", originalFontSize);
        extractColor(cells, "cell_color", colorsInt[0]);
        auto color1 = toml::find_or(cells, "color1", "");
        auto sl = splitString(color1, ',');
        if (sl.size() == 1) {
            useColorTexture[0] = true;
            colorTextureFile[0] = color1;
        } else {
            useColorTexture[0] = false;
            stringToColor(color1, colorsInt[1]);
        }
        auto color2 = toml::find_or(cells, "color2", "");
        sl = splitString(color2, ',');
        if (sl.size() == 1) {
            useColorTexture[1] = true;
            colorTextureFile[1] = color2;
        } else {
            useColorTexture[1] = false;
            stringToColor(color2, colorsInt[2]);
        }
        auto uctv = toml::find_or(cells, "use_color_texture", toml::value());
        if (uctv.is_array() && uctv.size() >= 2) {
            useColorTexture[0] = toml::get_or(uctv.at(0), useColorTexture[0]);
            useColorTexture[1] = toml::get_or(uctv.at(1), useColorTexture[1]);
        }
        auto ctv = toml::find_or(cells, "color_texture", toml::value());
        if (ctv.is_array() && ctv.size() >= 2) {
            colorTextureFile[0] = toml::get_or(ctv.at(0), colorTextureFile[0]);
            colorTextureFile[1] = toml::get_or(ctv.at(1), colorTextureFile[1]);
        }
    }
    const auto score = toml::find_or(data, "scores_window", toml::value());
    if (score.is_table()) {
        playerName[0] = Utf8ToUnicode(toml::find_or(score, "player1", UnicodeToUtf8(playerName[0])));
        playerName[1] = Utf8ToUnicode(toml::find_or(score, "player2", UnicodeToUtf8(playerName[1])));
        scoreFontFile = toml::find_or(score, "font_file", scoreFontFile);
        auto style = toml::find_or(score, "font_style", "-");
        if (style != "-") scoreFontStyle = setFontStyle(style);
        scoreFontSize = toml::find_or(score, "font_size", scoreFontSize);
        scoreTextShadow = toml::find_or(score, "text_shadow", scoreTextShadow);
        extractColor(score, "text_shadow_color", scoreTextShadowColor);
        extractOffset(score, "text_shadow_offset", scoreTextShadowOffset[0], scoreTextShadowOffset[1]);
        scoreNameFontFile = toml::find_or(score, "name_font_file", scoreNameFontFile);
        style = toml::find_or(score, "name_font_style", "-");
        if (style != "-") scoreNameFontStyle = setFontStyle(style);
        scoreNameFontSize = toml::find_or(score, "name_font_size", scoreNameFontSize);
        scoreNameTextShadow = toml::find_or(score, "name_text_shadow", scoreNameTextShadow);
        extractColor(score, "name_text_shadow_color", scoreNameTextShadowColor);
        extractOffset(score, "name_text_shadow_offset", scoreNameTextShadowOffset[0], scoreNameTextShadowOffset[1]);
        extractColor(score, "background_color", scoreBackgroundColor);
        scorePadding = toml::find_or(score, "padding", scorePadding);
        scoreRoundCorner = toml::find_or(score, "round_corner", scoreRoundCorner);
        scoreWinText = toml::find_or(score, "win_text", scoreWinText);
        scoreBingoText = toml::find_or(score, "bingo_text", scoreBingoText);
        scoreNameWinText = toml::find_or(score, "name_win_text", scoreNameWinText);
        scoreNameBingoText = toml::find_or(score, "name_bingo_text", scoreNameBingoText);
    }
    const auto rules = toml::find_or(data, "rules", toml::value());
    if (rules.is_table()) {
        bingoBrawlersMode = toml::find_or(rules, "bingo_brawlers_mode", bingoBrawlersMode);
        const auto scr = toml::find_or(rules, "scores", toml::value());
        if (scr.is_array()) {
            for (size_t i = 0; i < scr.size() && i < 5; i++) {
                this->scores[i] = toml::get<int>(scr.at(i));
            }
        }
        const auto nfs = toml::find_or(rules, "none_first_scores", toml::value());
        if (nfs.is_array()) {
            for (size_t i = 0; i < nfs.size() && i < 5; i++) {
                this->nFScores[i] = toml::get<int>(nfs.at(i));
            }
        }
        lineScore = toml::find_or(rules, "line_score", lineScore);
        maxPerRow = toml::find_or(rules, "max_per_row", maxPerRow);
        clearScore = toml::find_or(rules, "clear_score", clearScore);
        clearQuestMultiplier = toml::find_or(rules, "clear_quest_multiplier", clearQuestMultiplier);
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
}

void Config::save() {
    auto colorToString = [](const SDL_Color &c) {
        return std::to_string(c.r) + "," + std::to_string(c.g) + "," + std::to_string(c.b) + "," + std::to_string(c.a);
    };
    toml::value data = {
        {
            "cells_window",
            {
                {"width", originCellSizeX},
                {"height", cellSize[1]},
                {"spacing", cellSpacing},
                {"border", cellBorder},
                {"spacing_color", colorToString(cellSpacingColor)},
                {"border_color", colorToString(cellBorderColor)},
                {"auto_fit", cellAutoFit},
                {"text_color", colorToString(textColor)},
                {"text_shadow", textShadow},
                {"text_shadow_color", colorToString(textShadowColor)},
                {"text_shadow_offset", {textShadowOffset[0], textShadowOffset[1]}},
                {"font_file", fontFile},
                {"font_style", std::string(fontStyle & TTF_STYLE_BOLD ? "B" : "") + (fontStyle & TTF_STYLE_ITALIC ? "I" : "")},
                {"font_size", originalFontSize},
                {"cell_color", colorToString(colorsInt[0])},
                {"color1", colorToString(colorsInt[1])},
                {"color2", colorToString(colorsInt[2])},
                {"use_color_texture", {useColorTexture[0], useColorTexture[1]}},
                {"color_texture", {colorTextureFile[0], colorTextureFile[1]}},
            }
        },
        {
            "scores_window",
            {
                {"player1", UnicodeToUtf8(playerName[0])},
                {"player2", UnicodeToUtf8(playerName[1])},
                {"font_file", scoreFontFile},
                {"font_style", std::string(scoreFontStyle & TTF_STYLE_BOLD ? "B" : "") + (scoreFontStyle & TTF_STYLE_ITALIC ? "I" : "")},
                {"font_size", scoreFontSize},
                {"text_shadow", scoreTextShadow},
                {"text_shadow_color", colorToString(scoreTextShadowColor)},
                {"text_shadow_offset", {scoreTextShadowOffset[0], scoreTextShadowOffset[1]}},
                {"name_font_file", scoreNameFontFile},
                {"name_font_style", std::string(scoreNameFontStyle & TTF_STYLE_BOLD ? "B" : "") + (scoreNameFontStyle & TTF_STYLE_ITALIC ? "I" : "")},
                {"name_font_size", scoreNameFontSize},
                {"name_text_shadow", scoreNameTextShadow},
                {"name_text_shadow_color", colorToString(scoreNameTextShadowColor)},
                {"name_text_shadow_offset", {scoreNameTextShadowOffset[0], scoreNameTextShadowOffset[1]}},
                {"background_color", colorToString(scoreBackgroundColor)},
                {"padding", scorePadding},
                {"round_corner", scoreRoundCorner},
                {"win_text", scoreWinText},
                {"bingo_text", scoreBingoText},
                {"name_win_text", scoreNameWinText},
                {"name_bingo_text", scoreNameBingoText},
            }
        },
        {
            "rules",
            {
                {"bingo_brawlers_mode", bingoBrawlersMode},
                {"scores", {scores[0], scores[1], scores[2], scores[3], scores[4]}},
                {"none_first_scores", {nFScores[0], nFScores[1], nFScores[2], nFScores[3], nFScores[4]}},
                {"line_score", lineScore},
                {"max_per_row", maxPerRow},
                {"clear_score", clearScore},
                {"clear_quest_multiplier", clearQuestMultiplier},
            }
        },
    };
    std::ofstream file("config.toml");
    if (file.is_open())
        file << toml::format(data);
}

void Config::oldLoad() {
    std::ifstream ifs("data/config.txt");
    if (!ifs.is_open()) return;
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
                    originCellSizeX = cellSize[0] = cellSize[1] = std::stoi(sl[0]);
                    break;
                case 2:
                    originCellSizeX = cellSize[0] = std::stoi(sl[0]);
                    cellSize[1] = std::stoi(sl[1]);
                    break;
                default:
                    break;
            }
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
        } else if (key == "CellAutoFit") {
            cellAutoFit = std::clamp(std::stoi(value), 0, 2);
        } else if (key == "FontSize") {
            originalFontSize = fontSize = std::stoi(value);
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
                useColorTexture[0] = false;
                colorTextureFile[0].clear();
            } else if (sl.size() == 1) {
                useColorTexture[0] = true;
                colorTextureFile[0] = sl[0];
            }
        } else if (key == "Color2") {
            auto sl = splitString(value, ',');
            if (sl.size() == 3) {
                colorsInt[2].r = std::stoi(sl[0]);
                colorsInt[2].g = std::stoi(sl[1]);
                colorsInt[2].b = std::stoi(sl[2]);
                useColorTexture[1] = false;
                colorTextureFile[1].clear();
            } else if (sl.size() == 1) {
                useColorTexture[1] = true;
                colorTextureFile[1] = sl[0];
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
