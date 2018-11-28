#include "sfUISessionsPanel.h"
#include "sfUISessionRow.h"
#include "../SceneFusion.h"
#include "../sfConfig.h"

#include <Editor.h>
#include <Widgets/Input/SButton.h>
#include <Widgets/Input/SHyperlink.h>
#include <Widgets/Layout/SExpandableArea.h>

#define LOG_CHANNEL "sfUISessionsPanel"
#define LAN_SERVER_FOLDER "KSServer/"
#define LAN_SERVER_EXECUTEBLE "Reactor.exe"
#define LAN_CONFIG "config.json"
#define GAME_JSON "sf_game.json"
#define SCENE_JSON "sf_scene.json"

sfUISessionsPanel::sfUISessionsPanel() : sfUIPanel("Sessions")
{
    m_socket = nullptr;
    m_socketReceiverPtr = nullptr;

    InitializeUDP();
    AccountInfoArea();
    SessionsArea();
    ManualConnectArea();
    
    // Refresh session list periodically
    FTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateLambda([this](float time)->bool {
            RedrawSessionsList();
            if (m_widgetPtr.IsValid() && m_widgetPtr->GetVisibility() == EVisibility::Visible) {
                SceneFusion::WebService->GetSessions(
                    sfBaseWebService::OnGetSessionsDelegate::CreateRaw(this, &sfUISessionsPanel::GetSessionsReply)
                );
            }
            return true; // returning true will reschedule this delegate.
        }),
        3.0f // delay in seconds
    );
}

sfUISessionsPanel::~sfUISessionsPanel()
{
    CleanupUDP();
}

void sfUISessionsPanel::AccountInfoArea()
{
    m_contentPtr->AddSlot().HAlign(HAlign_Fill).VAlign(VAlign_Center).Padding(5, 2).AutoHeight()
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Center)
        [
            SAssignNew(m_accountComboPtr, SComboBox<TSharedPtr<FString>>)
            .OptionsSource(&m_accounts)
            .OnSelectionChanged(SComboBox<TSharedPtr<FString>>::FOnSelectionChanged::CreateRaw(this, &sfUISessionsPanel::SelectAccount))
            .OnGenerateWidget(SComboBox<TSharedPtr<FString>>::FOnGenerateWidget::CreateLambda(
                [this](TSharedPtr<FString> valuePtr)->TSharedRef<SWidget> {
                    return SNew(SBox)
                    .Padding(FMargin(2, 2))
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock).Text(FText::FromString(*valuePtr))
                    ];
                }
            ))
            .Content()
            [
                SAssignNew(m_selectedAccountPtr, STextBlock)
            ]
        ]
        + SHorizontalBox::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Center)
        [
            SAssignNew(m_projectComboPtr, SComboBox<TSharedPtr<sfProjectInfo>>)
            .OptionsSource(&m_projects)
            .OnSelectionChanged(SComboBox<TSharedPtr<sfProjectInfo>>::FOnSelectionChanged::CreateRaw(this, &sfUISessionsPanel::SelectProject))
            .OnGenerateWidget(SComboBox<TSharedPtr<sfProjectInfo>>::FOnGenerateWidget::CreateLambda(
                [this](TSharedPtr<sfProjectInfo> valuePtr)->TSharedRef<SWidget> {
                    return SNew(SBox)
                    .Padding(FMargin(2, 2))
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock).Text(FText::FromString(*valuePtr->Name))
                    ];
                }
            ))
            .Content()
            [
                SAssignNew(m_selectedProjectPtr, STextBlock)
            ]
        ]
    ];
}

