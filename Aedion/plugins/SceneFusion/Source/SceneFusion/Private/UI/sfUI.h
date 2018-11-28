#pragma once

#include "sfUISessionsPanel.h"
#include "sfUILoginPanel.h"
#include "sfUIOnlinePanel.h"
#include "sfOutlinerManager.h"
#include "../sfSessionInfo.h"
#include <sfSession.h>
#include <ksEvent.h>
#include <CoreMinimal.h>
#include <Widgets/Layout/SWidgetSwitcher.h>

using namespace KS::SceneFusion2;

/**
 * Scene Fusion User Interface
 */
class sfUI : public TSharedFromThis<sfUI>
{
public:
    /**
     * Initialize the styles and UI components used by Scene Fusion.
     */
    void Initialize();

    /**
     * Clean up the styles and UI components used by Scene Fusion.
     */
    void Cleanup();

    /**
     * Gets delegate for go to camera.
     *
     * @return sfUIOnlinePanel::OnGoToDelegate&
     */
    sfUIOnlinePanel::OnGoToDelegate& OnGoToUser();
    
    /**
     * Gets delegate for follow camera.
     *
     * @return sfUIOnlinePanel::OnFollowDelegate&
     */
    sfUIOnlinePanel::OnFollowDelegate& OnFollowUser();

    /**
     * Unfollows camera.
     */
    void UnfollowCamera();

    /**
     * Connects to a session.
     *
     * @param   TSharedPtr<sfSessionInfo> sessionInfoPtr - determines where to connect.
     */
    void JoinSession(TSharedPtr<sfSessionInfo> sessionInfoPtr);

private:
    // Commands
    TSharedPtr<class FUICommandList> m_UICommandListPtr;

    // UI components
    TSharedPtr<SWidgetSwitcher> m_panelSwitcherPtr;
    TSharedPtr<SWidget> m_activeWidget;
    sfUISessionsPanel m_sessionsPanel;
    sfUIOnlinePanel m_onlinePanel;
    sfUILoginPanel m_loginPanel;

    // Event pointers
    KS::ksEvent<sfSession::SPtr&, const std::string&>::SPtr m_disconnectEventPtr;
    KS::ksEvent<sfUser::SPtr&>::SPtr m_userJoinEventPtr;
    KS::ksEvent<sfUser::SPtr&>::SPtr m_userLeaveEventPtr;
    KS::ksEvent<sfUser::SPtr&>::SPtr m_userColorChangeEventPtr;

    TSharedPtr<sfOutlinerManager> m_outlinerManagerPtr;

    /**
     * Initialize styles.
     */
    void InitializeStyles();

    /**
     * Initialise commands.
     */
    void InitializeCommands();

    /**
     * Extend the level editor tool bar with a SF button
     */
    void ExtendToolBar();

    /**
     * Register a SF Tab panel with a tab spawner.
     */
    void RegisterSFTab();

    /**
     * Register Scene Fusion event handlers.
     */
    void RegisterSFHandlers();

    /**
     * Register UI event handlers
     */
    void RegisterUIHandlers();

    /**
     * Show the login panel, and hide other panels.
     */
    void ShowLoginPanel();

    /**
     * Show the sessions panel, and hide other panels.
     */
    void ShowSessionsPanel();

    /**
     * Show the online panel, and hide other panels.
     */
    void ShowOnlinePanel();

    /**
     * Called when a connection attempt completes.
     *
     * @param   sfSession::SPtr sessionPtr we connected to. nullptr if the connection failed.
     * @param   const std::string& errorMessage. Empty string if the connection was successful.
     */
    void OnConnectComplete(sfSession::SPtr sessionPtr, const std::string& errorMessage);

    /**
     * Called when we disconnect from a session.
     *
     * @param   sfSession::SPtr sessionPtr we disconnected from.
     * @param   const std::string& errorMessage. Empty string if no error occurred.
     */
    void OnDisconnect(sfSession::SPtr sessionPtr, const std::string& errorMessage);

    /**
     * Create the widgets used in the toolbar.
     *
     * @param   FToolBarBuilder& - toolbar builder
     */
    void OnExtendToolBar(FToolBarBuilder& builder);

    /**
     * Create tool bar menu.
     * 
     * @return  TSharedRef<SWidget>
     */
    TSharedRef<SWidget> OnCreateToolBarMenu();

    /**
     * Create the Scene Fusion tab.
     * 
     * @param   const FSpawnTabArgs& - tab spawning arguments
     * @return  TSharedRef<SDockTab>
     */
    TSharedRef<SDockTab> OnCreateSFTab(const FSpawnTabArgs& args);
};