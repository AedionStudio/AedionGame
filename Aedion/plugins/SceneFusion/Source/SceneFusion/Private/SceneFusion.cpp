#include "SceneFusion.h"
#include "Web/sfWebService.h"
#include "Web/sfMockWebService.h"
#include "Testing/sfTestUtil.h"
#include "Consts.h"
#include "sfConfig.h"
#include "sfObjectMap.h"
#include "sfPropertyUtil.h"
#include "sfLoader.h"
#include "sfUtils.h"

#include <Developer/HotReload/Public/IHotReload.h>

// Log setup
DEFINE_LOG_CATEGORY(LogSceneFusion)

#define LOG_CHANNEL "SceneFusion"

TSharedPtr<sfBaseWebService> SceneFusion::WebService = MakeShareable(new sfWebService());
sfService::SPtr SceneFusion::Service = nullptr;
IConsoleCommand* SceneFusion::m_mockWebServiceCommand = nullptr;
sfObjectEventDispatcher::SPtr SceneFusion::ObjectEventDispatcher = nullptr;
TSharedPtr<sfMissingObjectManager> SceneFusion::MissingObjectManager = nullptr;
TSharedPtr<sfUndoManager> SceneFusion::m_undoManagerPtr = nullptr;
TSharedPtr<sfActorManager> SceneFusion::ActorManager = nullptr;
TSharedPtr<sfAvatarManager> SceneFusion::AvatarManager = nullptr;
TSharedPtr<sfComponentManager> SceneFusion::ComponentManager = nullptr;
TSharedPtr<sfLevelManager> SceneFusion::LevelManager = nullptr;
TSharedPtr<sfUI> SceneFusion::m_sfUIPtr = nullptr;
ksEvent<sfUser::SPtr&>::SPtr SceneFusion::m_onUserColorChangeEventPtr = nullptr;
ksEvent<sfUser::SPtr&>::SPtr SceneFusion::m_onUserLeaveEventPtr = nullptr;
UMaterialInterface* SceneFusion::m_lockMaterialPtr = nullptr;
FDelegateHandle SceneFusion::m_onObjectsReplacedHandle;
FDelegateHandle SceneFusion::m_onHotReloadHandle;
TMap<uint32_t, UMaterialInstanceDynamic*> SceneFusion::m_lockMaterials;
TArray<UObject*> SceneFusion::m_replacedObjects;
bool SceneFusion::IsSessionCreator = false;
bool SceneFusion::m_redrawActiveViewport = false;

void SceneFusion::StartupModule()
{
    KS::Log::RegisterHandler("Root", HandleLog, KS::LogLevel::LOG_ALL, true);
    KS::Log::Info("Scene Fusion Client: 2.0.2", LOG_CHANNEL);
    sfConfig::Get().Load();
    InitializeWebService();

    m_lockMaterialPtr = LoadObject<UMaterialInterface>(nullptr, TEXT("/SceneFusion/LockMaterial"));

    Service = sfService::Create();
    ObjectEventDispatcher = sfObjectEventDispatcher::CreateSPtr();
    MissingObjectManager = MakeShareable(new sfMissingObjectManager);
    m_undoManagerPtr = MakeShareable(new sfUndoManager);
    LevelManager = MakeShareable(new sfLevelManager);
    ObjectEventDispatcher->Register(sfType::Level, LevelManager);
    ObjectEventDispatcher->Register(sfType::LevelLock, LevelManager);
    ObjectEventDispatcher->Register(sfType::LevelProperties, LevelManager);
    ObjectEventDispatcher->Register(sfType::GameMode, LevelManager);
    ActorManager = MakeShareable(new sfActorManager(LevelManager));
    ObjectEventDispatcher->Register(sfType::Actor, ActorManager);
    AvatarManager = MakeShareable(new sfAvatarManager);
    ObjectEventDispatcher->Register(sfType::Avatar, AvatarManager);
    ComponentManager = MakeShareable(new sfComponentManager);
    ObjectEventDispatcher->Register(sfType::Component, ComponentManager);
    TSharedPtr<sfMeshStandInManager> meshStandInManagerPtr = MakeShareable(new sfMeshStandInManager);
    ObjectEventDispatcher->Register(sfType::MeshBounds, meshStandInManagerPtr);
    sfLoader::Get().RegisterStandInGenerator(UStaticMesh::StaticClass(), meshStandInManagerPtr);

    if (FSlateApplication::IsInitialized())
    {
        m_sfUIPtr = MakeShareable(new sfUI);
        m_sfUIPtr->Initialize();
        m_sfUIPtr->OnGoToUser().BindRaw(AvatarManager.Get(), &sfAvatarManager::MoveViewportToUser);
        m_sfUIPtr->OnFollowUser().BindRaw(AvatarManager.Get(), &sfAvatarManager::Follow);
        AvatarManager->OnUnfollow.BindRaw(m_sfUIPtr.Get(), &sfUI::UnfollowCamera);
    }

    sfTestUtil::RegisterCommands();

    // Register an FTickerDelegate to be called 60 times per second.
    m_updateHandle = FTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateRaw(this, &SceneFusion::Tick), 1.0f / 60.0f);
}

