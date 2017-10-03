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

#include "MorphTargetGroupWidget.h"
#include <AzCore/Casting/numeric_cast.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>


namespace EMStudio
{

    MorphTargetGroupWidget::MorphTargetGroupWidget(const char* name, EMotionFX::ActorInstance* actorInstance, const AZStd::vector<EMotionFX::MorphTarget*>& morphTargets, const AZStd::vector<EMotionFX::MorphSetupInstance::MorphTarget*>& morphTargetInstances, QWidget* parent)
        : QWidget(parent)
    {
        // keep values
        mName = name;
        mActorInstance = actorInstance;

        // init the edit window to nullptr
        mEditWindow = nullptr;

        // create the layout
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setSpacing(2);
        layout->setMargin(0);

        // checkbox to enable/disable manual mode for all morph targets
        mSelectAll = new QCheckBox("Select All");
        mSelectAll->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
        connect(mSelectAll, SIGNAL(stateChanged(int)), this, SLOT(SetManualModeForAll(int)));

        // button for resetting all morph targets
        QPushButton* resetAll = new QPushButton("Reset All");
        resetAll->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        connect(resetAll, SIGNAL(clicked()), this, SLOT(ResetAll()));

        // add controls to the top layout
        QHBoxLayout* topControlLayout = new QHBoxLayout();
        topControlLayout->addWidget(mSelectAll);
        topControlLayout->addWidget(resetAll);
        topControlLayout->setSpacing(5);
        topControlLayout->setMargin(0);

        // add the top control layout in the main layout
        layout->addLayout(topControlLayout);

        // create the grid layout to add all morph target in it
        QGridLayout* gridLayout = new QGridLayout();
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setVerticalSpacing(2);

        const size_t numMorphTargets = morphTargets.size();
        mMorphTargets.resize(numMorphTargets);

        for (size_t i=0; i<numMorphTargets; ++i)
        {
            // keep values
            mMorphTargets[i].mMorphTarget = morphTargets[i];
            mMorphTargets[i].mMorphTargetInstance = morphTargetInstances[i];

            // add the number label
            const int intIndex = aznumeric_caster(i);
            QLabel* numberLabel = new QLabel(QString("%1").arg(intIndex + 1));
            gridLayout->addWidget(numberLabel, intIndex, 0);

            // add the manual mode checkbox
            mMorphTargets[i].mManualMode = new QCheckBox();
            mMorphTargets[i].mManualMode->setMaximumWidth(15);
            mMorphTargets[i].mManualMode->setProperty("MorphTargetIndex", intIndex);
            mMorphTargets[i].mManualMode->setStyleSheet("QCheckBox{ spacing: 0px; }");
            gridLayout->addWidget(mMorphTargets[i].mManualMode, intIndex, 1);
            connect(mMorphTargets[i].mManualMode, SIGNAL(clicked()), this, SLOT(ManualModeClicked()));

            // create slider to adjust the morph target
            mMorphTargets[i].mSliderWeight = new MysticQt::FloatSlider();
            mMorphTargets[i].mSliderWeight->setMinimumWidth(50);
            mMorphTargets[i].mSliderWeight->SetSingleStep(0.1f);
            mMorphTargets[i].mSliderWeight->setProperty("MorphTargetIndex", intIndex);
            mMorphTargets[i].mSliderWeight->GetSpinBox()->setMinimumWidth(40);
            mMorphTargets[i].mSliderWeight->GetSpinBox()->setMaximumWidth(40);
            gridLayout->addWidget(mMorphTargets[i].mSliderWeight, intIndex, 2);
            connect(mMorphTargets[i].mSliderWeight, SIGNAL(ValueChanged(float)), this, SLOT(SliderWeightMoved(float)));
            connect(mMorphTargets[i].mSliderWeight, SIGNAL(FinishedValueChange(float)), this, SLOT(SliderWeightReleased(float)));

            // create the name label
            QLabel* nameLabel = new QLabel(morphTargets[i]->GetName());
            gridLayout->addWidget(nameLabel, intIndex, 3);

            // create the edit button
            QPushButton* edit = new QPushButton("Edit");
            edit->setProperty("MorphTargetIndex", intIndex);
            gridLayout->addWidget(edit, intIndex, 4);
            connect(edit, SIGNAL(clicked()), this, SLOT(EditClicked()));
        }

        // add the grid layout in the main layout
        layout->addLayout(gridLayout);

        // set the layout
        setLayout(layout);

        // set the size policy
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    }


    // the destructor
    MorphTargetGroupWidget::~MorphTargetGroupWidget()
    {
        delete mEditWindow;
    }


