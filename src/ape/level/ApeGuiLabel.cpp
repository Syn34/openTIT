// openTIT - GUI labels: text formatting, glyph layout and drawing.
//
// A label's text goes through two steps. SetText formats it: "$Name"
// references are replaced by string table entries (recursively), "\n"
// becomes a line feed and the text is wrapped at the wrap width by turning
// the last space of a line into '\r' (Japanese text may break between most
// characters; the kinsoku tables below list the characters a line must not
// start with). Layout then turns every character that a font of the font
// set has into a glyph quad. Glyphs whose font cell is followed by more
// cells are animated (one cell per frame).
#include "ape/level/ApeGuiTypes.h"

#include "agl/AglDrawList.h"
#include "agl/AglRenderTarget.h"
#include "agl/AglResources.h"
#include "agl/AglTexture.h"
#include "ape/core/ApeString.h"
#include "ape/level/ApeGuiCreate.h"
#include "ape/sys/ApePlatform.h"
#include "ape/xml/ApeStringTable.h"
#include "platform/Platform.h"

#include <algorithm>
#include <iterator>
#include <cstring>

namespace ape {

namespace {

constexpr uint32_t kGuiDrawTag = 0x475549;   // 'GUI'
constexpr size_t kFormatBufferSize = 0x1000;
constexpr int kMaxFormatDepth = 10;
constexpr uint32_t kMaxLines = 256;

int s_formatDepth = 0;   // 0x00F668CC: nesting of "$Name" expansions

// Characters a Japanese line may not be broken before (sorted; 0x00AEF78C
// and 0x00AEF6C8): opening brackets and quotes, then closing punctuation,
// small kana and prolonged sound marks.
constexpr uint32_t kNoBreakOpening[] = { 0x2018, 0x201C, 0x3008, 0x300A, 0x300C, 0x300E,
                                         0x3010, 0x3014, 0xFF08, 0xFF3B, 0xFF5B };
constexpr uint32_t kNoBreakClosing[] = {
    0x2019, 0x201D, 0x2026, 0x3001, 0x3002, 0x3005, 0x3009, 0x300B, 0x300D, 0x300F, 0x3011, 0x3015,
    0x3041, 0x3043, 0x3045, 0x3047, 0x3049, 0x3063, 0x3083, 0x3085, 0x3087, 0x308E, 0x309D, 0x309E,
    0x30A1, 0x30A3, 0x30A5, 0x30A7, 0x30A9, 0x30C3, 0x30E3, 0x30E5, 0x30E7, 0x30EE, 0x30F5, 0x30F6,
    0x30FB, 0x30FC, 0x30FD, 0x30FE, 0xFF01, 0xFF09, 0xFF0C, 0xFF0E, 0xFF1A, 0xFF1B, 0xFF1F, 0xFF3D,
    0xFF5D,
};

bool InSortedTable(const uint32_t* table, size_t count, uint32_t value)
{
    for (size_t i = 0; i < count && table[i] <= value; ++i) {
        if (table[i] == value)
            return true;
    }
    return false;
}

bool IsJapanese()
{
    return g_gameLanguage == kGameLang_Japanese;
}

bool IsLineBreak(uint32_t c)
{
    return c == '\n' || c == '\r';
}

// Glyph cell corners in strip order (0x00AEF970).
constexpr float kCorners[4][2] = { { 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f } };

uint32_t RoundUp32(uint32_t value)
{
    return (value + 31u) & ~31u;
}

} // namespace

// @func 0x004DA380 DecodeGuiChar
// Decodes the UTF-8 character at 'cursor' (1 to 4 bytes); 'length' receives
// its size and the cursor moves past it when 'advance' is set.
uint32_t DecodeGuiChar(const char*& cursor, uint32_t* length, bool advance)
{
    const auto* p = reinterpret_cast<const uint8_t*>(cursor);
    uint32_t value;
    uint32_t size;
    if ((p[0] & 0x80) == 0) {
        value = p[0];
        size = 1;
    } else if ((p[0] & 0xF0) == 0xE0) {
        value = ((p[0] & 0x0Fu) << 12) | ((p[1] & 0x3Fu) << 6) | (p[2] & 0x3Fu);
        size = 3;
    } else if ((p[0] & 0xF0) == 0xF0) {
        value = ((p[0] & 0x07u) << 18) | ((p[1] & 0x3Fu) << 12) | ((p[2] & 0x3Fu) << 6) | (p[3] & 0x3Fu);
        size = 4;
    } else {
        value = ((p[0] & 0x1Fu) << 6) | (p[1] & 0x3Fu);
        size = 2;
    }
    if (length)
        *length = size;
    if (advance)
        cursor += size;
    return value;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

// @func 0x004DC430 ApeGuiLabel::ApeGuiLabel
// @func 0x004DC5BC ApeGuiLabel::ApeGuiLabel (thunk)
ApeGuiLabel::ApeGuiLabel(const GuiLabelDesc& desc) : ApeGuiObject(desc)
{
    font = desc.font;
    leading = desc.leading;
    tracking = desc.tracking;
    fixedWidth = desc.fixedWidth;
    layoutDirty = true;
    field_94 = (uint8_t)(desc.field_94 & 0xF);
    animMode = (uint8_t)(desc.animMode & 0xF);
    wrapWidth = (uint16_t)desc.wrapWidth;
    if (font) {
        font->AddRef();
        font->AddLoadRef();
    }
    ApeGuiLabel::SetText(desc.text, desc.layoutNow);
    animPeriod = desc.animRate >= 1e-5f ? 1.0f / desc.animRate : 3.4028235e38f;
}

namespace {

// The label attributes of an element (0x004DC720); the defaults come from
// 'source' when a label is cloned.
// @func 0x004DC720 ReadGuiLabelDesc
void ReadGuiLabelDesc(xml::XmlDocument* doc, xml::XmlElement* element, ApeGuiObject* source, GuiLabelDesc& desc)
{
    ApeGuiFontSet* defaultFont = nullptr;
    const char* defaultText = "";
    float defaultLeading = 1.0f;
    float defaultTracking = 0.0f;
    int32_t defaultWrap = 0;
    float defaultRate = 3.0f;
    bool defaultFixedWidth = false;
    if (source) {
        const auto* label = static_cast<const ApeGuiLabel*>(source);
        defaultFont = label->font;
        if (!label->text.empty())
            defaultText = label->text.c_str();
        defaultLeading = label->leading;
        defaultTracking = label->tracking;
        defaultWrap = label->wrapWidth;
        defaultRate = 1.0f / label->animPeriod;
        defaultFixedWidth = label->fixedWidth;
    }
    const char* path = doc ? doc->GetDocumentPath() : "";

    if (const char* fontName = element->GetString("font")) {
        desc.font = static_cast<ApeGuiFontSet*>(ApeGuiResource::FindByName(HashNameNoCase(fontName)));
        if (!desc.font)
            plat::Log(plat::LogLevel::Warning, "%s: unknown GUI font '%s'\n", path, fontName);
    }
    if (!desc.font) {
        desc.font = defaultFont;
        if (!desc.font)
            plat::Log(plat::LogLevel::Warning, "%s: GUI label without a font\n", path);
    }
    if (!desc.font)
        return;

    desc.text = element->GetString("string");
    if (!desc.text)
        desc.text = defaultText;
    element->GetFloat("leading", &desc.leading, defaultLeading);
    element->GetFloat("tracking", &desc.tracking, defaultTracking);
    int32_t wrap = 0;
    element->GetInt("wrap", &wrap, defaultWrap);
    desc.wrapWidth = (uint32_t)wrap;
    element->GetBool("fixedwidth", &desc.fixedWidth, defaultFixedWidth);
    element->GetFloat("animrate", &desc.animRate, defaultRate);
    uint32_t modeHash = 0;
    if (element->GetHash("animmode", &modeHash, 0) == 0) {
        // Same names as the GUI animation modes, without the "fwd" prefix.
        const uint32_t names[5] = { HashNameNoCase("loop"), HashNameNoCase("hold"), HashNameNoCase("pingpong"),
                                    HashNameNoCase("revloop"), HashNameNoCase("revhold") };
        uint32_t mode = 0;
        while (mode < 5 && names[mode] != modeHash)
            ++mode;
        if (mode < 5)
            desc.animMode = mode;
        else
            plat::Log(plat::LogLevel::Warning, "%s: unknown GUI label animmode\n", path);
    }
}

} // namespace

// @func 0x004DCE7C CreateGuiLabel
ApeGuiObject* CreateGuiLabel(xml::XmlDocument* doc, xml::XmlElement* element, const GuiObjectDesc& base,
                             ApeGuiObject* source)
{
    GuiLabelDesc desc;
    static_cast<GuiObjectDesc&>(desc) = base;
    ReadGuiLabelDesc(doc, element, source, desc);
    if (!desc.font)
        return nullptr;
    return new ApeGuiLabel(desc);
}

// @func 0x004E1840 ApeGuiLabel::Clone
ApeGuiObject* ApeGuiLabel::Clone(const char* newName, ApeGuiObject* newParent, void* newOwner,
                                 bool recursive) const
{
    GuiLabelDesc desc;
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
    auto* copy = new ApeGuiLabel(desc);
    if (recursive)
        CloneChildren(copy, newOwner);
    return copy;
}

// ---------------------------------------------------------------------------
// Text
// ---------------------------------------------------------------------------


// @func 0x004DB828 ApeGuiLabel::FormatText
void ApeGuiLabel::FormatText(const char* src, FormatState& st)
{
    if (!src || !*src)
        return;
    if (s_formatDepth >= kMaxFormatDepth) {
        // The console prints the object path here (the print is stripped).
        return;
    }
    ++s_formatDepth;

    // Width of a space, unscaled.
    float space = 0.0f;
    uint32_t spaceFont, spaceGlyph;
    if (font->FindGlyph(' ', spaceFont, spaceGlyph))
        space = (float)font->fonts[spaceFont]->glyphs[spaceGlyph].width * font->fontScales[spaceFont] * font->scale[0];
    else if (font->fontCount && font->fonts[0])
        space = (float)font->fonts[0]->GetHeight() * font->scale[0] * 0.5f;

    StringTable* table = GetGuiStringTable();
    const bool japanese = IsJapanese();
    const char* p = src;
    for (;;) {
        if (*p == '\0' || st.bytes >= st.capacity - 1) {
            *st.out = '\0';
            --s_formatDepth;
            return;
        }

        if (*p == '\\') {
            ++p;
            if (*p == 'n') {
                *st.out++ = '\n';
                st.lastChar = '\n';
                st.lineWidth = 0.0f;
                st.lastBreak = nullptr;
                ++st.bytes;
                st.widthAtBreak = 0.0f;
                ++st.chars;
                st.lineChars = 0;
                ++p;
            }
            // Any other escaped character is formatted as it is.
            continue;
        }

        if (*p == '$' && table) {
            char name[0x100];
            size_t n = 0;
            ++p;
            while ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '_') {
                if (n < sizeof(name) - 1)
                    name[n++] = *p;
                ++p;
            }
            name[n] = '\0';
            FormatText(table->Find(name, nullptr, false), st);
            continue;
        }

        uint32_t length = 0;
        const uint32_t c = DecodeGuiChar(p, &length, false);
        if (c == ' ' || c == '\t') {
            st.lastBreak = st.out;
            st.widthAtBreak = st.lineWidth;
            *st.out++ = *p++;
            ++st.bytes;
            ++st.chars;
            ++st.lineChars;
            st.lineWidth += space;
            st.lastChar = ' ';
            continue;
        }
        if (c == '\r') {
            // A carriage return counts as a space (dropped in Japanese).
            if (!japanese) {
                st.lastBreak = st.out;
                st.widthAtBreak = st.lineWidth;
                *st.out++ = ' ';
                ++st.bytes;
                ++st.chars;
                ++st.lineChars;
                st.lastChar = ' ';
                st.lineWidth += space;
            }
            ++p;
            continue;
        }
        if (c == '\n') {
            *st.out++ = *p++;
            st.lineWidth = 0.0f;
            st.lastBreak = nullptr;
            ++st.bytes;
            ++st.chars;
            st.widthAtBreak = 0.0f;
            st.lineChars = 0;
            st.lastChar = '\n';
            continue;
        }

        // Japanese lines may break before any character but the listed ones.
        if (japanese && c > 0x2000 && !InSortedTable(kNoBreakOpening, std::size(kNoBreakOpening), c) &&
            !InSortedTable(kNoBreakClosing, std::size(kNoBreakClosing), c)) {
            st.lastBreak = st.out;
            st.widthAtBreak = st.lineWidth;
        }

        uint32_t glyphFont, glyphIndex;
        if (!fixedWidth && c != ' ' && font->FindGlyph(c, glyphFont, glyphIndex)) {
            if (st.lineChars)
                st.lineWidth += space * (tracking + font->fontTracking[glyphFont]);
            st.lineWidth += (float)font->fonts[glyphFont]->glyphs[glyphIndex].width * font->fontScales[glyphFont] *
                            font->scale[0];
        } else {
            if (st.lineChars)
                st.lineWidth += space * tracking;
            st.lineWidth += space;
        }

        if (wrapWidth != 0 && (float)wrapWidth <= st.lineWidth) {
            if (!st.lastBreak) {
                // No break point on the line: Japanese breaks before
                // ideographs, other text overflows.
                if (japanese && c > 0x3000) {
                    *st.out++ = '\r';
                    ++st.bytes;
                    st.lineWidth = 0.0f;
                    st.lastBreak = nullptr;
                    st.widthAtBreak = 0.0f;
                    ++st.chars;
                    ++st.lineChars;
                }
            } else {
                if (japanese) {
                    // Insert the break before the character at lastBreak.
                    std::memmove(st.lastBreak + 1, st.lastBreak, (size_t)(st.out - st.lastBreak));
                    ++st.out;
                    ++st.bytes;
                }
                *st.lastBreak = '\r';
                st.lastBreak = nullptr;
                st.lineWidth -= st.widthAtBreak;
                st.widthAtBreak = 0.0f;
            }
        }

        for (; length != 0 && st.bytes < st.capacity - 1; --length) {
            *st.out++ = *p++;
            ++st.bytes;
        }
        p += length;   // the part that did not fit
        st.lastChar = c;
        ++st.chars;
        ++st.lineChars;
    }
}

// @func 0x004DC1B8 ApeGuiLabel::SetText
void ApeGuiLabel::SetText(const char* newText, bool layoutNow)
{
    textId = newText ? newText : "";

    char buffer[kFormatBufferSize];
    buffer[0] = '\0';
    FormatState state;
    state.out = buffer;
    state.capacity = sizeof(buffer);
    FormatText(newText, state);

    ReserveText(state.bytes);
    // Glyph slots for every character (spaces and breaks included).
    ReserveGlyphs(state.chars);
    if (state.bytes)
        text.assign(buffer, state.bytes);

    Invalidate();
    if (layoutNow && layoutDirty)
        Layout(true);
}

// @func 0x004DB780 ApeGuiLabel::ReserveText
void ApeGuiLabel::ReserveText(uint32_t bytes)
{
    if (text.capacity() <= bytes || bytes == 0) {
        text.clear();
        text.shrink_to_fit();
        if (bytes)
            text.reserve((bytes + 32u) & ~31u);
    }
}

// @func 0x004DB6CC ApeGuiLabel::ReserveGlyphs
void ApeGuiLabel::ReserveGlyphs(uint32_t count)
{
    if (glyphs.size() < count || count == 0) {
        glyphs.clear();
        if (count)
            glyphs.assign(RoundUp32(count), GuiLabelGlyph{});
    }
}

// @func 0x004E2178 ApeGuiLabel::CountGlyphs
void ApeGuiLabel::CountGlyphs()
{
    uint32_t count = 0;
    if (!text.empty()) {
        const char* p = text.c_str();
        while (*p && count < glyphs.size()) {
            const uint32_t c = DecodeGuiChar(p, nullptr, true);
            uint32_t fontIndex, glyphIndex;
            if (!IsLineBreak(c) && c != ' ' && font->FindGlyph(c, fontIndex, glyphIndex))
                ++count;
        }
    }
    glyphCount = count;
}

// @func 0x004E22DC ApeGuiLabel::Invalidate
void ApeGuiLabel::Invalidate()
{
    revealTime = 0.0f;
    layoutDirty = true;
    CountGlyphs();
    ApeGuiObject::Invalidate();
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

// @func 0x004DB1E8 ApeGuiLabel::GetSpaceWidth
float ApeGuiLabel::GetSpaceWidth(const Vec3& worldScale) const
{
    if (!font)
        return 0.0f;
    uint32_t fontIndex, glyphIndex;
    if (font->FindGlyph(' ', fontIndex, glyphIndex))
        return (float)font->fonts[fontIndex]->glyphs[glyphIndex].width * font->fontScales[fontIndex] *
               font->scale[0] * worldScale.x;
    if (font->fontCount && font->fonts[0])
        return (float)font->fonts[0]->GetHeight() * font->scale[0] * 0.5f;   // NOTE: not scaled, as on the console
    return 0.0f;
}

// @func 0x004DAEE8 ApeGuiLabel::ComputeLineOffsets
// Each line starts at -width/2 times the horizontal alignment.
uint32_t ApeGuiLabel::ComputeLineOffsets(float spaceWidth, float* offsets, uint32_t maxLines,
                                         const Vec3& worldScale) const
{
    offsets[0] = 0.0f;
    if (text.empty())
        return 0;
    uint32_t line = 0;
    uint32_t lineChars = 0;
    const char* p = text.c_str();
    while (*p) {
        const uint32_t c = DecodeGuiChar(p, nullptr, true);
        if (IsLineBreak(c)) {
            // NOTE: the console does not check the buffer size.
            if (line + 1 < maxLines)
                ++line;
            offsets[line] = 0.0f;
            lineChars = 0;
            continue;
        }
        uint32_t fontIndex, glyphIndex;
        if (!fixedWidth && font->FindGlyph(c, fontIndex, glyphIndex)) {
            if (lineChars)
                offsets[line] += spaceWidth * (tracking + font->fontTracking[fontIndex]);
            offsets[line] += (float)font->fonts[fontIndex]->glyphs[glyphIndex].width * font->fontScales[fontIndex] *
                             font->scale[0] * worldScale.x;
        } else {
            if (lineChars)
                offsets[line] += spaceWidth * tracking;
            offsets[line] += spaceWidth;
        }
        ++lineChars;
    }
    const uint32_t lines = line + 1;
    const float align = (float)((flags >> 14) & 3);
    for (uint32_t i = 0; i < lines; ++i)
        offsets[i] = offsets[i] * -0.5f * align;
    return lines;
}

// @func 0x004DAAA8 ApeGuiLabel::BuildGlyph
// The glyph's top sits on the line's baseline (3/4 of the line height
// down) minus the glyph's ascent; fixed width text centres each glyph in a
// space-wide cell.
bool ApeGuiLabel::BuildGlyph(float spaceWidth, GuiLabelGlyph& glyph, uint32_t codePoint, float& x, float y,
                             const Vec3& worldScale, uint32_t line) const
{
    if (!font)
        return false;
    uint32_t fontIndex, glyphIndex;
    if (!font->FindGlyph(codePoint, fontIndex, glyphIndex))
        return false;
    const ApeGuiBitmapFont* bitmapFont = font->fonts[fontIndex];
    const GuiGlyph& source = bitmapFont->glyphs[glyphIndex];
    const float fontScale = font->fontScales[fontIndex];
    const float lineHeight = font->GetLineHeight();

    glyph.material = bitmapFont->materials[GetBlendMode()];
    glyph.untinted = bitmapFont->flag_4704F4C;
    const float width = (float)source.width * fontScale * font->scale[0] * worldScale.x;
    glyph.x = fixedWidth ? (spaceWidth - width) * 0.5f + x : x;
    glyph.y = lineHeight * worldScale.y * 3.0f * 0.25f + y - (float)source.top * fontScale * font->scale[1] * worldScale.y;
    glyph.width = width;
    glyph.height = (float)(source.top + source.bottom + 1) * fontScale * font->scale[1] * worldScale.y;
    glyph.texX = source.x;
    glyph.texY = source.y;
    glyph.texWidth = source.width;
    glyph.texHeight = (uint16_t)(source.top + source.bottom + 1);
    glyph.frames = source.cells & 0xF;
    // Reversed animation modes start on the last frame.
    glyph.frame = animMode > kGuiAnim_PingPong ? (uint8_t)((glyph.frames - 1) & 0xF) : 0;
    glyph.animTime = 0.0f;
    glyph.line = (uint16_t)(line & 0x3FF);

    x += fixedWidth ? spaceWidth : width;
    x += spaceWidth * (tracking + font->fontTracking[fontIndex]);
    return true;
}

// @func 0x004DB378 ApeGuiLabel::Layout
void ApeGuiLabel::Layout(bool force)
{
    if (!force && !(IsVisible() && font && !text.empty()))
        return;

    uint32_t built = 0;
    uint32_t line = 0;
    if (!glyphs.empty()) {
        const Vec3 worldScale = GetWorldScale();
        const float space = GetSpaceWidth(worldScale);
        float offsets[kMaxLines];
        const uint32_t lines = ComputeLineOffsets(space, offsets, kMaxLines, worldScale);
        const float lineStep = font->GetLineHeight() * worldScale.y * leading;
        const float alignV = (float)((flags >> 18) & 3);
        float x = offsets[0];
        float y = lineStep * -0.5f * (float)lines * alignV;

        std::fill(glyphs.begin(), glyphs.end(), GuiLabelGlyph{});
        const char* p = text.c_str();
        uint32_t slot = 0;
        while (*p && slot < glyphs.size()) {
            const uint32_t c = DecodeGuiChar(p, nullptr, true);
            if (IsLineBreak(c)) {
                if (line < kMaxLines - 1)
                    ++line;
                y += lineStep;
                x = offsets[line];
                continue;
            }
            if (c != ' ' && BuildGlyph(space, glyphs[slot++], c, x, y, worldScale, line)) {
                ++built;
                continue;
            }
            x += space * (tracking + 1.0f);
        }
    }
    layoutDirty = false;
    progress[0] = progress[1] = 0.0f;
    if (revealInterval <= 0.0f) {
        progress[0] = (float)built;
        progress[1] = (float)(line + 1);
    }
}

uint32_t ApeGuiLabel::GetVisibleGlyphLimit() const
{
    const uint32_t slots = (uint32_t)glyphs.size();
    if (revealInterval > 0.0f)
        return std::min((uint32_t)(revealTime / revealInterval), slots);
    return slots;
}

// @func 0x004E1B58 ApeGuiLabel::GetLayoutSize
// The size of the text block: the widest line and the line heights.
void ApeGuiLabel::GetLayoutSize(float out[2]) const
{
    if (HasFlag(kFlag_Dirty)) {
        float width = 0.0f;
        float height = 0.0f;
        if (font && !text.empty()) {
            const Vec3 worldScale = GetWorldScale();
            const float space = GetSpaceWidth(worldScale);
            const float lineHeight = font->GetLineHeight() * worldScale.y;
            height = lineHeight;
            const uint32_t limit = GetVisibleGlyphLimit();
            float lineWidth = 0.0f;
            uint32_t counted = 0;
            const char* p = text.c_str();
            while (*p && counted < limit) {
                const uint32_t c = DecodeGuiChar(p, nullptr, true);
                if (IsLineBreak(c)) {
                    height = std::max(height + lineHeight * leading, 0.0f) + lineHeight;
                    lineWidth = 0.0f;
                    continue;
                }
                // NOTE: the console compares the read position with the line
                // start after decoding, so the tracking is added before every
                // character, the first of a line included.
                uint32_t fontIndex, glyphIndex;
                if (!fixedWidth && c != ' ' && font->FindGlyph(c, fontIndex, glyphIndex)) {
                    lineWidth += space * (tracking + font->fontTracking[fontIndex]);
                    lineWidth += (float)font->fonts[fontIndex]->glyphs[glyphIndex].width * font->fontScales[fontIndex] *
                                 font->scale[0] * worldScale.x;
                    ++counted;
                } else {
                    lineWidth += space * tracking;
                    lineWidth += space;
                }
                width = std::max(width, lineWidth);
            }
        }
        layoutSize[0] = width;
        layoutSize[1] = height;
        flags &= ~kFlag_Dirty;
    }
    out[0] = layoutSize[0];
    out[1] = layoutSize[1];
}

// @func 0x004E20A8 ApeGuiLabel::GetLineSpacing
float ApeGuiLabel::GetLineSpacing() const
{
    const Vec3 worldScale = GetWorldScale();
    return font->GetLineHeight() * worldScale.y * leading;
}

// ---------------------------------------------------------------------------
// Animation and drawing
// ---------------------------------------------------------------------------

// @func 0x004E1454 GuiLabelGlyph::Tick
void GuiLabelGlyph::Tick(float dt, float period, uint32_t mode)
{
    if (frames <= 1)
        return;
    animTime += dt;
    while (animTime >= period) {
        switch (mode) {
        case kGuiAnim_FwdHold:
            frame = (uint8_t)std::min<uint32_t>(frame + 1u, frames - 1u);
            break;
        case kGuiAnim_PingPong:
            frame = (uint8_t)((reverse ? frame - 1 : frame + 1) & 0xF);
            if (frame == 0 || frame == frames - 1)
                reverse = !reverse;
            break;
        case kGuiAnim_RevLoop:
            frame = (uint8_t)((frame ? frame : frames) - 1);
            break;
        case kGuiAnim_RevHold:
            frame = frame ? (uint8_t)(frame - 1) : 0;
            break;
        default:   // forward loop
            frame = (uint8_t)((frame + 1u) % frames);
            break;
        }
        animTime -= period;
    }
}

// @func 0x004E23A0 ApeGuiLabel::TickGlyphs
void ApeGuiLabel::TickGlyphs(float dt)
{
    if (!IsVisible())
        return;
    revealTime += dt;
    // A label being revealed changes size every frame.
    if (revealInterval > 0.0f)
        flags |= kFlag_Dirty;
    for (GuiLabelGlyph& glyph : glyphs)
        glyph.Tick(dt, animPeriod, animMode);
}

// @func 0x004E2480 ApeGuiLabel::Tick
void ApeGuiLabel::Tick(float dt)
{
    if (layoutDirty)
        Layout(false);
    TickGlyphs(dt);
    ApeGuiObject::Tick(dt);
}

// @func 0x004E2628 ApeGuiLabel::Draw
void ApeGuiLabel::Draw()
{
    if (IsVisible() && font && !text.empty()) {
        Matrix34 world = GetWorldMatrix();
        agl::AglRenderTarget* target = GetGuiDrawTarget();
        if (target && target->projectionType != agl::kAglProjection_Perspective)
            world.translation.z = -1.0f;
        const xml::Color8 tint = GetWorldColor();
        // Untinted glyphs keep their colours and only take the alpha.
        const uint8_t colors[2][4] = { { tint.r, tint.g, tint.b, tint.a }, { 0xFF, 0xFF, 0xFF, tint.a } };

        const uint32_t limit = GetVisibleGlyphLimit();
        agl::AglDrawList& list = GetGuiDrawList();
        list.SetWorldMatrix(world, kGuiDrawTag);
        uint32_t lines = 0;
        for (uint32_t i = 0; i < limit; ++i) {
            const GuiLabelGlyph& glyph = glyphs[i];
            lines = std::max<uint32_t>(lines, glyph.line + 1u);
            if (!glyph.material)
                continue;
            const agl::AglTexture* image = glyph.material->textures[0];
            const float invWidth = 1.0f / (float)image->width;
            const float invHeight = 1.0f / (float)image->height;
            list.SetMaterial(glyph.material, kGuiDrawTag);
            agl::AglDrawPrimitive primitive;
            list.AllocPrimitive(&primitive, (uint8_t)agl::gpu::PrimitiveType::TriangleStrip, 4, kGuiDrawTag);
            const uint8_t* color = colors[glyph.untinted ? 1 : 0];
            for (int v = 0; v < 4; ++v) {
                agl::AglDrawVertex& vertex = primitive.vertices[v];
                std::memcpy(vertex.color, color, 4);
                vertex.position[0] = glyph.width * kCorners[v][0] + glyph.x;
                vertex.position[1] = glyph.height * kCorners[v][1] + glyph.y;
                vertex.position[2] = 0.0f;
                vertex.uv[0] = ((float)glyph.texWidth * ((float)glyph.frame + kCorners[v][0]) + (float)glyph.texX) * invWidth;
                vertex.uv[1] = ((float)glyph.texHeight * kCorners[v][1] + (float)glyph.texY) * invHeight;
            }
            list.EndPrimitive();
        }
        progress[0] = (float)limit;
        progress[1] = (float)lines;
    }
    ApeGuiObject::Draw();
}

} // namespace ape
