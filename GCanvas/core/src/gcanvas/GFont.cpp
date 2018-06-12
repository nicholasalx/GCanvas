/**
 * Created by G-Canvas Open Source Team.
 * Copyright (c) 2017, Alibaba, Inc. All rights reserved.
 *
 * This source code is licensed under the Apache Licence 2.0.
 * For the full copyright and license information, please view
 * the LICENSE file in the root directory of this source tree.
 */
#include "GFont.h"
#include "GCanvas2dContext.h"
#include <assert.h>
#include "../GCanvas.hpp"
#ifdef GFONT_LOAD_BY_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H

#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST     {
#define FT_ERROR_END_LIST       { 0, 0 } };
const struct {
    int          code;
    const char*  message;
} FT_Errors[] =
#include FT_ERRORS_H
#endif

#define PT_TO_PX(pt) ceilf((pt) * (1.0f + (1.0f / 3.0f)))

void * (*GFont::getFontCallback)(const char *fontDefinition) = nullptr;
bool (*GFont::getFontImageCallback)(void *font, wchar_t charcode,
                                     int &ftBitmapWidth, int &ftBitmapHeight,
                                     unsigned char *&bitmapBuffer, int &left,
                                     int &top, float &advanceX,
                                     float &advanceY) = nullptr;

GFontMetrics::GFontMetrics() : unitsPerEM(0), ascender(0.0f), descender(0.0f)
{
}

#ifdef GFONT_LOAD_BY_FREETYPE
GFont::GFont(const char *fontName, const float size)
    : mPointSize(size), mFontName(fontName), mHinting(1), mOutlineType(0),
      mOutlineThickness(0), mFont(0)
{
}
#else
GFont::GFont(const char *fontDefinition)
    : m_fontDefinition(fontDefinition), m_font(0)
{
}
#endif

GFont::~GFont() {}

void GFont::DrawText(wchar_t text, GCanvasContext *context, float &x, float y,
                      GColorRGBA color)
{
    const GGlyph *glyph = GetGlyph(text);
    if (glyph != nullptr)
    {
        drawGlyph(glyph, context, x, y, color);
        x += glyph->advanceX;
    }

    context->SendVertexBufferToGPU();
}

void GFont::DrawText(const wchar_t *text, GCanvasContext *context, float &x,
                      float y, GColorRGBA color)
{
    if (text == nullptr || wcslen(text) == 0)
    {
        return;
    }

    for (size_t i = 0; i < wcslen(text); ++i)
    {
        const GGlyph *glyph = GetGlyph(text[i]);

        if (glyph != nullptr)
        {
            drawGlyph(glyph, context, x, y, color);
            x += glyph->advanceX;
        }
    }

    context->SendVertexBufferToGPU();
}

void GFont::drawGlyph(const GGlyph *glyph, GCanvasContext *context, float x,
                       float y, GColorRGBA color)
{
    context->SetTexture(glyph->texture);

    float x0 = (float)(x + glyph->offsetX);
    float y0 = (float)(y + glyph->height - glyph->offsetY);
    float x1 = (float)(x0 + glyph->width);
    float y1 = (float)(y0 - glyph->height);
    float s0 = glyph->s0;
    float t0 = glyph->t0;
    float s1 = glyph->s1;
    float t1 = glyph->t1;

    context->PushRectangle(x0, y1, x1 - x0, y0 - y1, s0, t0, s1 - s0, t1 - t0,
                           color);
}

void GFont::SetFontCallback(
        void *(*getFontCB)(const char *fontDefinition),
        bool (*getFontImageCB)(void *font, wchar_t charcode, int &ftBitmapWidth,
                               int &ftBitmapHeight, unsigned char *&bitmapBuffer,
                               int &left, int &top, float &advanceX,
                               float &advanceY))
{
    GFont::getFontCallback = getFontCB;
    GFont::getFontImageCallback = getFontImageCB;
}