    // set manual mode for all morph targets
    void MorphTargetGroupWidget::SetManualModeForAll(int value)
    {
        // create our command group
        MCore::CommandGroup commandGroup("Adjust morph targets");
        AZStd::string command;

        // loop trough all morph targets and enable/disable manual mode
        const size_t numMorphTargets = mMorphTargets.size();
        for (size_t i = 0; i < numMorphTargets; ++i)
        {
            EMotionFX::MorphTarget* morphTarget = mMorphTargets[i].mMorphTarget;

            command = AZStd::string::format("AdjustMorphTarget -actorInstanceID %i -lodLevel %i -name \"%s\" -manualMode ", mActorInstance->GetID(), mActorInstance->GetLODLevel(), morphTarget->GetName());
            if (value == Qt::Checked)
            {
                command += "true";
            }
            else
            {
                command += "false";
            }

            commandGroup.AddCommandString(command);
        }

        AZStd::string result;
        if (CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result) == false)
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    // reset all morph targets
    void MorphTargetGroupWidget::ResetAll()
    {
        // create our command group
        MCore::CommandGroup commandGroup("Adjust morph targets");
        AZStd::string command;

        // loop trough all morph targets and enable/disable manual mode
        const size_t numMorphTargets = mMorphTargets.size();
        for (size_t i = 0; i < numMorphTargets; ++i)
        {
            EMotionFX::MorphTarget* morphTarget = mMorphTargets[i].mMorphTarget;

            command = AZStd::string::format("AdjustMorphTarget -actorInstanceID %i -lodLevel %i -name \"%s\" -weight %f", mActorInstance->GetID(), mActorInstance->GetLODLevel(), morphTarget->GetName(), morphTarget->CalcZeroInfluenceWeight());
            commandGroup.AddCommandString(command);
        }

        AZStd::string result;
        if (CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result) == false)
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    // manual mode
    void MorphTargetGroupWidget::ManualModeClicked()
    {
        QCheckBox* checkBox = static_cast<QCheckBox*>(sender());
        const int morphTargetIndex = checkBox->property("MorphTargetIndex").toInt();
        EMotionFX::MorphTarget* morphTarget = mMorphTargets[morphTargetIndex].mMorphTarget;

        AZStd::string result;
        const AZStd::string command = AZStd::string::format("AdjustMorphTarget -actorInstanceID %i -lodLevel %i -name \"%s\" -weight %f -manualMode %s", mActorInstance->GetID(), mActorInstance->GetLODLevel(), morphTarget->GetName(), 0.0f, (checkBox->isChecked()) ? "true" : "false");
        if (EMStudio::GetCommandManager()->ExecuteCommand(command, result) == false)
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    // slider weight moved
    void MorphTargetGroupWidget::SliderWeightMoved(float value)
    {
        // get the morph target
        MysticQt::FloatSlider* floatSlider = static_cast<MysticQt::FloatSlider*>(sender());
        const int morphTargetIndex = floatSlider->property("MorphTargetIndex").toInt();
        EMotionFX::MorphSetupInstance::MorphTarget* morphTargetInstance = mMorphTargets[morphTargetIndex].mMorphTargetInstance;

        // update the weight
        morphTargetInstance->SetWeight(value);
    }


    // slider weight released
    void MorphTargetGroupWidget::SliderWeightReleased(float value)
    {
        // get the morph target and the morph target instance
        MysticQt::FloatSlider* floatSlider = static_cast<MysticQt::FloatSlider*>(sender());
        const int morphTargetIndex = floatSlider->property("MorphTargetIndex").toInt();
        EMotionFX::MorphTarget* morphTarget = mMorphTargets[morphTargetIndex].mMorphTarget;
        EMotionFX::MorphSetupInstance::MorphTarget* morphTargetInstance = mMorphTargets[morphTargetIndex].mMorphTargetInstance;

        // set the old weight to have the undo correct
        morphTargetInstance->SetWeight(mMorphTargets[morphTargetIndex].mOldWeight);

        // execute command
        AZStd::string result;
        const AZStd::string command = AZStd::string::format("AdjustMorphTarget -actorInstanceID %i -lodLevel %i -name \"%s\" -weight %f", mActorInstance->GetID(), mActorInstance->GetLODLevel(), morphTarget->GetName(), value);
        if (EMStudio::GetCommandManager()->ExecuteCommand(command, result) == false)
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // set the new old weight value
        mMorphTargets[morphTargetIndex].mOldWeight = value;
    }


    // edit
    void MorphTargetGroupWidget::EditClicked()
    {
        // get the morph target
        QPushButton* button = static_cast<QPushButton*>(sender());
        const int morphTargetIndex = button->property("MorphTargetIndex").toInt();
        EMotionFX::MorphTarget* morphTarget = mMorphTargets[morphTargetIndex].mMorphTarget;

        // show the edit window
        delete mEditWindow;
        mEditWindow = new MorphTargetEditWindow(mActorInstance, morphTarget, this);
        mEditWindow->exec();
    }


    // updates the interface of the morph target group
    void MorphTargetGroupWidget::UpdateInterface()
    {
        bool selectAllChecked = true;

        const size_t numMorphTargets = mMorphTargets.size();
        for (size_t i = 0; i < numMorphTargets; ++i)
        {
            const float rangeMin    = mMorphTargets[i].mMorphTarget->GetRangeMin();
            const float rangeMax    = mMorphTargets[i].mMorphTarget->GetRangeMax();
            const float weight      = mMorphTargets[i].mMorphTargetInstance->GetWeight();
            const bool  manualMode  = mMorphTargets[i].mMorphTargetInstance->GetIsInManualMode();

            // check if the select all should not be checked
            if (manualMode == false)
            {
                selectAllChecked = false;
            }

            // disable signals
            mMorphTargets[i].mSliderWeight->BlockSignals(true);
            mMorphTargets[i].mManualMode->blockSignals(true);

            // update the manual mode
            mMorphTargets[i].mManualMode->setChecked(manualMode);

            // update the slider weight
            mMorphTargets[i].mSliderWeight->setDisabled(!manualMode);
            mMorphTargets[i].mSliderWeight->SetRange(rangeMin, rangeMax);
            mMorphTargets[i].mSliderWeight->SetValue(weight);

            // enable signals
            mMorphTargets[i].mSliderWeight->BlockSignals(false);
            mMorphTargets[i].mManualMode->blockSignals(false);

            // store the current weight
            // the weight is updated in realtime but before to execute the adjust command it has to be reset to have the undo correct
            mMorphTargets[i].mOldWeight = weight;
        }

        // update the select all
        mSelectAll->blockSignals(true);
        mSelectAll->setChecked(selectAllChecked);
        mSelectAll->blockSignals(false);

        // update the edit window
        if (mEditWindow)
        {
            mEditWindow->UpdateInterface();
        }
    }


    void MorphTargetGroupWidget::UpdateMorphTarget(const char* name)
    {
        // update the row
        const size_t numMorphTargets = mMorphTargets.size();
        for (size_t i = 0; i < numMorphTargets; ++i)
        {
            // continue of the name is not the same
            if (mMorphTargets[i].mMorphTarget->GetNameString() != name)
            {
                continue;
            }

            // get values
            const float rangeMin    = mMorphTargets[i].mMorphTarget->GetRangeMin();
            const float rangeMax    = mMorphTargets[i].mMorphTarget->GetRangeMax();
            const float weight      = mMorphTargets[i].mMorphTargetInstance->GetWeight();
            const bool  manualMode  = mMorphTargets[i].mMorphTargetInstance->GetIsInManualMode();

            // disable signals
            mMorphTargets[i].mSliderWeight->BlockSignals(true);
            mMorphTargets[i].mManualMode->blockSignals(true);

            // update the manual mode
            mMorphTargets[i].mManualMode->setChecked(manualMode);

            // update the slider weight
            mMorphTargets[i].mSliderWeight->setDisabled(!manualMode);
            mMorphTargets[i].mSliderWeight->SetRange(rangeMin, rangeMax);
            mMorphTargets[i].mSliderWeight->SetValue(weight);

            // enable signals
            mMorphTargets[i].mSliderWeight->BlockSignals(false);
            mMorphTargets[i].mManualMode->blockSignals(false);

            // store the current weight
            // the weight is updated in realtime but before to execute the adjust command it has to be reset to have the undo correct
            mMorphTargets[i].mOldWeight = weight;

            // update edit window in case it's the edit of this morph target
            if (mEditWindow && mEditWindow->GetMorphTarget() == mMorphTargets[i].mMorphTarget)
            {
                mEditWindow->UpdateInterface();
            }

            // stop here because we found it
            break;
        }

        // update the select all
        bool selectAllChecked = true;
        for (uint32 i = 0; i < numMorphTargets; ++i)
        {
            if (mMorphTargets[i].mMorphTargetInstance->GetIsInManualMode() == false)
            {
                selectAllChecked = false;
                break;
            }
        }
        mSelectAll->blockSignals(true);
        mSelectAll->setChecked(selectAllChecked);
        mSelectAll->blockSignals(false);
    }
} // namespace EMStudio


#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MorphTargetsWindow/MorphTargetGroupWidget.moc>