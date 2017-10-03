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

#pragma once

#include "EMotionFXConfig.h"
#include "AnimGraphObject.h"


namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;

    class EMFX_API AnimGraphTransitionCondition
        : public AnimGraphObject
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphTransitionCondition, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_CONDITIONS);

    public:
        AZ_RTTI(AnimGraphTransitionCondition, "{DD14D0C7-AC88-4F90-BB4C-0F6810A6BAE7}", AnimGraphObject);

        enum
        {
            BASETYPE_ID = 0x00000003
        };

        AnimGraphTransitionCondition(AnimGraph* animGraph, uint32 typeID);
        virtual ~AnimGraphTransitionCondition();

        virtual bool TestCondition(AnimGraphInstance* animGraphInstance) const = 0;
        virtual void Reset(AnimGraphInstance* animGraphInstance)       { MCORE_UNUSED(animGraphInstance); }

        virtual AnimGraphObject* RecursiveClone(AnimGraph* animGraph, AnimGraphObject* parentObject) override;

        uint32 GetBaseType() const override;
        ECategory GetPaletteCategory() const override;

        void UpdatePreviousTestResult(AnimGraphInstance* animGraphInstance, bool newTestResult);

    protected:
        bool mPreviousTestResult; /**< Result of the last TestCondition() call. */
    };
} // namespace EMotionFX