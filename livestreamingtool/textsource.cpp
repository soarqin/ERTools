#include "textsource.h"

#include <cmath>
#if defined(max)
#undef max
#endif
#if defined(min)
#undef min
#endif

using namespace Gdiplus;

static const auto EPSILON = std::numeric_limits<REAL>::epsilon();

#ifndef clamp
#define clamp(val, min_val, max_val) \
    if (val < min_val)           \
        val = min_val;       \
    else if (val > max_val)      \
        val = max_val;
#endif

#define MIN_SIZE_CX 2
#define MIN_SIZE_CY 2
#define MAX_SIZE_CX 16384
#define MAX_SIZE_CY 16384

#define MAX_AREA (4096LL * 4096LL)

TextSettings::TextSettings() :
    hdc(CreateCompatibleDC(nullptr)),
    graphics(hdc) {
}

TextSettings::~TextSettings() {
    if (hdc) {
        DeleteDC(hdc);
        hdc = nullptr;
    }
}

void TextSettings::updateFont() {
    font.reset(nullptr);

    LOGFONTW lf = {};
    lf.lfHeight = -MulDiv(face_size, GetDeviceCaps(GetDC(nullptr), LOGPIXELSY), 72);
    lf.lfWeight = bold ? FW_BOLD : FW_DONTCARE;
    lf.lfItalic = italic;
    lf.lfQuality = CLEARTYPE_NATURAL_QUALITY;
    lf.lfCharSet = DEFAULT_CHARSET;

    if (!face.empty()) {
        lstrcpyW(lf.lfFaceName, face.c_str());
    }
    font = std::make_unique<Font>(hdc, &lf);

    if (!font->IsAvailable()) {
        wcscpy(lf.lfFaceName, L"Arial");
        font = std::make_unique<Font>(hdc, &lf);
    }
}

TextSource::~TextSource() {
    if (surface) {
        SDL_DestroySurface(surface);
        surface = nullptr;
    }
}

void TextSettings::getStringFormat(StringFormat &format) const {
    INT flags = StringFormatFlagsNoFitBlackBox |
        StringFormatFlagsMeasureTrailingSpaces;

    if (vertical)
        flags |= StringFormatFlagsDirectionVertical |
            StringFormatFlagsDirectionRightToLeft;

    format.SetFormatFlags(flags);
    format.SetTrimming(StringTrimmingWord);

    switch (align) {
        case Align::Left:
            if (vertical)
                format.SetLineAlignment(StringAlignmentFar);
            else
                format.SetAlignment(StringAlignmentNear);
            break;
        case Align::Center:
            if (vertical)
                format.SetLineAlignment(StringAlignmentCenter);
            else
                format.SetAlignment(StringAlignmentCenter);
            break;
        case Align::Right:
            if (vertical)
                format.SetLineAlignment(StringAlignmentNear);
            else
                format.SetAlignment(StringAlignmentFar);
    }

    switch (valign) {
        case VAlign::Top:
            if (vertical)
                format.SetAlignment(StringAlignmentNear);
            else
                format.SetLineAlignment(StringAlignmentNear);
            break;
        case VAlign::Center:
            if (vertical)
                format.SetAlignment(StringAlignmentCenter);
            else
                format.SetLineAlignment(StringAlignmentCenter);
            break;
        case VAlign::Bottom:
            if (vertical)
                format.SetAlignment(StringAlignmentFar);
            else
                format.SetLineAlignment(StringAlignmentFar);
    }
}

/* GDI+ treats '\n' as an extra character with an actual render size when
 * calculating the texture size, so we have to calculate the size of '\n' to
 * remove the padding.  Because we always add a newline to the string, we
 * also remove the extra unused newline. */
void TextSettings::removeNewlinePadding(const StringFormat &format, RectF &box) const {
    RectF before;
    RectF after;

    graphics.MeasureString(L"W", -1, font.get(), PointF(0.0f, 0.0f),
                           &format, &before);

    graphics.MeasureString(L"W\n", -1, font.get(), PointF(0.0f, 0.0f),
                           &format, &after);

    float offset_cx = after.Width - before.Width;
    float offset_cy = after.Height - before.Height;

    if (!vertical) {
        if (offset_cx >= 1.0f)
            offset_cx -= 1.0f;

        if (valign == VAlign::Center)
            box.Y -= offset_cy * 0.5f;
        else if (valign == VAlign::Bottom)
            box.Y -= offset_cy;
    } else {
        if (offset_cy >= 1.0f)
            offset_cy -= 1.0f;

        if (align == Align::Center)
            box.X -= offset_cx * 0.5f;
        else if (align == Align::Right)
            box.X -= offset_cx;
    }

    box.Width -= offset_cx;
    box.Height -= offset_cy;
}