const GGlyph *GFont::GetGlyph(const wchar_t charcode)
{
    std::map< wchar_t, GGlyph >::const_iterator iter = mGlyphs.find(charcode);
    if (iter != mGlyphs.end())
    {
        return &(iter->second);
    }

#ifdef GFONT_LOAD_BY_FREETYPE
    wchar_t buffer[2] = {0, 0};
    buffer[0] = charcode;
    loadGlyphs(buffer);

    iter = mGlyphs.find(charcode);
    assert(iter != mGlyphs.end());
    return &(iter->second);
#else
    bool found = false;
    if (nullptr == m_font && nullptr != getFontCallback)
    {
        m_font = getFontCallback(m_fontDefinition.c_str());
    }

    if (nullptr != m_font && getFontImageCallback != nullptr)
    {
        int ftBitmapWidth, ftBitmapHeight, left, top;
        float advanceX, advanceY;
        unsigned char *bitmapBuffer;
        found = getFontImageCallback(m_font, charcode, ftBitmapWidth,
                                     ftBitmapHeight, bitmapBuffer, left, top,
                                     advanceX, advanceY);
        if (found)
        {
            loadGlyph(charcode, ftBitmapWidth, ftBitmapHeight, bitmapBuffer,
                      left, top, advanceX, advanceY);
            iter = m_glyphs.find(charcode);
            assert(iter != m_glyphs.end());
            return &(iter->second);
        }
    }
    return nullptr;
#endif
}

void GFont::RemoveGlyph(const wchar_t charcode) { mGlyphs.erase(charcode); }

void GFont::LoadGlyph(wchar_t charcode, int ftBitmapWidth, int ftBitmapHeight,
                       unsigned char *bitmapBuffer, int left, int top,
                       float advanceX, float advanceY)
{

    GTexture *texture =
        new GTexture(ftBitmapWidth, ftBitmapHeight, GL_ALPHA, bitmapBuffer);

    GGlyph glyph;
    glyph.charcode = charcode;
    glyph.texture = texture;
    glyph.width = ftBitmapWidth;
    glyph.height = ftBitmapHeight;
    glyph.outlineType = 0;
    glyph.outlineThickness = 0;
    glyph.offsetX = left;
    glyph.offsetY = top;
    glyph.s0 = 0;
    glyph.t0 = 0;
    glyph.s1 = 1;
    glyph.t1 = 1;
    glyph.advanceX = advanceX;
    glyph.advanceY = advanceY;

    mGlyphs.insert(std::pair< wchar_t, GGlyph >(glyph.charcode, glyph));
}

#ifdef GFONT_LOAD_BY_FREETYPE
bool GFont::loadFace(FT_Library *library, const char *filename,
                      const float size, FT_Face *face)
{
    size_t hres = 64;
    FT_Matrix matrix = {(int)((1.0 / hres) * 0x10000L), (int)((0.0) * 0x10000L),
                        (int)((0.0) * 0x10000L), (int)((1.0) * 0x10000L)};

    assert(library);
    assert(filename);
    assert(size);

    /* Initialize library */
    FT_Error error = FT_Init_FreeType(library);
    if (error)
    {
        return false;
    }

    /* Load face */
    error = FT_New_Face(*library, filename, 0, face);
    if (error)
    {
        LOG_D(filename);
        assert(filename == 0);
        FT_Done_FreeType(*library);
        return false;
    }

    /* Select charmap */
    error = FT_Select_Charmap(*face, FT_ENCODING_UNICODE);
    if (error)
    {
        FT_Done_Face(*face);
        FT_Done_FreeType(*library);
        return false;
    }

    /* Set char size */
    error =
        FT_Set_Char_Size(*face, (int)(size * 64), 0, (FT_UInt)72 * hres, 72);
    if (error)
    {
        FT_Done_Face(*face);
        FT_Done_FreeType(*library);
        return false;
    }

    /* Set transform matrix */
    FT_Set_Transform(*face, &matrix, nullptr);

    return true;
}

