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

#include "DockWidgetUtils.h"
#include "Include/EditorCoreAPI.h"
#include <QtViewPane.h>
#include "Resource.h"
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzQtComponents/Components/DockTabWidget.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <QObject>
#include <QVector>
#include <QPointer>
#include <QSettings>
#include <QAction>
#include <QByteArray>

#include <LyViewPaneNames.h>

#include <AzCore/std/functional.h>

class QMainWindow;
struct ViewLayoutState;
class FancyDocking;

// Widget Names/Contexts supporting DragAndDrop with the DragAndDropEvents Bus
namespace DragAndDropContexts {
    static const AZ::Crc32 MainWindow = AZ_CRC("MainWindow", 0xa280a607);
}

typedef AZStd::function<QWidget*()> ViewPaneFactory;

class DockWidget
    : public AzQtComponents::StyledDockWidget
{
    Q_OBJECT
public:
    explicit DockWidget(QWidget* widget, QtViewPane* pane, QSettings* settings, QMainWindow* parent, FancyDocking* advancedDockManager);

    QString PaneName() const;
    void RestoreState(bool forceDefault = false);

    /**
     * Gets the setting name for a given pane.
     */
    static QString settingsKey(const QString& paneName);

protected:
    bool event(QEvent* qtEvent) override;
private:
    void reparentToMainWindowFix();
    QRect ProperGeometry() const;
    QString settingsKey() const;
    QSettings* const m_settings;
    QMainWindow* const m_mainWindow;
    QtViewPane* const m_pane;
    FancyDocking* m_advancedDockManager;
};

struct QtViewPane
{
    enum class OpenMode
    {
        None                = 0x0,
        UseDefaultState     = 0x1, // Use default geometry and docking position when opening
        MultiplePanes       = 0x2,
        RestoreLayout       = 0x4,
        OnlyOpen            = 0x8,
    };
    Q_DECLARE_FLAGS(OpenModes, OpenMode)

    enum class CloseMode
    {
        None                = 0x0,
        Destroy             = 0x1, // Destroy window when closing it
        Force               = 0x2 // Force the dialog to close instead of querying the view if we can close
    };
    Q_DECLARE_FLAGS(CloseModes, CloseMode)

    int m_id; // between ID_VIEW_OPENPANE_FIRST and ID_VIEW_OPENPANE_LAST
    QString m_name;
    QString m_category;
    ViewPaneFactory m_factoryFunc;
    QPointer<DockWidget> m_dockWidget;
    AzToolsFramework::ViewPaneOptions m_options;

    bool IsValid() const
    {
        return m_id >= ID_VIEW_OPENPANE_FIRST && m_id <= ID_VIEW_OPENPANE_LAST && !m_name.isEmpty();
    }

    bool IsVisible() const
    {
        return m_dockWidget && m_dockWidget->isVisible();
    }

    bool IsConstructed() const
    {
        return m_dockWidget != nullptr;
    }

    QWidget* Widget() const
    {
        return IsConstructed() ? m_dockWidget->widget() : nullptr;
    }

    bool IsViewportPane() const
    {
        return m_category == QLatin1String("Viewport") && m_options.viewportType != -1;
    }

    bool IsPreview() const
    {
        return m_options.isPreview;
    }

    bool IsTabbed() const;
    AzQtComponents::DockTabWidget* ParentTabWidget() const;

    bool Close(CloseModes = CloseMode::Destroy);
};

typedef QVector<QtViewPane> QtViewPanes;