void sfUISessionsPanel::SessionsArea()
{
    // Online session
    m_contentPtr->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(5, 10, 5, 5).AutoHeight()
    [
        SNew(SBox)
        .WidthOverride(150)
        [
            SNew(SButton)
            .HAlign(HAlign_Center)
            .Text(FText::FromString("Start Online Session"))
            .OnClicked(FOnClicked::CreateRaw(this, &sfUISessionsPanel::StartSession))
        ]
    ];

    // LAN session
    m_contentPtr->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(5, 5).AutoHeight()
    [
        SNew(SHorizontalBox)
        .IsEnabled_Lambda([this]() -> bool {
            return FPaths::DirectoryExists(FPaths::ProjectDir() + LAN_SERVER_FOLDER);
        })
        + SHorizontalBox::Slot().AutoWidth()
        [
            SNew(SBox)
            .WidthOverride(150)
            [
                SNew(SButton)
                .HAlign(HAlign_Center)
                .Text(FText::FromString("Start LAN Session"))
                .OnClicked(FOnClicked::CreateRaw(this, &sfUISessionsPanel::StartLANSession))
            ]
        ]
        + SHorizontalBox::Slot().Padding(10, 0).VAlign(VAlign_Center).AutoWidth()
        [
           SNew(STextBlock)
            .Text(FText::FromString("Port"))
        ]
        + SHorizontalBox::Slot().AutoWidth()
        [
            SNew(SBox)
            .WidthOverride(50)
            [
                SAssignNew(m_portPtr, SEditableTextBox)
                .Text(FText::FromString("8000"))
            ]
        ]
    ];

    m_contentPtr->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(5, 5, 5, 10).AutoHeight()
    [
        SNew(SBox)
        .Visibility_Lambda([this]() -> EVisibility {
            return (!FPaths::DirectoryExists(FPaths::ProjectDir() + LAN_SERVER_FOLDER)) ? 
                EVisibility::Visible: 
                EVisibility::Collapsed;
        })
        [
            SNew(SHyperlink)
            .Text(FText::FromString("Contact us to enable LAN sessions."))
            .OnNavigate(FSimpleDelegate::CreateLambda([]() {
                FString error = "";
                FPlatformProcess::LaunchURL(
                    *FString("https://www.kinematicsoup.com/contact-us-ent"), nullptr, &error);
            }))
        ]
    ];

    // Running Sessions List
    m_contentPtr->AddSlot().HAlign(HAlign_Fill).VAlign(VAlign_Center).Padding(5, 5).AutoHeight()
    [
        SAssignNew(m_sessionListPtr, SListView<TSharedPtr<sfSessionInfo>>)
        .ItemHeight(24)
        .ListItemsSource(&m_sessions)
        .OnGenerateRow(TSlateDelegates<TSharedPtr<sfSessionInfo>>::FOnGenerateRow::CreateLambda(
            [this](TSharedPtr<sfSessionInfo> sessionInfoPtr,
            const TSharedRef<STableViewBase>& owner)->TSharedRef<ITableRow> {
                return SNew(sfUISessionRow, owner)
                .Item(sessionInfoPtr)
                .OnClicked(sfUISessionRow::OnJoinSession::CreateRaw(this, &sfUISessionsPanel::JoinSession));
            }
        ))
        .SelectionMode(ESelectionMode::None)
        .HeaderRow
        (
            SNew(SHeaderRow)
            + SHeaderRow::Column("Level").DefaultLabel(FText::FromString("Level"))
            + SHeaderRow::Column("Creator").DefaultLabel(FText::FromString("Creator"))
            + SHeaderRow::Column("Version").DefaultLabel(FText::FromString("Version"))
            + SHeaderRow::Column("Join").DefaultLabel(FText::FromString(""))
        )
    ];
}