void GFont::loadGlyphs(const wchar_t *charcodes)
{
    FT_Library library;
    FT_Face face;
    FT_Glyph ft_glyph;
    FT_GlyphSlot slot;
    FT_Bitmap ft_bitmap;

    assert(charcodes);

    if (!loadFace(&library, mFontName.c_str(), mPointSize, &face))
    {
        return;
    }

    FT_Int32 flags = 0;
    if (mOutlineType > 0)
    {
        flags |= FT_LOAD_NO_BITMAP;
    }
    else
    {
        flags |= FT_LOAD_RENDER;
    }

    if (mHinting)
    {
        flags |= FT_LOAD_FORCE_AUTOHINT;
    }
    else
    {
        flags |= FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT;
    }

    /* Load each glyph */
    for (size_t i = 0; i < wcslen(charcodes); ++i)
    {
        int ft_bitmap_width = 0;
        int ft_bitmap_rows = 0;
        int ft_bitmap_pitch = 0;
        int ft_glyph_top = 0;
        int ft_glyph_left = 0;
        FT_UInt glyph_index = FT_Get_Char_Index(face, charcodes[i]);

        FT_Error error = FT_Load_Glyph(face, glyph_index, flags);
        if (error)
        {
            FT_Done_Face(face);
            FT_Done_FreeType(library);
            return;
        }

        if (mOutlineType == 0)
        {
            slot = face->glyph;
            ft_bitmap = slot->bitmap;
            ft_bitmap_width = slot->bitmap.width;
            ft_bitmap_rows = slot->bitmap.rows;
            ft_bitmap_pitch = slot->bitmap.pitch;
            ft_glyph_top = slot->bitmap_top;
            ft_glyph_left = slot->bitmap_left;

            (void)ft_bitmap_pitch;
        }
        else
        {
            FT_Stroker stroker;
            FT_BitmapGlyph ft_bitmap_glyph;
            error = FT_Stroker_New(library, &stroker);
            if (error)
            {
                FT_Done_Face(face);
                FT_Stroker_Done(stroker);
                FT_Done_FreeType(library);
                return;
            }
            FT_Stroker_Set(stroker, (int)(mOutlineThickness * 64),
                           FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND,
                           0);
            error = FT_Get_Glyph(face->glyph, &ft_glyph);
            if (error)
            {
                FT_Done_Face(face);
                FT_Stroker_Done(stroker);
                FT_Done_FreeType(library);
                return;
            }

            if (mOutlineType == 1)
            {
                error = FT_Glyph_Stroke(&ft_glyph, stroker, 1);
            }
            else if (mOutlineType == 2)
            {
                error = FT_Glyph_StrokeBorder(&ft_glyph, stroker, 0, 1);
            }
            else if (mOutlineType == 3)
            {
                error = FT_Glyph_StrokeBorder(&ft_glyph, stroker, 1, 1);
            }
            if (error)
            {
                FT_Done_Face(face);
                FT_Stroker_Done(stroker);
                FT_Done_FreeType(library);
                return;
            }

            error = FT_Glyph_To_Bitmap(&ft_glyph, FT_RENDER_MODE_NORMAL, 0, 1);
            if (error)
            {
                FT_Done_Face(face);
                FT_Stroker_Done(stroker);
                FT_Done_FreeType(library);
                return;
            }

            ft_bitmap_glyph = (FT_BitmapGlyph)ft_glyph;
            ft_bitmap = ft_bitmap_glyph->bitmap;
            ft_bitmap_width = ft_bitmap.width;
            ft_bitmap_rows = ft_bitmap.rows;
            ft_bitmap_pitch = ft_bitmap.pitch;
            ft_glyph_top = ft_bitmap_glyph->top;
            ft_glyph_left = ft_bitmap_glyph->left;
            FT_Stroker_Done(stroker);
        }

        slot = face->glyph;
        float advanceX = (float)(slot->advance.x / 64.0f);
        float advanceY = (float)(slot->advance.y / 64.0f);

        LoadGlyph(charcodes[i], ft_bitmap_width, ft_bitmap_rows,
                  ft_bitmap.buffer, ft_glyph_left, ft_glyph_top, advanceX,
                  advanceY);

        if (mOutlineType > 0)
        {
            FT_Done_Glyph(ft_glyph);
        }
    }

    if (FT_IS_SCALABLE(face))
    {
        this->mFontMetrics.unitsPerEM = face->units_per_EM;
        // 26.6 pixel format: convert it from font units
        this->mFontMetrics.ascender =
            (float)(face->size->metrics.ascender) / 64.0f;
        this->mFontMetrics.descender =
            (float)(face->size->metrics.descender) / 64.0f;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(library);
}
#endif