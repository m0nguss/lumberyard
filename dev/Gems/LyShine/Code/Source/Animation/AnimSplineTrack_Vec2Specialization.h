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

#pragma once

#include <LyShine/UiBase.h>

template <>
inline TUiAnimSplineTrack<Vec2>::TUiAnimSplineTrack()
    : m_refCount(0)
{
    AllocSpline();
    m_flags = 0;
    m_defaultValue = Vec2(0, 0);
    m_fMinKeyValue = 0.0f;
    m_fMaxKeyValue = 0.0f;
    m_bCustomColorSet = false;
}
template <>
inline void TUiAnimSplineTrack<Vec2>::GetValue(float time, float& value)
{
    if (GetNumKeys() == 0)
    {
        value = m_defaultValue.y;
    }
    else
    {
        Spline::ValueType tmp;
        m_spline->Interpolate(time, tmp);
        value = tmp[0];
    }
}
template <>
inline EUiAnimCurveType TUiAnimSplineTrack<Vec2>::GetCurveType() { return eUiAnimCurveType_BezierFloat; }
template <>
inline EUiAnimValue TUiAnimSplineTrack<Vec2>::GetValueType() { return eUiAnimValue_Float; }
template <>
inline void TUiAnimSplineTrack<Vec2>::SetValue(float time, const float& value, bool bDefault)
{
    if (!bDefault)
    {
        I2DBezierKey key;
        key.value = Vec2(time, value);
        SetKeyAtTime(time, &key);
    }
    else
    {
        m_defaultValue = Vec2(time, value);
    }
}

template <>
inline void TUiAnimSplineTrack<Vec2>::GetKey(int index, IKey* key) const
{
    assert(index >= 0 && index < GetNumKeys());
    assert(key != 0);
    Spline::key_type& k = m_spline->key(index);
    I2DBezierKey* bezierkey = (I2DBezierKey*)key;
    bezierkey->time = k.time;
    bezierkey->flags = k.flags;

    bezierkey->value = k.value;
}

template <>
inline void TUiAnimSplineTrack<Vec2>::SetKey(int index, IKey* key)
{
    assert(index >= 0 && index < GetNumKeys());
    assert(key != 0);
    Spline::key_type& k = m_spline->key(index);
    I2DBezierKey* bezierkey = (I2DBezierKey*)key;
    k.time = bezierkey->time;
    k.flags = bezierkey->flags;
    k.value = bezierkey->value;
    UpdateTrackValueRange(k.value.y);
    Invalidate();
}

//! Create key at given time, and return its index.
template <>
inline int TUiAnimSplineTrack<Vec2>::CreateKey(float time)
{
    float value;

    int nkey = GetNumKeys();

    if (nkey > 0)
    {
        GetValue(time, value);
    }
    else
    {
        value = m_defaultValue.y;
    }

    UpdateTrackValueRange(value);

    Spline::ValueType tmp;
    tmp[0] = value;
    tmp[1] = 0;
    return m_spline->InsertKey(time, tmp);
}

template <>
inline int TUiAnimSplineTrack<Vec2>::CopyKey(IUiAnimTrack* pFromTrack, int nFromKey)
{
    // This small time offset is applied to prevent the generation of singular tangents.
    float timeOffset = 0.01f;
    I2DBezierKey key;
    pFromTrack->GetKey(nFromKey, &key);
    float t = key.time + timeOffset;
    int newIndex =  CreateKey(t);
    key.time = key.value.x = t;
    SetKey(newIndex, &key);
    return newIndex;
}

