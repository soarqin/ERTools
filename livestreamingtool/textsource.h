#pragma once

#define WIN32_LEAN_AND_MEAN
#include <SDL3/SDL.h>
#include <windows.h>
#include <combaseapi.h>
#include <gdiplus.h>
#include <string>
#include <memory>
#undef WIN32_LEAN_AND_MEAN

using namespace Gdiplus;

template<typename T, typename T2, BOOL WINAPI deleter(T2)> class GDIObj {
    T obj = nullptr;

    inline GDIObj &Replace(T obj_)
    {
        if (obj)
            deleter(obj);
        obj = obj_;
        return *this;
    }

public:
    inline GDIObj() = default;
    inline explicit GDIObj(T obj_) : obj(obj_) {}
    inline ~GDIObj() { deleter(obj); }

    inline T operator=(T obj_)
    {
        Replace(obj_);
        return obj;
    }

    inline operator T() const { return obj; }

    inline bool operator==(T obj_) const { return obj == obj_; }
    inline bool operator!=(T obj_) const { return obj != obj_; }
};

using HDCObj = GDIObj<HDC, HDC, DeleteDC>;

enum class Align {
    Left,
    Center,
    Right,
};

enum class VAlign {
    Top,
    Center,
    Bottom,
};

enum class ShadowMode {
    None,
    Outline,
    Shadow,
};

struct TextSettings {
    TextSettings();
    ~TextSettings();
    void updateFont();
    void getStringFormat(StringFormat &format) const;
    void removeNewlinePadding(const StringFormat &format, RectF &box) const;

    HDCObj hdc;
    Graphics graphics;

    std::unique_ptr<Font> font;

    std::wstring face;
    int face_size = 0;
    uint32_t color = 0xFFFFFF;
    Align align = Align::Left;
    VAlign valign = VAlign::Top;
    bool bold = false;
    bool italic = false;
    bool antialiasing = true;
    bool vertical = false;

    ShadowMode shadow_mode = ShadowMode::None;
    float shadow_size = 1.f;
    float shadow_offset[2] = {0.f, 0.f};
    uint32_t shadow_color = 0;
};

struct TextSource {
    TextSource() = default;
    inline explicit TextSource(TextSettings *s): settings(s) {}
    ~TextSource();

    void calculateTextSizes(const StringFormat &format, RectF &bounding_box,
                            SIZE &text_size) const;
    void renderTextWithShadow(Graphics &graphics, const GraphicsPath &path,
                              const Brush &brush, float width) const;
    void renderText();
    void setAntiAliasing(Graphics &graphics_bitmap) const;

    void update();

    SDL_Surface *surface = nullptr;

    TextSettings *settings = nullptr;

    uint32_t cx = 0;
    uint32_t cy = 0;

    std::wstring text;

    bool use_extents = false;
    bool wrap = false;
    uint32_t extents_cx = 0;
    uint32_t extents_cy = 0;
};
