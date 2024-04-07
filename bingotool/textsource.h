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

struct TextSource {
    SDL_Surface *surface = nullptr;

    uint32_t cx = 0;
    uint32_t cy = 0;

    HDCObj hdc;
    Graphics graphics;

    std::unique_ptr<Font> font;

    std::wstring text;
    std::wstring face;
    int face_size = 0;
    uint32_t color = 0xFFFFFF;
    uint32_t color2 = 0xFFFFFF;
    float gradient_dir = 0;
    uint32_t opacity = 100;
    uint32_t opacity2 = 100;
    uint32_t bk_color = 0;
    uint32_t bk_opacity = 0;
    Align align = Align::Left;
    VAlign valign = VAlign::Top;
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool strikeout = false;
    bool antialiasing = true;
    bool vertical = false;

    bool use_outline = false;
    float outline_size = 0.0f;
    uint32_t shadow_offset[2] = {0, 0};
    uint32_t outline_color = 0;
    uint32_t outline_opacity = 100;

    bool use_extents = false;
    bool wrap = false;
    uint32_t extents_cx = 0;
    uint32_t extents_cy = 0;

    TextSource();
    ~TextSource();

    void UpdateFont();
    void GetStringFormat(StringFormat &format) const;
    void RemoveNewlinePadding(const StringFormat &format, RectF &box) const;
    void CalculateTextSizes(const StringFormat &format, RectF &bounding_box,
                            SIZE &text_size) const;
    void RenderOutlineText(Graphics &graphics, const GraphicsPath &path,
                           const Brush &brush) const;
    void RenderText();
    void SetAntiAliasing(Graphics &graphics_bitmap) const;

    void Update();
};
