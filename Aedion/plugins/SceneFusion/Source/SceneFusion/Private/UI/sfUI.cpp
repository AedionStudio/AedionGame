#include "sfUI.h"
#include "sfUIStyles.h"
#include "sfUICommands.h"
#include "../SceneFusion.h"
#include "../sfConfig.h"
#include "../ObjectManagers/sfActorManager.h"

#include <iostream>

#include <sfService.h>
#include <ksRoomInfo.h>
#include <LevelEditor.h>
#include <Framework/MultiBox/MultiBoxBuilder.h>
#include <Runtime/Core/Public/Misc/MessageDialog.h>
#include <Widgets/Docking/SDockTab.h>
#include <Runtime/Launch/Resources/Version.h>

#define LOG_CHANNEL "sfUI"

using namespace KS;
using namespace KS::SceneFusion2;

static const FName SceneFusionTabName("Scene Fusion");

void sfUI::Initialize()
{
    KS::Log::Info("Scene Fusion intializing UI.", LOG_CHANNEL);

    InitializeStyles();
    InitializeCommands();
    ExtendToolBar();
    RegisterSFTab();
    RegisterSFHandlers();

    m_outlinerManagerPtr = MakeShareable(new sfOutlinerManager);
    SceneFusion::ActorManager->OnLockStateChange.BindRaw(m_outlinerManagerPtr.Get(), &sfOutlinerManager::SetLockState);
}

void sfUI::Cleanup()
{
    KS::Log::Info("Scene Fusion cleanup UI.", LOG_CHANNEL);
    m_activeWidget = nullptr;
    m_panelSwitcherPtr.Reset();
    m_loginPanel.Hide();
    m_sessionsPanel.Hide();
    m_onlinePanel.Hide();

    sfUIStyles::Shutdown();
    sfUICommands::Unregister();
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SceneFusionTabName);
}

void sfUI::InitializeStyles()
{
    sfUIStyles::Initialize();
    sfUIStyles::ReloadTextures();
}

void sfUI::InitializeCommands()
{
    sfUICommands::Register();
    m_UICommandListPtr = MakeShareable(new FUICommandList);

    // Map Scene Fusion actions to UI commands
    m_UICommandListPtr->MapAction(
        sfUICommands::Get().ToolBarClickPtr,
        FExecuteAction::CreateLambda([this]() { FGlobalTabmanager::Get()->InvokeTab(SceneFusionTabName); }),
        FCanExecuteAction()
    );
}

// Add the tool bar button using a ui extender
void sfUI::ExtendToolBar()
{
    FLevelEditorModule& module = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
    TSharedPtr<FExtender> extenderPtr = MakeShareable(new FExtender);
    extenderPtr->AddToolBarExtension(
        "Content",
        EExtensionHook::Position::After,
        m_UICommandListPtr,
        FToolBarExtensionDelegate::CreateRaw(this, &sfUI::OnExtendToolBar)
    );
    module.GetToolBarExtensibilityManager()->AddExtender(extenderPtr);
}

void sfUI::RegisterSFTab()
{
    FTabSpawnerEntry& tabSpawner = FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        SceneFusionTabName,
        FOnSpawnTab::CreateRaw(this, &sfUI::OnCreateSFTab)
    );
    tabSpawner.SetDisplayName(FText::FromString(TEXT("Scene Fusion")));
    tabSpawner.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void sfUI::RegisterSFHandlers()
{
    m_disconnectEventPtr = SceneFusion::Service->RegisterOnDisconnectHandler(
        [this](sfSession::SPtr& sessionPtr, const std::string& errorMessage)
    {
        OnDisconnect(sessionPtr, errorMessage);
    });
}