void SceneFusion::ShutdownModule()
{
    KS::Log::Info("Scene Fusion shut down module.", LOG_CHANNEL);

    m_sfUIPtr->Cleanup();
    m_sfUIPtr.Reset();
    sfTestUtil::CleanUp();
    IConsoleManager::Get().UnregisterConsoleObject(m_mockWebServiceCommand);
    FTicker::GetCoreTicker().RemoveTicker(m_updateHandle);
}

void SceneFusion::OnConnect()
{
    ObjectEventDispatcher->Initialize();
    MissingObjectManager->Initialize();
    m_undoManagerPtr->Initialize();
    sfPropertyUtil::EnablePropertyChangeHandler();
    sfLoader::Get().Start();
    m_onUserColorChangeEventPtr = Service->Session()->RegisterOnUserColorChangeHandler(&OnUserColorChange);
    m_onUserLeaveEventPtr = Service->Session()->RegisterOnUserLeaveHandler(&OnUserLeave);
    m_onObjectsReplacedHandle = GEditor->OnObjectsReplaced().AddStatic(&OnObjectsReplaced);
    m_onHotReloadHandle = IHotReloadModule::Get().OnHotReload().AddStatic(&OnHotReload);
}

void SceneFusion::OnDisconnect()
{
    for (auto iter : m_lockMaterials)
    {
        iter.Value->ClearFlags(EObjectFlags::RF_Standalone);// Allow unreal to destroy the material instances
    }
    m_lockMaterials.Empty();
    m_onUserColorChangeEventPtr.reset();
    m_onUserLeaveEventPtr.reset();
    GEditor->OnObjectsReplaced().Remove(m_onObjectsReplacedHandle);
    IHotReloadModule::Get().OnHotReload().Remove(m_onHotReloadHandle);
    ObjectEventDispatcher->CleanUp();
    MissingObjectManager->CleanUp();
    m_undoManagerPtr->CleanUp();
    sfPropertyUtil::CleanUp();
    sfPropertyUtil::DisablePropertyChangeHandler();
    sfObjectMap::Clear();
    sfLoader::Get().Stop();
}