void TextSource::calculateTextSizes(const StringFormat &format,
                                    RectF &bounding_box, SIZE &text_size) const {
    RectF layout_box;
    RectF temp_box;

    if (!text.empty()) {
        if (use_extents && wrap) {
            layout_box.X = layout_box.Y = 0;
            layout_box.Width = float(extents_cx);
            layout_box.Height = float(extents_cy);

            switch (settings->shadow_mode) {
                case ShadowMode::None:
                    break;
                case ShadowMode::Outline:
                    layout_box.Width -= settings->shadow_size * 2.f;
                    layout_box.Height -= settings->shadow_size * 2.f;
                    break;
                case ShadowMode::Shadow:
                    layout_box.Width -= std::abs(settings->shadow_offset[0]) + std::max(settings->shadow_size - 1.f, 0.f);
                    layout_box.Height -= std::abs(settings->shadow_offset[1]) + std::max(settings->shadow_size - 1.f, 0.f);
                    break;
            }

            settings->graphics.MeasureString(text.c_str(),
                                             -1,
                                             settings->font.get(), layout_box,
                                             &format, &bounding_box);

            temp_box = bounding_box;
            settings->removeNewlinePadding(format, bounding_box);
        } else {
            settings->graphics.MeasureString(
                text.c_str(), -1, settings->font.get(),
                PointF(0.0f, 0.0f), &format, &bounding_box);
            temp_box = bounding_box;

            bounding_box.X = 0.0f;
            bounding_box.Y = 0.0f;

            settings->removeNewlinePadding(format, bounding_box);

            switch (settings->shadow_mode) {
                case ShadowMode::None:
                    break;
                case ShadowMode::Outline:
                    bounding_box.Width += settings->shadow_size * 2.f;
                    bounding_box.Height += settings->shadow_size * 2.f;
                    break;
                case ShadowMode::Shadow:
                    bounding_box.Width += std::abs(settings->shadow_offset[0]) + std::max(settings->shadow_size - 1.f, 0.f);
                    bounding_box.Height += std::abs(settings->shadow_offset[1]) + std::max(settings->shadow_size - 1.f, 0.f);
                    break;
            }
        }
    }

    if (settings->vertical) {
        if (bounding_box.Width < settings->face_size) {
            text_size.cx = settings->face_size;
            bounding_box.Width = float(settings->face_size);
        } else {
            text_size.cx = LONG(bounding_box.Width + EPSILON);
        }

        text_size.cy = LONG(bounding_box.Height + EPSILON);
    } else {
        if (bounding_box.Height < settings->face_size) {
            text_size.cy = settings->face_size;
            bounding_box.Height = float(settings->face_size);
        } else {
            text_size.cy = LONG(bounding_box.Height + EPSILON);
        }

        text_size.cx = LONG(bounding_box.Width + EPSILON);
    }

    if (use_extents) {
        text_size.cx = extents_cx;
        text_size.cy = extents_cy;
    }

    text_size.cx += text_size.cx % 2;
    text_size.cy += text_size.cy % 2;

    int64_t total_size = int64_t(text_size.cx) * int64_t(text_size.cy);

    /* GPUs typically have texture size limitations */
    clamp(text_size.cx, MIN_SIZE_CX, MAX_SIZE_CX);
    clamp(text_size.cy, MIN_SIZE_CY, MAX_SIZE_CY);

    /* avoid taking up too much VRAM */
    if (total_size > MAX_AREA) {
        if (text_size.cx > text_size.cy)
            text_size.cx = (LONG)MAX_AREA / text_size.cy;
        else
            text_size.cy = (LONG)MAX_AREA / text_size.cx;
    }

    /* the internal text-rendering bounding box for is reset to
     * its internal value in case the texture gets cut off */
    bounding_box.Width = temp_box.Width;
    bounding_box.Height = temp_box.Height;
}

void TextSource::renderTextWithShadow(Graphics &graphics, const GraphicsPath &path,
                                      const Brush &brush, float width) const {
    Pen pen(Color(settings->shadow_color), width);
    pen.SetLineJoin(LineJoinRound);

    graphics.DrawPath(&pen, &path);

    graphics.FillPath(&brush, &path);
}