void sfUI::RegisterUIHandlers()
{
    m_loginPanel.OnLogin.BindRaw(this, &sfUI::ShowSessionsPanel);

    m_sessionsPanel.OnLogout.BindLambda([this]() {
        SceneFusion::WebService->Logout(sfBaseWebService::OnLogoutDelegate::CreateLambda([this]() {
            ShowLoginPanel(); 
        }));
    });
    
    m_sessionsPanel.OnStartSession.BindLambda([this](TSharedPtr<sfSessionInfo> sessionInfoPtr)
    {
        SceneFusion::IsSessionCreator = true;
        JoinSession(sessionInfoPtr);
    });

    m_sessionsPanel.OnJoinSession.BindLambda([this](TSharedPtr<sfSessionInfo> sessionInfoPtr)
    {
        SceneFusion::IsSessionCreator = false;
        JoinSession(sessionInfoPtr);
    });

    m_onlinePanel.OnLeaveSession.BindLambda([this]()
    {
        SceneFusion::Service->LeaveSession();
    });

    FTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateLambda([this](float time)->bool {
            if (SceneFusion::WebService->IsLoggedIn()) {
                SceneFusion::WebService->RefreshToken(
                    sfBaseWebService::OnRefreshTokenDelegate::CreateLambda([this](const FString& error) {
                        if (!error.IsEmpty()) {
                            KS::Log::Error("Error refreshing Scene Fusion authentication token: " +
                                std::string(TCHAR_TO_UTF8(*error)), LOG_CHANNEL);
                            m_sessionsPanel.OnLogout.Execute();
                        }
                    })
                );
            }
            return true; // returning true will reschedule this delegate.
        }),
        30.0f // delay period
     );
}

void sfUI::ShowLoginPanel()
{   
    m_loginPanel.Show();
    m_sessionsPanel.Hide();
    m_onlinePanel.Hide();
    m_activeWidget = m_loginPanel.Widget();
    m_panelSwitcherPtr->SetActiveWidget(m_loginPanel.Widget());
}

void sfUI::ShowSessionsPanel()
{
    m_loginPanel.Hide();
    m_sessionsPanel.Show();
    m_onlinePanel.Hide();
    m_activeWidget = m_sessionsPanel.Widget();
    m_panelSwitcherPtr->SetActiveWidget(m_sessionsPanel.Widget());
}

void sfUI::ShowOnlinePanel()
{
    m_loginPanel.Hide();
    m_sessionsPanel.Hide();
    m_onlinePanel.Show();
    m_activeWidget = m_onlinePanel.Widget();
    m_panelSwitcherPtr->SetActiveWidget(m_onlinePanel.Widget());
}

void sfUI::JoinSession(TSharedPtr<sfSessionInfo> sessionInfoPtr)
{
    FString version = "Unreal Engine " + FString(ENGINE_VERSION_STRING);
    std::string token(TCHAR_TO_UTF8(*sfConfig::Get().SFToken));
    std::string username(TCHAR_TO_UTF8(*sfConfig::Get().Name));
    std::string application(TCHAR_TO_UTF8(*version));
    SceneFusion::Service->JoinSession(sessionInfoPtr->RoomInfoPtr, token, username, application,
        [this](sfSession::SPtr sessionPtr, const std::string& errorMessage)
    {
        OnConnectComplete(sessionPtr, errorMessage);
    });
}

void sfUI::OnConnectComplete(sfSession::SPtr sessionPtr, const std::string& errorMessage)
{
    m_sessionsPanel.Enable();
    if (sessionPtr != nullptr)
    {
        // UI Session Event Registration
        m_userJoinEventPtr = sessionPtr->RegisterOnUserJoinHandler(
            [this](sfUser::SPtr value) { m_onlinePanel.AddUser(std::move(value)); }
        );
        
        m_userLeaveEventPtr = sessionPtr->RegisterOnUserLeaveHandler(
            [this](sfUser::SPtr value) { m_onlinePanel.RemoveUser(std::move(value)); }
        );

        m_userColorChangeEventPtr = sessionPtr->RegisterOnUserColorChangeHandler(
            [this](sfUser::SPtr value) { m_onlinePanel.UpdateUserColor(std::move(value)); }
        );

        ShowOnlinePanel();
        SceneFusion::OnConnect();
        m_outlinerManagerPtr->Initialize();
        sfDetailsPanelManager::Get().Initialize();
    }
    else if (!errorMessage.empty())
    {
        m_sessionsPanel.DisplayMessage(FString(UTF8_TO_TCHAR(errorMessage.c_str())), sfUIMessageBox::ERROR);
    }
}