template <>
inline bool TUiAnimSplineTrack<Vec2>::Serialize(IUiAnimationSystem* uiAnimationSystem, XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    if (bLoading)
    {
        int num = xmlNode->getChildCount();

        int flags = m_flags;
        xmlNode->getAttr("Flags", flags);
        xmlNode->getAttr("defaultValue", m_defaultValue);
        SetFlags(flags);
        xmlNode->getAttr("HasCustomColor", m_bCustomColorSet);
        if (m_bCustomColorSet)
        {
            unsigned int abgr;
            xmlNode->getAttr("CustomColor", abgr);
            m_customColor = ColorB(abgr);
        }

        SetNumKeys(num);
        for (int i = 0; i < num; i++)
        {
            I2DBezierKey key; // Must be inside loop.

            XmlNodeRef keyNode = xmlNode->getChild(i);
            if (!keyNode->getAttr("time", key.time))
            {
                CryLog("[UI_ANIMATION:TUiAnimSplineTrack<Vec2>::Serialize]Ill formed legacy track:missing time information.");
                return false;
            }
            if (!keyNode->getAttr("value", key.value))
            {
                CryLog("[UI_ANIMATION:TUiAnimSplineTrack<Vec2>::Serialize]Ill formed legacy track:missing value information.");
                return false;
            }
            //assert(key.time == key.value.x);

            keyNode->getAttr("flags", key.flags);

            SetKey(i, &key);

            // In-/Out-tangent
            if (!keyNode->getAttr("ds", m_spline->key(i).ds))
            {
                CryLog("[UI_ANIMATION:TUiAnimSplineTrack<Vec2>::Serialize]Ill formed legacy track:missing ds spline information.");
                return false;
            }

            if (!keyNode->getAttr("dd", m_spline->key(i).dd))
            {
                CryLog("[UI_ANIMATION:TUiAnimSplineTrack<Vec2>::Serialize]Ill formed legacy track:dd spline information.");
                return false;
            }
        }

        if ((!num) && (!bLoadEmptyTracks))
        {
            return false;
        }
    }
    else
    {
        int num = GetNumKeys();
        xmlNode->setAttr("Flags", GetFlags());
        xmlNode->setAttr("defaultValue", m_defaultValue);
        xmlNode->setAttr("HasCustomColor", m_bCustomColorSet);
        if (m_bCustomColorSet)
        {
            xmlNode->setAttr("CustomColor", m_customColor.pack_abgr8888());
        }
        I2DBezierKey key;
        for (int i = 0; i < num; i++)
        {
            GetKey(i, &key);
            XmlNodeRef keyNode = xmlNode->newChild("Key");
            assert(key.time == key.value.x);
            keyNode->setAttr("time", key.time);
            keyNode->setAttr("value", key.value);

            int flags = key.flags;
            // Just save the in/out/unify mask part. Others are for editing convenience.
            flags &= (SPLINE_KEY_TANGENT_IN_MASK | SPLINE_KEY_TANGENT_OUT_MASK | SPLINE_KEY_TANGENT_UNIFY_MASK);
            if (flags != 0)
            {
                keyNode->setAttr("flags", flags);
            }

            // We also have to save in-/out-tangents, because TCB infos are not used for custom tangent keys.
            keyNode->setAttr("ds", m_spline->key(i).ds);
            keyNode->setAttr("dd", m_spline->key(i).dd);
        }
    }
    return true;
}