class EDITOR_CORE_API QtViewPaneManager
    : public QObject
{
    Q_OBJECT
public:
    explicit QtViewPaneManager(QObject* parent = nullptr);
    ~QtViewPaneManager();
    void SetMainWindow(QMainWindow*, QSettings* settings, const QByteArray& lastMainWindowState, bool useNewDocking, bool enableLegacyCryEntities);
    void RegisterPane(const QString &name, const QString &category, ViewPaneFactory, const AzToolsFramework::ViewPaneOptions& = {});
    void UnregisterPane(const QString& name);
    QtViewPane* GetPane(int id);
    QtViewPane* GetPane(const QString& name);
    QtViewPane* GetViewportPane(int viewportType);
    QDockWidget* GetView(const QString& name);
    bool IsVisible(const QString& name);

    /**
     * Constructs and shows a view pane.
     * The pane is a QDockWidget who's widget was created with QtViewPane::m_factoryFunc.
     * If useDefaultState is true, the default docking area and geometry are used, not the last one.
     *
     * Returns the view on success, nullptr otherwise
     */
    const QtViewPane* OpenPane(const QString& name, QtViewPane::OpenModes = QtViewPane::OpenMode::None);
    bool ClosePane(const QString& name, QtViewPane::CloseModes = QtViewPane::CloseMode::None);

    /**
     * If the pane is not visible, it will be opened and made visible.
     * If the pane is visible, it will be closed.
     */
    void TogglePane(const QString& name);
    bool CloseAllPanes();

    /**
     * Closes all non standard panes. Standard panes are for example rollup and console.
     */
    void CloseAllNonStandardPanes();

    /**
     * Creates and returns a widget by calling QtViewPane::m_factoryFunc() for the view pane with name paneName.
     * This is similar to OpenPane(), except that there's no dock widget involved. The widget will be used in a
     * CLayoutViewPane (the embedded viewports).
     *
     * Returns nullptr if the specified pane name is not registered.
     */
    QWidget* CreateWidget(const QString& paneName);

    void RestoreLayout();
    bool RestoreLayout(QString name);
    void RestoreDefaultLayout(bool resetSettings = false);
    void RestoreLegacyLayout();
    void SaveLayout();
    void SaveLayout(QString name);
    void RenameLayout(QString name, QString newName);
    void RemoveLayout(QString name);
    bool HasLayout(const QString& name) const;
    QStringList LayoutNames(bool userLayoutsOnly = true) const;

    void SerializeLayout(XmlNodeRef& parentNode) const;
    bool DeserializeLayout(const XmlNodeRef& parentNode);

    static QtViewPaneManager* instance();

    /**
     * Returns the known view panes (regardless of them being open or not).
     * If viewPaneMenuOnly is true, only those appearing in "View->Open View Pane" will be show,
     * meaning panes such as the rollup bar or console aren't returned.
     */
    QtViewPanes GetRegisteredPanes(bool viewPaneMenuOnly = true) const;
    QtViewPanes GetRegisteredMultiInstancePanes(bool viewPaneMenuOnly = true) const;
    QtViewPanes GetRegisteredViewportPanes() const; // only returns the Top/Bottom/Left etc. ones

    //! Attempts to closes everything not in the input list. Returns false if any failed, and restores all previously opened windows if it does. Returns true otherwise
    bool ClosePanesWithRollback(const QVector<QString>& panesToKeepOpen);

signals:
    void savedLayoutsChanged();
    void layoutReset();
    void viewPaneCreated(const QtViewPane* pane);
    void registeredPanesChanged();

private:

    ViewLayoutState GetLayout() const;
    bool RestoreLayout(const ViewLayoutState& state);
    void SaveStateToLayout(const ViewLayoutState& state, const QString& layoutName);

    bool ClosePane(QtViewPane* pane, QtViewPane::CloseModes closeModes = QtViewPane::CloseMode::None);
    int NextAvailableId();
    QtViewPanes m_registeredPanes;
    QByteArray m_defaultMainWindowState;
    QByteArray m_loadedMainWindowState;
    QMainWindow* m_mainWindow;
    QSettings* m_settings;
    QList<int> m_knownIdsSet; // Semantically a set, but QList is faster for small collections than QSet
    bool m_restoreInProgress;

    bool m_useNewDocking;
    bool m_enableLegacyCryEntities;
    FancyDocking* m_advancedDockManager;
};

template<class TWidget>
bool RegisterQtViewPane(IEditor* editor, const QString& name, const QString& category, const AzToolsFramework::ViewPaneOptions& options = {})
{
    QtViewPaneManager::instance()->RegisterPane(name, category, []() { return new TWidget(); }, options);
    return true;
}

template<class TWidget>
bool RegisterQtViewPaneWithName(IEditor* editor, const QString& name, const QString& category, const AzToolsFramework::ViewPaneOptions& options = {})
{
    QtViewPaneManager::instance()->RegisterPane(name, category, [name]() { return new TWidget(name); }, options);
    return true;
}

template<class TWidget>
void UnregisterQtViewPane()
{
    // always close any views that the pane is responsible for before you remove it!
    GetIEditor()->CloseView(CQtViewClass<TWidget>::GetClassID());
    GetIEditor()->GetClassFactory()->UnregisterClass(CQtViewClass<TWidget>::GetClassID());
}

template<typename TWidget>
TWidget* FindViewPane(const QString& name)
{
    QtViewPane* pane = QtViewPaneManager::instance()->GetPane(name);
    return pane ? qobject_cast<TWidget*>(pane->Widget()) : nullptr;
}

Q_DECLARE_OPERATORS_FOR_FLAGS(QtViewPane::OpenModes)
Q_DECLARE_OPERATORS_FOR_FLAGS(QtViewPane::CloseModes)