void sfUI::OnDisconnect(sfSession::SPtr sessionPtr, const std::string& errorMessage)
{
    sessionPtr->UnregisterOnUserJoinHandler(m_userJoinEventPtr);
    sessionPtr->UnregisterOnUserLeaveHandler(m_userLeaveEventPtr);
    sessionPtr->UnregisterOnUserColorChangeHandler(m_userColorChangeEventPtr);

    ShowSessionsPanel();
    m_onlinePanel.ClearUsers();
    if (!errorMessage.empty())
    {
        m_sessionsPanel.DisplayMessage(FString(UTF8_TO_TCHAR(errorMessage.c_str())), sfUIMessageBox::ERROR);
    }
    SceneFusion::OnDisconnect();
    m_outlinerManagerPtr->CleanUp();
    sfDetailsPanelManager::Get().CleanUp();
}

void sfUI::OnExtendToolBar(FToolBarBuilder& builder)
{
    TAttribute<FText> label;
    label.Set(FText::FromString(TEXT("Scene Fusion Options")));

    builder.AddToolBarButton(sfUICommands::Get().ToolBarClickPtr);

    // TODO: Add drop down menu with other SF links 
    //builder.AddComboButton(
    //    FUIAction(),
    //    FOnGetContent::CreateRaw(this, &sfUI::OnCreateToolBarMenu),
    //    label,
    //    label,
    //    FSlateIcon(),
    //    true
    //);
}

TSharedRef<SWidget> sfUI::OnCreateToolBarMenu()
{
    TSharedPtr<FExtender> extender = MakeShareable(new FExtender);
    FMenuBuilder builder(true, m_UICommandListPtr, extender);

    // Add menu bar commands
    builder.AddMenuEntry(
        sfUICommands::Get().ToolBarClickPtr,
        NAME_None,
        TAttribute<FText>(),
        TAttribute<FText>(),
        FSlateIcon()
    );

    return builder.MakeWidget();
}

TSharedRef<SDockTab> sfUI::OnCreateSFTab(const FSpawnTabArgs& args)
{
    auto tab = SNew(SDockTab)
        .Icon(sfUIStyles::Get().GetBrush("SceneFusion.TabIcon"))
        .TabRole(NomadTab)
        [
            SAssignNew(m_panelSwitcherPtr, SWidgetSwitcher)
        ];
    RegisterUIHandlers();

    m_panelSwitcherPtr->AddSlot(0).AttachWidget(m_loginPanel.Widget());
    m_panelSwitcherPtr->AddSlot(1).AttachWidget(m_sessionsPanel.Widget());
    m_panelSwitcherPtr->AddSlot(2).AttachWidget(m_onlinePanel.Widget());

    if (m_activeWidget.IsValid()) {
        m_panelSwitcherPtr->SetActiveWidget(m_activeWidget.ToSharedRef());
    }
    else {
        ShowLoginPanel();
        m_loginPanel.Authenticate();
    }

    return tab;
}

sfUIOnlinePanel::OnGoToDelegate& sfUI::OnGoToUser()
{
    return m_onlinePanel.OnGoTo;
}

sfUIOnlinePanel::OnFollowDelegate& sfUI::OnFollowUser()
{
    return m_onlinePanel.OnFollow;
}

void sfUI::UnfollowCamera()
{
    m_onlinePanel.UnfollowCamera();
}

#undef LOG_CHANNEL