void TextSource::renderText() {
    StringFormat format(StringFormat::GenericTypographic());

    RectF box;
    SIZE size;

    settings->getStringFormat(format);
    calculateTextSizes(format, box, size);

    std::unique_ptr<uint8_t[]> bits(new uint8_t[size.cx * size.cy * 4]);
    Bitmap bitmap(size.cx, size.cy, 4 * size.cx, PixelFormat32bppARGB,
                  bits.get());

    Graphics graphics_bitmap(&bitmap);
    SolidBrush brush((Color(settings->color)));

    graphics_bitmap.Clear(Color(0));

    graphics_bitmap.SetCompositingMode(CompositingModeSourceOver);
    setAntiAliasing(graphics_bitmap);

    if (!text.empty()) {
        switch (settings->shadow_mode) {
            case ShadowMode::None:
                graphics_bitmap.DrawString(text.c_str(),
                                           (int)text.size(),
                                           settings->font.get(), box,
                                           &format, &brush);
                break;
            case ShadowMode::Outline: {
                box.Offset(settings->shadow_size, settings->shadow_size);

                FontFamily family;
                GraphicsPath path;

                settings->font->GetFamily(&family);
                path.AddString(text.c_str(), (int)text.size(),
                               &family, settings->font->GetStyle(),
                               settings->font->GetSize(), box, &format);

                renderTextWithShadow(graphics_bitmap, path, brush, settings->shadow_size * 2);
                break;
            }
            case ShadowMode::Shadow:
                if (settings->shadow_offset[0] != 0.f || settings->shadow_offset[1] != 0.f) {
                    SolidBrush bkbrush((Color(settings->shadow_color)));
                    auto bkbox = box;
                    if (settings->shadow_offset[0] > 0)
                        bkbox.X += settings->shadow_offset[0];
                    else
                        box.X -= settings->shadow_offset[0];
                    if (settings->shadow_offset[1] > 0)
                        bkbox.Y += settings->shadow_offset[1];
                    else
                        box.Y -= settings->shadow_offset[1];
                    if (settings->shadow_size > 0.5f) {
                        FontFamily family;
                        GraphicsPath path;

                        settings->font->GetFamily(&family);
                        path.AddString(text.c_str(), (int)text.size(),
                                       &family, settings->font->GetStyle(),
                                       settings->font->GetSize(), bkbox, &format);

                        renderTextWithShadow(graphics_bitmap, path, bkbrush, settings->shadow_size);
                    } else {
                        graphics_bitmap.DrawString(text.c_str(),
                                                   (int)text.size(),
                                                   settings->font.get(), bkbox,
                                                   &format, &bkbrush);
                    }
                }
                graphics_bitmap.DrawString(text.c_str(),
                                           (int)text.size(),
                                           settings->font.get(), box,
                                           &format, &brush);
                break;
        }
    }

    if (!surface || (LONG)cx != size.cx || (LONG)cy != size.cy) {
        if (surface) {
            SDL_DestroySurface(surface);
            surface = nullptr;
        }
        surface = SDL_CreateSurface(size.cx, size.cy, SDL_PIXELFORMAT_ARGB8888);
        cx = (uint32_t)size.cx;
        cy = (uint32_t)size.cy;
    }
    if (!surface) return;
    if (!SDL_MUSTLOCK(surface) || SDL_LockSurface(surface) == 0) {
        auto *data = (uint8_t *)surface->pixels;
        auto pitch = surface->pitch;
        int srcpitch = int(cx * 4);
        if (pitch == srcpitch) {
            memcpy(data, bits.get(), cx * cy * 4);
        } else {
            auto srcdata = bits.get();
            for (int i = 0; i < cy; i++) {
                memcpy(data, srcdata, srcpitch);
                data += pitch;
                srcdata += srcpitch;
            }
        }
        if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);
    }
}

void TextSource::setAntiAliasing(Graphics &graphics_bitmap) const {
    if (!settings->antialiasing) {
        graphics_bitmap.SetTextRenderingHint(
            TextRenderingHintSingleBitPerPixel);
        graphics_bitmap.SetSmoothingMode(SmoothingModeNone);
        return;
    }

    graphics_bitmap.SetTextRenderingHint(TextRenderingHintAntiAlias);
    graphics_bitmap.SetSmoothingMode(SmoothingModeAntiAlias);
}

#define obs_data_get_uint32 (uint32_t) obs_data_get_int

void TextSource::update() {
    if (!settings->font) settings->updateFont();

    if (!text.empty() && text.back() != '\n')
        text.push_back('\n');

    renderText();
}