template <>
inline bool TUiAnimSplineTrack<Vec2>::SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected, float fTimeOffset)
{
    if (bLoading)
    {
        int numCur = GetNumKeys();
        int num = xmlNode->getChildCount();

        int type;
        xmlNode->getAttr("TrackType", type);

        if (type != GetCurveType())
        {
            return false;
        }

        SetNumKeys(num + numCur);
        for (int i = 0; i < num; i++)
        {
            I2DBezierKey key; // Must be inside loop.

            XmlNodeRef keyNode = xmlNode->getChild(i);
            keyNode->getAttr("time", key.time);
            keyNode->getAttr("value", key.value);
            assert(key.time == key.value.x);
            key.time += fTimeOffset;
            key.value.x += fTimeOffset;

            keyNode->getAttr("flags", key.flags);

            SetKey(i + numCur, &key);

            if (bCopySelected)
            {
                SelectKey(i + numCur, true);
            }

            // In-/Out-tangent
            keyNode->getAttr("ds", m_spline->key(i + numCur).ds);
            keyNode->getAttr("dd", m_spline->key(i + numCur).dd);
        }
        SortKeys();
    }
    else
    {
        int num = GetNumKeys();
        xmlNode->setAttr("TrackType", GetCurveType());

        I2DBezierKey key;
        for (int i = 0; i < num; i++)
        {
            GetKey(i, &key);
            assert(key.time == key.value.x);

            if (!bCopySelected || IsKeySelected(i))
            {
                XmlNodeRef keyNode = xmlNode->newChild("Key");
                keyNode->setAttr("time", key.time);
                keyNode->setAttr("value", key.value);

                int flags = key.flags;
                // Just save the in/out mask part. Others are for editing convenience.
                flags &= (SPLINE_KEY_TANGENT_IN_MASK | SPLINE_KEY_TANGENT_OUT_MASK);
                if (flags != 0)
                {
                    keyNode->setAttr("flags", flags);
                }

                // We also have to save in-/out-tangents, because TCB infos are not used for custom tangent keys.
                keyNode->setAttr("ds", m_spline->key(i).ds);
                keyNode->setAttr("dd", m_spline->key(i).dd);
            }
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
template<>
inline void TUiAnimSplineTrack<Vec2>::GetKeyInfo(int index, const char*& description, float& duration)
{
    duration = 0;

    static char str[64];
    description = str;
    assert(index >= 0 && index < GetNumKeys());
    Spline::key_type& k = m_spline->key(index);
    sprintf_s(str, "%.2f", k.value.y);
}

//////////////////////////////////////////////////////////////////////////
typedef UiSpline::BezierSpline<Vec2, UiSpline::SplineKeyEx<Vec2> > BezierSplineVec2;
typedef UiSpline::TSpline<UiSpline::SplineKeyEx<Vec2>, spline::BezierBasis> TSplineBezierBasisVec2;

//////////////////////////////////////////////////////////////////////////
namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(UiSpline::TrackSplineInterpolator<Vec2>, "{38F814D4-6041-4442-9704-9F68E996D55B}");
    AZ_TYPE_INFO_SPECIALIZE(UiSpline::SplineKey<Vec2>, "{E2301E81-6BAF-4A17-886C-76F1A9C37118}");
    AZ_TYPE_INFO_SPECIALIZE(UiSpline::SplineKeyEx<Vec2>, "{1AE37C63-D5C2-4E65-A08B-7020E7696233}");
    AZ_TYPE_INFO_SPECIALIZE(BezierSplineVec2, "{EC8BA7BD-EF3B-453A-8017-CD1BF5B7C011}");
    AZ_TYPE_INFO_SPECIALIZE(TSplineBezierBasisVec2, "{B661D05E-B912-4BD9-B102-FA82938243A9}");
}

namespace UiSpline
{
    //////////////////////////////////////////////////////////////////////////
    template <>
    inline void TSplineBezierBasisVec2::Reflect(AZ::SerializeContext* serializeContext)
    {
        serializeContext->Class<TSplineBezierBasisVec2>()
            ->Version(1)
            ->Field("Keys", &BezierSplineVec2::m_keys);
    }

    //////////////////////////////////////////////////////////////////////////
    template <>
    inline void BezierSplineVec2::Reflect(AZ::SerializeContext* serializeContext)
    {
        TSplineBezierBasisVec2::Reflect(serializeContext);

        serializeContext->Class<BezierSplineVec2, TSplineBezierBasisVec2>()
            ->Version(1)
            ->SerializerForEmptyClass();
    }
}

//////////////////////////////////////////////////////////////////////////
template<>
inline void TUiAnimSplineTrack<Vec2>::Reflect(AZ::SerializeContext* serializeContext)
{
    UiSpline::SplineKey<Vec2>::Reflect(serializeContext);
    UiSpline::SplineKeyEx<Vec2>::Reflect(serializeContext);

    UiSpline::TrackSplineInterpolator<Vec2>::Reflect(serializeContext);
    BezierSplineVec2::Reflect(serializeContext);

    serializeContext->Class<TUiAnimSplineTrack<Vec2> >()
        ->Version(1)
        ->Field("Flags", &TUiAnimSplineTrack<Vec2>::m_flags)
        ->Field("DefaultValue", &TUiAnimSplineTrack<Vec2>::m_defaultValue)
        ->Field("ParamType", &TUiAnimSplineTrack<Vec2>::m_nParamType)
        ->Field("ParamData", &TUiAnimSplineTrack<Vec2>::m_componentParamData)
        ->Field("Spline", &TUiAnimSplineTrack<Vec2>::m_spline);
}

