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

#include <QGraphicsLinearLayout>
#include <QGraphicsWidget>

#include <AzCore/Component/Component.h>

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeLayoutBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <Styling/StyleHelper.h>

namespace GraphCanvas
{
    class GeneralSlotLayoutGraphicsWidget;

    //! Lays out the slots for the General Node
    class GeneralSlotLayoutComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(GeneralSlotLayoutComponent, "{F6554B50-A42A-4C79-8B1D-547EEA1EA52D}");
        static void Reflect(AZ::ReflectContext*);

        GeneralSlotLayoutComponent();
        ~GeneralSlotLayoutComponent();

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("GraphCanvas_SlotsContainerService", 0x948b6696));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incombatible)
        {
            incombatible.push_back(AZ_CRC("GraphCanvas_SlotsContainerService", 0x948b6696));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("GraphCanvas_StyledGraphicItemService", 0xeae4cdf4));
            required.push_back(AZ_CRC("GraphCanvas_SceneMemberService", 0xe9759a2d));
        }

        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // NodeSlotsRequestBus
        QGraphicsWidget* GetGraphicsWidget();
        ////

    private:
        GeneralSlotLayoutComponent(const GeneralSlotLayoutComponent&) = delete;

        bool                      m_enableDividers;
        SlotGroupConfigurationMap m_slotGroupConfigurations;

        friend class GeneralSlotLayoutGraphicsWidget;
        GeneralSlotLayoutGraphicsWidget* m_nodeSlotsUi;
    };

    //! The slots QGraphicsWidget for displaying a the node slots
    //! QtWidgets cannot be serialized out, so the component wrapper
    //! stores the actual configuration map for serialization.
    class GeneralSlotLayoutGraphicsWidget
        : public QGraphicsWidget
        , public SlotLayoutRequestBus::Handler
        , public NodeSlotsRequestBus::Handler
        , public NodeNotificationBus::Handler
        , public StyleNotificationBus::Handler
        , public SceneMemberNotificationBus::Handler
    {
    public:
        class LayoutDividerWidget
            : public QGraphicsWidget
        {
        public:
            AZ_CLASS_ALLOCATOR(LayoutDividerWidget, AZ::SystemAllocator, 0);
            LayoutDividerWidget(QGraphicsItem* parent);

            void UpdateStyle(const Styling::StyleHelper& styleHelper);
        };

        class LinearSlotGroupWidget
            : public QGraphicsWidget
        {
        public:
            AZ_CLASS_ALLOCATOR(LinearSlotGroupWidget, AZ::SystemAllocator, 0);
            LinearSlotGroupWidget(QGraphicsItem* parent);

            void DisplaySlot(const AZ::EntityId& slotId);
            void RemoveSlot(const AZ::EntityId& slotId);

            const AZStd::vector< AZ::EntityId >& GetInputSlots() const;
            const AZStd::vector< AZ::EntityId >& GetOutputSlots() const;

            bool IsEmpty() const;
            void UpdateStyle(const Styling::StyleHelper& styleHelper);

        private:

            QGraphicsLayoutItem* GetLayoutItem(const AZ::EntityId& slotId) const;

            QGraphicsLinearLayout* m_inputs;
            AZStd::vector< AZ::EntityId > m_inputSlots;

            QGraphicsLinearLayout* m_outputs;
            AZStd::vector< AZ::EntityId > m_outputSlots;
        };

    public:
        AZ_TYPE_INFO(GeneralSlotLayoutGraphicsWidget, "{9DE7D3C0-D88C-47D8-85D4-5E0F619E60CB}");
        AZ_CLASS_ALLOCATOR(GeneralSlotLayoutGraphicsWidget, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context) = delete;

        GeneralSlotLayoutGraphicsWidget(GeneralSlotLayoutComponent& nodeSlots);
        ~GeneralSlotLayoutGraphicsWidget() override;

        void Activate();
        void Deactivate();

        // NodeNotificationBus
        void OnNodeActivated() override;

        void OnSlotAdded(const AZ::EntityId& slot) override;        
        void OnSlotRemoved(const AZ::EntityId& slot) override;
        ////

        // NodeSlotsRequestBus
        QGraphicsLayoutItem* GetGraphicsLayoutItem() override;
        ////

        // SceneMemberNotificationBus
        void OnSceneSet(const AZ::EntityId& sceneId);
        ////

        // SlotLayoutRequestBus
        void SetDividersEnabled(bool enabled) override;
        void ConfigureSlotGroup(SlotGroup group, SlotGroupConfiguration configuration) override;

        void SetSlotGroupVisible(SlotGroup group, bool visible) override;
        void ClearSlotGroup(SlotGroup group);
        ////

        // StyleNotificationBus
        void OnStyleChanged() override;
        ////

    protected:

        // QGraphicsItem
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

        const AZ::EntityId& GetEntityId() const { return m_entityId; }

    private:

        bool DisplaySlot(const AZ::EntityId& slotId);
        bool RemoveSlot(const AZ::EntityId& slotId);

        void ActivateSlots();

        void ClearLayout();
        void UpdateLayout();

        void UpdateStyles();
        void RefreshDisplay();

        LinearSlotGroupWidget* FindCreateSlotGroupWidget(SlotGroup slotType);
        LayoutDividerWidget*   FindCreateDividerWidget(int index);

        GeneralSlotLayoutGraphicsWidget(const GeneralSlotLayoutComponent&) = delete;

        GeneralSlotLayoutComponent& m_nodeSlots;

        QGraphicsLinearLayout* m_groupLayout;

        AZStd::unordered_map< SlotGroup, LinearSlotGroupWidget* > m_slotGroups;
        AZStd::vector< LayoutDividerWidget* > m_dividers;

        Styling::StyleHelper m_styleHelper;
        AZ::EntityId         m_entityId;

        bool                 m_addedToScene;
    };
}