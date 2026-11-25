// openTIT - GUI tickers: labels whose single text line scrolls and wraps.
#include "ape/level/ApeGuiTypes.h"

#include "ape/level/ApeGuiCreate.h"
#include "platform/Platform.h"

#include <algorithm>
#include <cmath>

namespace ape {

namespace {

constexpr uint32_t kAttrScrollSpeed = 0xFC87E57A;   // "scrollspeed"
constexpr uint32_t kAttrWidth = 0xB75E2EC9;         // NOTE: name unknown
constexpr uint32_t kAttrGap = 0x77B9554D;           // NOTE: name unknown
constexpr uint32_t kAttrStartOffset = 0x911A2AA2;   // NOTE: name unknown
constexpr uint32_t kAttrScroll = 0x280D85C8;        // NOTE: name unknown

// The ticker attributes; defaults from 'source' when a ticker is cloned.
// @func 0x004DCA08 ReadGuiTickerDesc
void ReadGuiTickerDesc(xml::XmlDocument* doc, xml::XmlElement* element, ApeGuiObject* source, GuiTickerDesc& desc)
{
    ReadGuiLabelDesc(doc, element, source, desc);
    if (!desc.font)
        return;
    float defaultSpeed = 1.0f;
    float defaultWidth = 0.0f;
    float defaultGap = 0.0f;
    bool defaultScroll = false;
    if (source) {
        if (source->GetType() == kGuiObject_Type5) {
            const auto* ticker = static_cast<const ApeGuiTicker*>(source);
            defaultSpeed = ticker->scrollSpeed;
            defaultWidth = ticker->width;
            defaultGap = ticker->gap;
            defaultScroll = ticker->scrollEnabled;
        } else {
            plat::Log(plat::LogLevel::Warning, "%s: GUI clone source of a different class\n",
                      doc ? doc->GetDocumentPath() : "");
        }
    }
    element->GetFloat(kAttrScrollSpeed, &desc.scrollSpeed, defaultSpeed);
    element->GetFloat(kAttrWidth, &desc.width, defaultWidth);
    element->GetFloat(kAttrGap, &desc.gap, defaultGap);
    element->GetFloat(kAttrStartOffset, &desc.startOffset, 0.0f);
    element->GetBool(kAttrScroll, &desc.scroll, defaultScroll);
    desc.scrollSpeed = std::max(desc.scrollSpeed, 0.0f);
    desc.width = std::max(desc.width, 0.0f);
    desc.gap = std::max(desc.gap, 0.0f);
}

} // namespace

// @func 0x004DCC04 CreateGuiTicker
ApeGuiObject* CreateGuiTicker(xml::XmlDocument* doc, xml::XmlElement* element, const GuiObjectDesc& base,
                              ApeGuiObject* source)
{
    GuiTickerDesc desc;
    static_cast<GuiObjectDesc&>(desc) = base;
    ReadGuiTickerDesc(doc, element, source, desc);
    if (!desc.font)
        return nullptr;
    return new ApeGuiTicker(desc);
}

// @func 0x004DC5E0 ApeGuiTicker::ApeGuiTicker
// Without a width an expanding ticker takes its parent's; the width never
// exceeds the layout width.
ApeGuiTicker::ApeGuiTicker(const GuiTickerDesc& desc) : ApeGuiLabel(desc)
{
    scrollSpeed = desc.scrollSpeed;
    width = desc.width;
    gap = desc.gap;
    scrollOffset = 0.0f;
    startOffset = desc.startOffset;
    scrolling = desc.scroll;
    scrollEnabled = desc.scroll;
    if (width == 0.0f && HasFlag(kFlag_Expand) && parent) {
        float parentSize[2];
        parent->GetLayoutSize(parentSize);
        width = parentSize[0];
    }
    float size[2];
    ApeGuiTicker::GetLayoutSize(size);
    if (size[0] < width)
        width = size[0];
}

// @func 0x004E6D3C ApeGuiTicker::Clone
ApeGuiObject* ApeGuiTicker::Clone(const char* newName, ApeGuiObject* newParent, void* newOwner,
                                  bool recursive) const
{
    GuiTickerDesc desc;
    FillCloneDesc(desc, newName, newParent, newOwner);
    desc.font = font;
    desc.text = text.c_str();
    desc.leading = leading;
    desc.tracking = tracking;
    desc.wrapWidth = wrapWidth;
    desc.field_94 = field_94;
    desc.fixedWidth = fixedWidth;
    desc.animMode = animMode;
    desc.animRate = 1.0f / animPeriod;
    desc.layoutNow = false;
    desc.scrollSpeed = scrollSpeed;
    desc.width = width;
    desc.gap = gap;
    desc.startOffset = startOffset;
    desc.scroll = scrollEnabled;
    auto* copy = new ApeGuiTicker(desc);
    if (recursive)
        CloneChildren(copy, newOwner);
    return copy;
}

// @func 0x004DA4C0 ApeGuiTicker::GetLayoutSize
// One line: the text width (or the parent's width when expanding, if
// larger) plus the gap; line breaks are ignored.
void ApeGuiTicker::GetLayoutSize(float out[2]) const
{
    if (HasFlag(kFlag_Dirty)) {
        float textWidth = 0.0f;
        float height = 0.0f;
        if (font && !text.empty()) {
            const Vec3 worldScale = GetWorldScale();
            const float space = GetSpaceWidth(worldScale);
            const uint32_t limit = GetVisibleGlyphLimit();
            float lineWidth = 0.0f;
            uint32_t counted = 0;
            const char* p = text.c_str();
            while (*p && counted < limit) {
                const uint32_t c = DecodeGuiChar(p, nullptr, true);
                if (c == '\n' || c == '\r')
                    continue;
                uint32_t fontIndex, glyphIndex;
                if (!fixedWidth && c != ' ' && font->FindGlyph(c, fontIndex, glyphIndex)) {
                    ++counted;
                    lineWidth += (float)font->fonts[fontIndex]->glyphs[glyphIndex].width * font->fontScales[fontIndex] *
                                 font->scale[0] * worldScale.x +
                                 space * tracking;
                } else {
                    lineWidth += space;
                }
                textWidth = std::max(textWidth, lineWidth);
            }
            height = font->GetLineHeight() * worldScale.y;
            if (HasFlag(kFlag_Expand)) {
                float expandSize[2];
                GetGuiExpandSize(this, expandSize);
                textWidth = std::max(expandSize[0], textWidth);
                height = expandSize[1];
            }
        }
        layoutSize[0] = textWidth + gap;
        layoutSize[1] = height;
        flags &= ~kFlag_Dirty;
        SetLayoutDirty();
        width = layoutSize[0];
    }
    out[0] = layoutSize[0];
    out[1] = layoutSize[1];
}

// @func 0x004E7A58 ApeGuiTicker::Invalidate
void ApeGuiTicker::Invalidate()
{
    revealTime = 0.0f;
    scrollOffset = startOffset;
    layoutDirty = true;
    CountGlyphs();
    ApeGuiObject::Invalidate();
}

// @func 0x004E70D0 ApeGuiTicker::Tick
// Rebuilds the glyphs of the visible part of the line: glyphs scrolled out
// on the left reappear after the end of the text (at the wrap position),
// glyphs past the width are skipped. Then advances the scroll position.
void ApeGuiTicker::Tick(float dt)
{
    if (IsVisible() && font && !text.empty() && layoutDirty) {
        const Vec3 worldScale = GetWorldScale();
        float size[2];
        GetLayoutSize(size);
        const float space = GetSpaceWidth(worldScale);
        float x = size[0] * -0.5f * (float)((flags >> 14) & 3) - scrollOffset;
        float wrapX = x + size[0];
        const float lineHeight = font->GetLineHeight() * worldScale.y;
        const float baseline = lineHeight * 3.0f * 0.25f;
        const float top = lineHeight * leading * -0.5f * (float)((flags >> 18) & 3);
        const uint32_t limit = GetVisibleGlyphLimit();

        uint32_t count = 0;
        const char* p = text.c_str();
        while (*p && count < limit) {
            const uint32_t c = DecodeGuiChar(p, nullptr, true);
            if (c == '\n' || c == '\r')
                continue;
            uint32_t fontIndex, glyphIndex;
            if (c == ' ' || !font->FindGlyph(c, fontIndex, glyphIndex)) {
                const float advance = space * tracking + space;
                x += advance;
                if (x < 0.0f)
                    wrapX += advance;
                continue;
            }
            const ApeGuiBitmapFont* bitmapFont = font->fonts[fontIndex];
            const GuiGlyph& source = bitmapFont->glyphs[glyphIndex];
            const float fontScale = font->fontScales[fontIndex];
            // NOTE: fixed width glyphs are centred with their unscaled width.
            float glyphX = fixedWidth ? (space - (float)source.width) * 0.5f + x : x;
            const float glyphWidth = (float)source.width * fontScale * font->scale[0] * worldScale.x;
            const float advance = space * tracking + (fixedWidth ? space : glyphWidth);
            x += advance;
            if (x < 0.0f) {
                // Scrolled out: place it after the end of the text.
                const float wrapped = advance + wrapX;
                if (wrapped < 0.0f || width < wrapX)
                    break;
                glyphX = wrapX;
                wrapX = wrapped;
            } else if (width < glyphX) {
                continue;
            }

            GuiLabelGlyph& glyph = glyphs[count++];
            glyph.material = bitmapFont->materials[GetBlendMode()];
            glyph.untinted = bitmapFont->flag_4704F4C;
            glyph.x = glyphX;
            glyph.y = baseline + top - (float)source.top * fontScale * font->scale[1] * worldScale.y;
            glyph.width = glyphWidth;
            glyph.height = (float)(source.top + source.bottom + 1) * fontScale * font->scale[1] * worldScale.y;
            glyph.texX = source.x;
            glyph.texY = source.y;
            glyph.texWidth = source.width;
            glyph.texHeight = (uint16_t)(source.top + source.bottom + 1);
        }
        for (uint32_t i = count; i < glyphs.size(); ++i)
            glyphs[i].material = nullptr;

        layoutDirty = false;
        progress[0] = (float)count;
        progress[1] = 1.0f;
        if (revealInterval > 0.0f) {
            flags |= kFlag_Dirty;
            revealTime += dt;
        }
        if (scrolling && scrollSpeed > 0.0f) {
            scrollOffset += dt * scrollSpeed;
            if (layoutSize[0] <= scrollOffset) {
                const float period = std::fabs(layoutSize[0]);
                const float cycles = scrollOffset / period;
                scrollOffset = period * (cycles - std::trunc(cycles));
            }
            layoutDirty = true;
        }
    }
    ApeGuiObject::Tick(dt);
}

} // namespace ape
