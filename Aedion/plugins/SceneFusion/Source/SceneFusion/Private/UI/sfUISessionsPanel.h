#pragma once

#include "sfUIPanel.h"
#include "../sfProjectInfo.h"
#include "../sfSessionInfo.h"
#include <Runtime/Slate/Public/Framework/SlateDelegates.h>
#include <Widgets/Views/SListView.h>
#include <Widgets/Input/SComboBox.h>
#include <Widgets/Input/SEditableTextBox.h>
#include <Runtime/Networking/Public/Common/UdpSocketReceiver.h>
#include <Runtime/Networking/Public/Common/UdpSocketBuilder.h>
#include <Runtime/Networking/Public/Networking.h>

/**
 * Scene Fusion sessions list
 */
class sfUISessionsPanel : public sfUIPanel
{
public:
    /**
     * Delegate for UI logout
     */
    DECLARE_DELEGATE(OnLogoutDelegate);

    /**
     * Delegate for join/start sessions
     *
     * @param    TSharedPtr<sfSessionInfo> - session information
     */
    DECLARE_DELEGATE_OneParam(OnJoinSessionDelegate, TSharedPtr<sfSessionInfo>);

    /**
     * Constructor
     */
    sfUISessionsPanel();

    /**
     * Destructor
     */
    virtual ~sfUISessionsPanel();

    /**
     * Show the UI widget
     */
    virtual void Show() override;
    virtual void Hide() override;

    /**
     * Logout event handler
     */
    OnLogoutDelegate OnLogout;

    /**
     * Join session event handler
     */
    OnJoinSessionDelegate OnJoinSession;

    /**
     * Start session event handler
     */
    OnJoinSessionDelegate OnStartSession;
private:
    // UI widgets
    TSharedPtr<SComboBox<TSharedPtr<FString>>> m_accountComboPtr;
    TSharedPtr<SComboBox<TSharedPtr<sfProjectInfo>>> m_projectComboPtr;
    TSharedPtr<SListView<TSharedPtr<sfSessionInfo>>> m_sessionListPtr;
    TSharedPtr<STextBlock> m_selectedAccountPtr;
    TSharedPtr<STextBlock> m_selectedProjectPtr;

    // Session Data
    FString m_project;
    TArray<TSharedPtr<FString>> m_accounts;
    TArray<TSharedPtr<sfProjectInfo>> m_projects;
    TArray<TSharedPtr<sfSessionInfo>> m_sessions;
    TMap<uint32, TSharedPtr<sfSessionInfo>> m_lanSessions;
    ProjectMap m_projectMap;

    //LAN
    TSharedPtr<SEditableTextBox> m_portPtr;
    TSharedPtr<SEditableTextBox> m_manualAddressPtr;
    TSharedPtr<SEditableTextBox> m_manualPortPtr;
    FProcHandle m_processHandle;
    FUdpSocketReceiver* m_socketReceiverPtr;
    FSocket* m_socket;

    /**
     * Send a web request to join a sesssion
     *
     * @param   TSharedPtr<sfSessionInfo> sessionPtr
     */
    void JoinSession(TSharedPtr<sfSessionInfo> sessionPtr);

    /**
     * Handle a select account/logout UI action
     *
     * @param   TSharedPtr<FString> - account name
     * @param   ESelectInfo::Type - selection type
     */
    void SelectAccount(TSharedPtr<FString> valuePtr, ESelectInfo::Type selectType);

    /**
     * Handle a select account/logout UI action
     *
     * @param   TSharedPtr<sfProjectInfo> - project information
     * @param   ESelectInfo::Type - selection type
     */
    void SelectProject(TSharedPtr<sfProjectInfo> valuePtr, ESelectInfo::Type selectType);

    /**
     * Send a web request to start a new session
     *
     * @return  FReply - UI event reply
     */
    FReply StartSession();

    /**
     * Handle the reply from a StartSession web request.
     *
     * @param   TSharedPtr<sfSessionInfo> - session information
     * @param   const FString& - error message
     */
    void StartSessionReply(TSharedPtr<sfSessionInfo> sessionInfoPtr, const FString& error);

    /**
     * Handle the reply from a GetSessions web request
     *
     * @param   ProjectMap& - project information
     * @param   const FString& - error message
     */
    void GetSessionsReply(ProjectMap& projectMap, const FString& error);

    /**
     * Redraw the session list
     */
    void RedrawSessionsList();

    /**
     * UI for account info.
     */
    void AccountInfoArea();

    /**
     * UI for sessions.
     */
    void SessionsArea();

    /**
     * UI for manual session connections.
     */
    void ManualConnectArea();

    /**
     * Start a LAN session.
     * 
     * @return  FReply
     */
    FReply StartLANSession();

    /**
     * Initialize a UDP socket used to receive SF LAN server broadcasts.
     */
    void InitializeUDP();

    /**
     * Cleanup the UDP sockets used to receive SF LAN server broadcasts.
     */
    void CleanupUDP();

    /**
     * Handle the reception of a UDP broadcast
     *
     * @param   const FArrayReaderPtr& - UDP packet data
     * @param   const FIPv4Endpoint& - UDP packet sender endpoint info
     */
    void OnBroadcastRecieved(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt);

    /**
     * Manually connect to a session.
     * 
     * @return  FReply
     */
    FReply ManualConnect();
};