bool SceneFusion::Tick(float deltaTime)
{
    Service->Update(deltaTime);
    m_replacedObjects.Empty();
    if (Service->Session() != nullptr && Service->Session()->IsConnected())
    {
        GLevelEditorModeTools().ActivateMode("SceneFusion", false);
        sfPropertyUtil::RehashProperties();// rehash to make sure state is valid before broadcasting events
        sfPropertyUtil::BroadcastChangeEvents();
        sfPropertyUtil::SyncProperties();
        sfPropertyUtil::RehashProperties();// rehash in case properties were reverted on locked objects
        if (LevelManager.IsValid())
        {
            LevelManager->Tick();
        }
        if (ActorManager.IsValid())
        {
            ActorManager->Tick(deltaTime);
        }
        if (AvatarManager.IsValid())
        {
            AvatarManager->Tick();
        }
    }

    // Redraw the active viewport
    if (m_redrawActiveViewport)
    {
        m_redrawActiveViewport = false;
        FViewport* viewport = GEditor->GetActiveViewport();
        if (viewport != nullptr)
        {
            viewport->Draw();
        }
    }
    return true;
}

void SceneFusion::HandleLog(KS::LogLevel level, const char* channel, const char* message)
{
    std::string str = "[" + KS::Log::GetLevelString(level) + ";" + channel + "] " + message;
    FString fstr(UTF8_TO_TCHAR(str.c_str()));
    switch (level)
    {
        case KS::LogLevel::LOG_DEBUG:
        {
            UE_LOG(LogSceneFusion, Log, TEXT("%s"), *fstr);
            break;
        }
        case KS::LogLevel::LOG_INFO:
        {
            UE_LOG(LogSceneFusion, Log, TEXT("%s"), *fstr);
            break;
        }
        case KS::LogLevel::LOG_WARNING:
        {
            UE_LOG(LogSceneFusion, Warning, TEXT("%s"), *fstr);
            break;
        }
        case KS::LogLevel::LOG_ERROR:
        {
            UE_LOG(LogSceneFusion, Error, TEXT("%s"), *fstr);
            break;
        }
        case KS::LogLevel::LOG_FATAL:
        {
            UE_LOG(LogSceneFusion, Fatal, TEXT("%s"), *fstr);
            break;
        }
    }
}

void SceneFusion::InitializeWebService()
{
    // Load Mock Web Service configs
    sfConfig& config = sfConfig::Get();
    if (!config.MockWebServerAddress.IsEmpty() && !config.MockWebServerPort.IsEmpty()) {
        KS::Log::Info("Mock Web Service enabled: " +
            std::string(TCHAR_TO_UTF8(*config.MockWebServerAddress)) + " " +
            std::string(TCHAR_TO_UTF8(*config.MockWebServerPort)), LOG_CHANNEL);
        WebService = MakeShareable(new sfMockWebService(config.MockWebServerAddress, config.MockWebServerPort));
    }

    // Register Mock Web Service Command
    m_mockWebServiceCommand = IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("SFMockWebService"),
        TEXT("Usage: SFMockWebService [host port]. If a host or port are ommitted then the mock web service will be disabled."),
        FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& args) {
        sfConfig& config = sfConfig::Get();
        if (args.Num() == 2) {
            KS::Log::Info("Mock Web Service enabled: " +
                std::string(TCHAR_TO_UTF8(*args[0])) + " " +
                std::string(TCHAR_TO_UTF8(*args[1])), LOG_CHANNEL);
            WebService = MakeShareable(new sfMockWebService(args[0], args[1]));
            config.MockWebServerAddress = args[0];
            config.MockWebServerPort = args[1];
        }
        else {
            KS::Log::Info("Mock Web Service disabled", LOG_CHANNEL);
            WebService = MakeShareable(new sfWebService());
            config.MockWebServerAddress.Empty();
            config.MockWebServerPort.Empty();
        }
        config.Save();
    })
    );
}

void SceneFusion::RedrawActiveViewport()
{
    m_redrawActiveViewport = true;
}

