/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Font class.


#ifndef CRYINCLUDE_CRYFONT_FFONT_H
#define CRYINCLUDE_CRYFONT_FFONT_H
#pragma once


#if !defined(USE_NULLFONT_ALWAYS)

#include <vector>
#include <Cry_Math.h>
#include <Cry_Color.h>
#include <CryString.h>

#include <AzCore/std/parallel/mutex.h>

struct ISystem;
class CCryFont;
class CFontTexture;


class CFFont
    : public IFFont
    , public IFFont_RenderProxy
{
public:
    struct SRenderingPass
    {
        ColorB m_color;
        Vec2 m_posOffset;
        int m_blendSrc;
        int m_blendDest;

        SRenderingPass()
            : m_color(255, 255, 255, 255)
            , m_posOffset(0, 0)
            , m_blendSrc(GS_BLSRC_SRCALPHA)
            , m_blendDest(GS_BLDST_ONEMINUSSRCALPHA)
        {
        }

        void GetMemoryUsage(ICrySizer* pSizer) const {}
    };

    struct SEffect
    {
        string m_name;
        std::vector<SRenderingPass> m_passes;

        SEffect(const char* name)
            : m_name(name)
        {
            assert(name);
        }

        SRenderingPass* AddPass()
        {
            m_passes.push_back(SRenderingPass());
            return &m_passes[m_passes.size() - 1];
        }

        void ClearPasses()
        {
            m_passes.resize(0);
        }

        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            pSizer->AddObject(m_name);
            pSizer->AddObject(m_passes);
        }
    };

    typedef std::vector<SEffect> Effects;
    typedef Effects::iterator EffectsIt;

public:
    virtual int32 AddRef() override;
    virtual int32 Release() override;
    virtual bool Load(const char* pFontFilePath, unsigned int width, unsigned int height, unsigned int widthNumSlots, unsigned int heightNumSlots, unsigned int flags);
    virtual bool Load(const char* pXMLFile);
    virtual void Free();
    virtual void DrawString(float x, float y, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx);
    virtual void DrawString(float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx);
    virtual Vec2 GetTextSize(const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx);
    virtual size_t GetTextLength(const char* pStr, const bool asciiMultiLine) const;
    virtual void WrapText(string& result, float maxWidth, const char* pStr, const STextDrawContext& ctx);
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual void GetGradientTextureCoord(float& minU, float& minV, float& maxU, float& maxV) const;
    virtual unsigned int GetEffectId(const char* pEffectName) const;
    virtual unsigned int GetNumEffects() const;
    virtual const char* GetEffectName(unsigned int effectId) const;
    virtual void AddCharsToFontTexture(const char* pChars) override;
    virtual Vec2 GetKerning(uint32_t leftGlyph, uint32_t rightGlyph, const STextDrawContext& ctx) const override;

public:
    virtual void RenderCallback(float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx);

public:
    CFFont(ISystem* pSystem, CCryFont* pCryFont, const char* pFontName);

    bool InitTexture();
    bool InitCache();

    CFontTexture* GetFontTexture() const { return m_pFontTexture; }
    const string& GetName() const { return m_name; }

    SEffect* AddEffect(const char* pEffectName);
    SEffect* GetDefaultEffect();

private:
    virtual ~CFFont();

    void Prepare(const char* pStr, bool updateTexture);
    void DrawStringUInternal(float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx);
    Vec2 GetTextSizeUInternal(const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx);

    struct TextScaleInfoInternal
    {
        TextScaleInfoInternal(const Vec2& _scale, float _rcpCellWidth)
            : scale(_scale), rcpCellWidth(_rcpCellWidth) { }

        Vec2 scale;
        float rcpCellWidth;
    };

    TextScaleInfoInternal CalculateScaleInternal(const STextDrawContext& ctx) const;

private:
    string m_name;
    string m_curPath;

    CFontTexture* m_pFontTexture;

    size_t m_fontBufferSize;
    unsigned char* m_pFontBuffer;

    int m_texID;

    ISystem* m_pSystem;

    AZStd::recursive_mutex m_fontMutex; //!< Controls access between main and render threads. It's common for one thread
                                        //!< to add un-cached glyphs to the font texture while another is accessing the
                                        //!< font texture.

    CCryFont* m_pCryFont;

    bool m_fontTexDirty;

    Effects m_effects;

    SVF_P3F_C4B_T2F* m_pDrawVB;

    volatile int32 m_nRefCount;

    bool m_monospacedFont; //!< True if this font is fixed/monospaced, false otherwise (obtained from FreeType)
};

#endif

#endif // CRYINCLUDE_CRYFONT_FFONT_H