void sfUISessionsPanel::ManualConnectArea()
{
     m_contentPtr->AddSlot()
    .HAlign(HAlign_Fill)
    .VAlign(VAlign_Center)
    .Padding(5, 2)
    .AutoHeight()
    [
        SNew(SExpandableArea)
        .AreaTitle(FText::FromString("Manual Connection"))
        .InitiallyCollapsed(true)
        .BodyContent()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(5, 5).HAlign(HAlign_Left).VAlign(VAlign_Center)
            [
                SNew(SBox)
                .WidthOverride(50)
                [
                    SNew(STextBlock).Text(FText::FromString("Address"))
                ]
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(5, 5).HAlign(HAlign_Left).VAlign(VAlign_Center)
            [
                SNew(SBox)
                .WidthOverride(100)
                [
                    SAssignNew(m_manualAddressPtr, SEditableTextBox).Text(FText::FromString(""))
                ]
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(5, 5).HAlign(HAlign_Left).VAlign(VAlign_Center)
            [
                SNew(SBox)
                .HAlign(HAlign_Center)
                .WidthOverride(30)
                [
                    SNew(STextBlock).Text(FText::FromString("Port"))
                ]
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(5, 5).HAlign(HAlign_Left).VAlign(VAlign_Center)
            [
                SNew(SBox)
                .WidthOverride(50)
                [
                    SAssignNew(m_manualPortPtr, SEditableTextBox).Text(FText::FromString("8000"))
                ]
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(5, 5).HAlign(HAlign_Left).VAlign(VAlign_Center)
            [
                SNew(SBox)
                .WidthOverride(75)
                [
                    SNew(SButton)
                    .HAlign(HAlign_Center)
                    .Text(FText::FromString("Connect"))
                    .OnClicked(FOnClicked::CreateRaw(this, &sfUISessionsPanel::ManualConnect))
                ]
            ]
        ]
    ];
}

void sfUISessionsPanel::Show()
{
    Enable();
    if (m_widgetPtr.IsValid()) {
        DisplayMessage("", sfUIMessageBox::INFO);
        m_widgetPtr->SetVisibility(EVisibility::Visible);

        TSharedPtr<FString> emailPtr = MakeShareable(new FString(sfConfig::Get().Email));
        m_accounts.Empty();
        m_accounts.Add(emailPtr);
        m_accounts.Add(MakeShareable(new FString("Logout")));
        m_accountComboPtr->SetSelectedItem(emailPtr);
        m_accountComboPtr->RefreshOptions();

        SceneFusion::WebService->GetSessions(
            sfBaseWebService::OnGetSessionsDelegate::CreateRaw(this, &sfUISessionsPanel::GetSessionsReply)
        );
    }
}

void sfUISessionsPanel::Hide()
{
    if (m_widgetPtr.IsValid()) {
        m_widgetPtr->SetVisibility(EVisibility::Hidden);
    }
}

void sfUISessionsPanel::JoinSession(TSharedPtr<sfSessionInfo> sessionInfoPtr)
{
    if (sessionInfoPtr.IsValid()) {
        DisplayMessage("Joining session...", sfUIMessageBox::INFO);
        Disable();
        OnJoinSession.ExecuteIfBound(sessionInfoPtr);
    }
}

void sfUISessionsPanel::SelectAccount(TSharedPtr<FString> valuePtr, ESelectInfo::Type selectType)
{
    m_selectedAccountPtr->SetText(FText::FromString(*valuePtr));
    if (valuePtr->Equals("Logout")) {
        DisplayMessage("Logging out...", sfUIMessageBox::INFO);
        Disable();
        OnLogout.Execute();
    }
}

void sfUISessionsPanel::SelectProject(TSharedPtr<sfProjectInfo> valuePtr, ESelectInfo::Type selectType)
{
    m_project = valuePtr->Name;
    sfConfig::Get().CompanyProject = m_project;
    m_selectedProjectPtr->SetText(FText::FromString(valuePtr->Name));
}

FReply sfUISessionsPanel::StartSession()
{
    DisplayMessage("Starting session...", sfUIMessageBox::INFO);
    Disable();
    SceneFusion::WebService->StartSession(
        m_project,
        FApp::GetProjectName(),
        sfBaseWebService::OnStartSessionDelegate::CreateRaw(this, &sfUISessionsPanel::StartSessionReply)
    );
    return FReply::Handled();
}

void sfUISessionsPanel::StartSessionReply(TSharedPtr<sfSessionInfo> sessionInfoPtr, const FString& error)
{
    Enable();
    if (!error.IsEmpty()) {
        DisplayMessage(error, sfUIMessageBox::ERROR);
        return;
    }

    if (sessionInfoPtr.IsValid())
    {
        OnStartSession.ExecuteIfBound(sessionInfoPtr);
    }
}

void sfUISessionsPanel::GetSessionsReply(ProjectMap& projectMap, const FString& error)
{
    if (!error.IsEmpty()) {
        DisplayMessage(error, sfUIMessageBox::WARNING);
        return;
    }

    // Replace the project map
    std::swap(m_projectMap, projectMap);
    m_projectMap.KeySort([](const FString& A, const FString& B) {
        return A.Compare(B, ESearchCase::IgnoreCase) > 0; 
    });

    // Clear the selected project
    m_project = sfConfig::Get().CompanyProject = m_project;
    if (!m_project.IsEmpty() && !m_projectMap.Contains(m_project)) {
        m_project = "";
    }

    // Rebuild the project list
    m_projects.Empty();
    if (m_projectMap.Num() > 0)  {
        for (auto& pair : m_projectMap) {
            m_projects.Add(pair.Value);
            if (m_project.IsEmpty()) {
                // Select the first project if one is not already selected
                m_project = pair.Key;
                sfConfig::Get().CompanyProject = m_project;
            }
        }
    }

    // Update the project selection combo
    m_projectComboPtr->RefreshOptions();
    m_projectComboPtr->SetSelectedItem(m_projectMap[m_project]);

    // Update the sessions list
    RedrawSessionsList();
}

void sfUISessionsPanel::RedrawSessionsList()
{
    m_sessions.Empty();
    if (!m_project.IsEmpty() && m_projectMap.Num() > 0  && m_projectMap.Contains(m_project)) {
        m_sessions.Append(m_projectMap[m_project]->Sessions);
    }

    TArray<uint32> removals;
    for (auto& lanSessionPair : m_lanSessions)
    {
        if (lanSessionPair.Value->Time < FDateTime::Now() - FTimespan::FromSeconds(5))
        {
            removals.Add(lanSessionPair.Key);
        }
        else 
        {
            m_sessions.Add(lanSessionPair.Value);
        }
    }
    for (auto& key : removals)
    {
        m_lanSessions.Remove(key);
    }

    m_sessionListPtr->RequestListRefresh();
}

void sfUISessionsPanel::InitializeUDP()
{
    // BUFFER SIZE
    m_socket = FUdpSocketBuilder(TEXT("SF2 Socket"))
        .AsNonBlocking()
        .AsReusable()
        .WithBroadcast()
        .BoundToAddress(FIPv4Address::Any)
        .BoundToEndpoint(FIPv4Endpoint::Any)
        .BoundToPort(8000)
        .WithReceiveBufferSize(1024);

    m_socketReceiverPtr = new FUdpSocketReceiver(m_socket, FTimespan::FromMilliseconds(1000), TEXT("SF2 Receiver"));
    m_socketReceiverPtr->OnDataReceived().BindRaw(this, &sfUISessionsPanel::OnBroadcastRecieved);
    m_socketReceiverPtr->Start();
}

void sfUISessionsPanel::CleanupUDP()
{
    if (m_socketReceiverPtr)
    {
        m_socketReceiverPtr->Stop();
        delete m_socketReceiverPtr;
        m_socketReceiverPtr = nullptr;
    }

    if (m_socket)
    {
        m_socket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(m_socket);
    }
}

void sfUISessionsPanel::OnBroadcastRecieved(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt)
{
    std::string temp;
    uint8* data = ArrayReaderPtr->GetData();

    TSharedPtr<sfSessionInfo> info = MakeShareable(new sfSessionInfo);
    info->RoomInfoPtr = ksRoomInfo::Create();
    info->RoomInfoPtr->Id() = *reinterpret_cast<uint32*>(ArrayReaderPtr->GetData());
    info->RoomInfoPtr->Port() = *reinterpret_cast<uint16*>(ArrayReaderPtr->GetData() + 4);
    info->RoomInfoPtr->Scene() = "Scene Fusion";
    info->RoomInfoPtr->Host() = std::string(TCHAR_TO_UTF8(*EndPt.Address.ToString()));
    info->RoomInfoPtr->Type() = "Scene Fusion";

    int offset = 6;
    uint8 len = ArrayReaderPtr->GetData()[offset++];
    temp.assign(reinterpret_cast<char const*>(data + offset), (size_t)len);
    info->ProjectName = FString(temp.c_str());
    offset += len;

    len = ArrayReaderPtr->GetData()[offset++];
    temp.assign(reinterpret_cast<char const*>(data + offset), (size_t)len);
    info->LevelName = FString(temp.c_str());
    offset += len;

    len = ArrayReaderPtr->GetData()[offset++];
    temp.assign(reinterpret_cast<char const*>(data + offset), (size_t)len);
    info->Creator = FString(temp.c_str());
    offset += len;

    len = ArrayReaderPtr->GetData()[offset++];
    temp.assign(reinterpret_cast<char const*>(data + offset), (size_t)len);
    info->RequiredVersion = FString(temp.c_str());
    offset += len;

    // Update session list
    if (m_lanSessions.Contains(info->RoomInfoPtr->Id())) {
        m_lanSessions[info->RoomInfoPtr->Id()] = info;
    }
    else {
        m_lanSessions.Add(info->RoomInfoPtr->Id(), info);
    }
}

FReply sfUISessionsPanel::StartLANSession()
{
    DisplayMessage("Starting LAN session...", sfUIMessageBox::INFO);
    Disable();

    // Construct paths
    FString serverPath = FPaths::ProjectDir() + LAN_SERVER_FOLDER;
    FString gameJsonPath = GAME_JSON;
    FString sceneJsonPath = SCENE_JSON;
    FString logFile = "sf_" + FDateTime::UtcNow().ToString() + ".log";
    if (!FPaths::FileExists(serverPath + LAN_SERVER_EXECUTEBLE))
    {
        KS::Log::Error("Failed to start LAN server. Could not find '"
            + std::string(TCHAR_TO_UTF8(*serverPath)) + LAN_SERVER_EXECUTEBLE + "'", LOG_CHANNEL);
        return FReply::Handled();
    }
    if (!FPaths::FileExists(serverPath + gameJsonPath) || !FPaths::FileExists(serverPath + sceneJsonPath))
    {
        KS::Log::Error("Failed to start LAN server. Could not find '" +
            std::string(TCHAR_TO_UTF8(*gameJsonPath)) +
            "' or '" + std::string(TCHAR_TO_UTF8(*sceneJsonPath)) + "'", LOG_CHANNEL);
        return FReply::Handled();
    }

    // Construct session info
    ksRoomInfo::SPtr roomInfoPtr = ksRoomInfo::Create();
    roomInfoPtr->Host() = "localhost";
    roomInfoPtr->Port() = (uint16_t)FCString::Atoi(*m_portPtr->GetText().ToString());
    roomInfoPtr->Scene() = "Scene Fusion";
    roomInfoPtr->Id() = (uint32_t)FDateTime::Now().GetTicks();

    TSharedPtr<sfSessionInfo> sessionInfoPtr = MakeShareable(new sfSessionInfo());
    sessionInfoPtr->RoomInfoPtr = roomInfoPtr;
    sessionInfoPtr->ProjectName = FApp::GetProjectName();
    sessionInfoPtr->LevelName = GEditor->GetEditorWorldContext().World()->GetMapName();
    sessionInfoPtr->Creator = FString(sfConfig::Get().Name);

    // Write LAN config
    TSharedPtr<FJsonObject> jsonPtr = MakeShareable(new FJsonObject);
    jsonPtr->SetStringField("company", m_project);
    jsonPtr->SetStringField("project", sessionInfoPtr->ProjectName);
    jsonPtr->SetStringField("scene", sessionInfoPtr->LevelName);
    jsonPtr->SetStringField("creator", sessionInfoPtr->Creator);
    jsonPtr->SetStringField("version", sessionInfoPtr->RequiredVersion);
    jsonPtr->SetNumberField("port", roomInfoPtr->Port());

    FString jsonString;
    TSharedRef<TJsonWriter<>> jsonWriter = TJsonWriterFactory<>::Create(&jsonString);
    FJsonSerializer::Serialize(jsonPtr.ToSharedRef(), jsonWriter);
    FFileHelper::SaveStringToFile(jsonString, *(serverPath + LAN_CONFIG));

    // Server a detached server process
    FString args = "";
    args = " -id="+ FString(std::to_string(roomInfoPtr->Id()).c_str());
    args += " -t=SceneFusion";
    args += " -tcp=" + FString::FromInt(roomInfoPtr->Port());
    args += " -l=\"" + logFile + "\"";
    args += " \"" + gameJsonPath + "\"";
    args += " \"" + sceneJsonPath + "\"";

    uint32_t processId = 0;
    m_processHandle = FPlatformProcess::CreateProc(
        *(serverPath + LAN_SERVER_EXECUTEBLE),  // executable name
        *args,  // command line arguments
        false,  // if false, the new process will be detached
        false,  // if true, the new process will not have a window
        false,  // if true, the process won't be there at all
        &processId,  // if non-NULL, this will be filled in with the ProcessId
        0,  // 2 idle, -1 low, 0 normal, 1 high, 2 higher
        *serverPath,  // Directory to start in when running the program, or NULL to use the current working directory
        nullptr); // Optional HANDLE to pipe for redirecting output

    if (m_processHandle.IsValid())
    {
        KS::Log::Info("Launching Scene Fusion server (PID: " + std::to_string(processId) + ")" , LOG_CHANNEL);
        KS::Log::Info(std::string(TCHAR_TO_ANSI(*args)), LOG_CHANNEL);
        FTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateLambda([this, sessionInfoPtr](float time)->bool {
                OnStartSession.ExecuteIfBound(sessionInfoPtr);
                return false; // returning true will reschedule this delegate.
            }),
            1.0f // delay period
        );
    }
    else
    {
        KS::Log::Error("Failed to start LAN server.", LOG_CHANNEL);
    }

    return FReply::Handled();
}

FReply sfUISessionsPanel::ManualConnect()
{
    ksRoomInfo::SPtr roomInfoPtr = ksRoomInfo::Create();
    roomInfoPtr->Host() = std::string(TCHAR_TO_UTF8(*m_manualAddressPtr->GetText().ToString()));
    roomInfoPtr->Port() = (uint16_t)FCString::Atoi(*m_manualPortPtr->GetText().ToString());
    roomInfoPtr->Scene() = "Scene Fusion";
    roomInfoPtr->Type() = "Scene Fusion";
    roomInfoPtr->Id() = 1;

    TSharedPtr<sfSessionInfo> sessionInfoPtr = MakeShareable(new sfSessionInfo());
    sessionInfoPtr->RoomInfoPtr = roomInfoPtr;
    sessionInfoPtr->ProjectName = FApp::GetProjectName();
    sessionInfoPtr->LevelName = GEditor->GetEditorWorldContext().World()->GetMapName();
    sessionInfoPtr->Creator = "Unknown";

    KS::Log::Info("Attempting a manual connection to a Scene Fusion server. (Address = " + 
        roomInfoPtr->Host() + ", Port = " + std::to_string(roomInfoPtr->Port()) + ")", LOG_CHANNEL);
    DisplayMessage("Joining session...", sfUIMessageBox::INFO);
    Disable();
    OnJoinSession.ExecuteIfBound(sessionInfoPtr);
    return FReply::Handled();
}

#undef LOG_CHANNEL
#undef LAN_SERVER_FOLDER
#undef LAN_SERVER_EXECUTEBLE
#undef GAME_JSON
#undef SCENE_JSON