UMaterialInterface* SceneFusion::GetLockMaterial(sfUser::SPtr userPtr)
{
    if (userPtr == nullptr || m_lockMaterialPtr == nullptr)
    {
        return m_lockMaterialPtr;
    }
    UMaterialInstanceDynamic** materialPtrPtr = m_lockMaterials.Find(userPtr->Id());
    if (materialPtrPtr != nullptr)
    {
        return Cast<UMaterialInterface>(*materialPtrPtr);
    }
    UMaterialInstanceDynamic* materialPtr = UMaterialInstanceDynamic::Create(m_lockMaterialPtr,
        GEditor->GetEditorWorldContext().World());
    // prevent material from being destroyed or saved
    materialPtr->SetFlags(EObjectFlags::RF_Standalone | EObjectFlags::RF_Transient);
    ksColor color = userPtr->Color();
    FLinearColor ucolor(color.R(), color.G(), color.B());
    materialPtr->SetVectorParameterValue("Color", ucolor);
    m_lockMaterials.Add(userPtr->Id(), materialPtr);
    return Cast<UMaterialInterface>(materialPtr);
}

void SceneFusion::OnObjectsReplaced(const TMap<UObject*, UObject*>& replacementMap)
{
    TSet<AActor*> actorPtrs;
    for (auto iter : replacementMap)
    {
        sfObject::SPtr objPtr = sfObjectMap::Remove(iter.Key);
        if (objPtr != nullptr)
        {
            sfObjectMap::Add(objPtr, iter.Value);
            m_replacedObjects.Add(iter.Value);

            // Record the affected actors
            UActorComponent* componentPtr = Cast<UActorComponent>(iter.Value);
            if (componentPtr != nullptr)
            {
                AActor* actorPtr = componentPtr->GetOwner();
                if (actorPtr != nullptr)
                {
                    actorPtrs.Add(actorPtr);
                }
            }
        }
    }
    
    // Relock the affected actors
    for (auto actorPtr : actorPtrs)
    {
        sfObject::SPtr objPtr = sfObjectMap::GetSFObject(actorPtr);
        if (objPtr != nullptr && objPtr->IsLocked())
        {
            ActorManager->Unlock(actorPtr);
            ActorManager->Lock(actorPtr, sfObjectMap::GetSFObject(actorPtr));
        }
    }
}

void SceneFusion::OnHotReload(bool automatic)
{
    for (UObject* uobjPtr : m_replacedObjects)
    {
        sfObject::SPtr objPtr = sfObjectMap::GetSFObject(uobjPtr);
        if (objPtr != nullptr)
        {
            // Apply server values for new properties
            sfPropertyUtil::ApplyProperties(uobjPtr, objPtr->Property()->AsDict());
            AActor* actorPtr = Cast<AActor>(uobjPtr);
            if (actorPtr != nullptr)
            {
                // Components may have been added or removed to so we sync components
                ComponentManager->SyncComponents(actorPtr, objPtr);
            }
        }
    }
    m_replacedObjects.Empty();
}

void SceneFusion::OnUserColorChange(sfUser::SPtr userPtr)
{
    UMaterialInstanceDynamic** materialPtrPtr = m_lockMaterials.Find(userPtr->Id());
    if (materialPtrPtr == nullptr)
    {
        return;
    }
    UMaterialInstanceDynamic* materialPtr = *materialPtrPtr;
    ksColor color = userPtr->Color();
    FLinearColor ucolor(color.R(), color.G(), color.B());
    materialPtr->SetVectorParameterValue("Color", ucolor);
}

void SceneFusion::OnUserLeave(sfUser::SPtr userPtr)
{
    UMaterialInstanceDynamic* materialPtr;
    if (m_lockMaterials.RemoveAndCopyValue(userPtr->Id(), materialPtr))
    {
        materialPtr->ClearFlags(EObjectFlags::RF_Standalone);// Allow unreal to destroy the material instance
    }
}

void SceneFusion::JoinSession(TSharedPtr<sfSessionInfo> sessionInfoPtr)
{
    m_sfUIPtr->JoinSession(sessionInfoPtr);
}

// Module loading
IMPLEMENT_MODULE(SceneFusion, SceneFusion)

#undef LOG_CHANNEL