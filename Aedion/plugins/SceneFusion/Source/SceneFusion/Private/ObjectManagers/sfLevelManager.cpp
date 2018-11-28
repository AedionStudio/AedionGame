#include "sfLevelManager.h"
#include "../Consts.h"
#include "../sfPropertyUtil.h"
#include "../SceneFusion.h"
#include "../sfUtils.h"
#include "../sfObjectMap.h"
#include "../Actors/sfMissingActor.h"

#include <Editor.h>
#include <Editor/UnrealEdEngine.h>
#include <Engine/Selection.h>
#include <UnrealEdGlobals.h>
#include <LevelUtils.h>
#include <FileHelpers.h>
#include <EditorLevelUtils.h>
#include <Settings/LevelEditorMiscSettings.h>
#include <EditorSupportDelegates.h>
#include <Engine/WorldComposition.h>
#include <EdMode.h>
#include <EditorModes.h>
#include <EditorModeManager.h>
#include <TabManager.h>
#include <Widgets/Docking/SDockTab.h>
#include <InputCoreTypes.h>
#include <Engine/LevelBounds.h>
#include <Widgets/Docking/SDockTab.h>
#include <KismetEditorUtilities.h>
#include <GameFramework/GameModeBase.h>
#include <Engine/Blueprint.h>

#define LOG_CHANNEL "sfLevelManager"

sfLevelManager::sfLevelManager() :
    m_initialized { false },
    m_uploadUnsyncedLevels{ false },
    m_worldPtr { nullptr },
    m_worldSettingsObjPtr { nullptr },
    m_worldTileDetailsClassPtr { nullptr },
    m_packageNamePropertyPtr { nullptr },
    m_worldSettingsDirty { false },
    m_hierarchicalLODSetupDirty { false }
{
    sfPropertyUtil::IgnoreDisableEditOnInstanceFlagForClass("LevelStreamingKisMet");
    sfPropertyUtil::IgnoreDisableEditOnInstanceFlagForClass("WorldSettings");

    RegisterPropertyChangeHandlers();
}

sfLevelManager::~sfLevelManager()
{

}

void sfLevelManager::Initialize()
{
    if (m_initialized)
    {
        return;
    }

    m_sessionPtr = SceneFusion::Service->Session();
    m_worldPtr = GEditor->GetEditorWorldContext().World();

    m_worldTileDetailsClassPtr = FindObject<UClass>(ANY_PACKAGE, UTF8_TO_TCHAR("WorldTileDetails"));
    if (m_worldTileDetailsClassPtr != nullptr)
    {
        m_packageNamePropertyPtr = m_worldTileDetailsClassPtr->FindPropertyByName(sfProp::PackageName->c_str());
    }

    m_onAcknowledgeSubscriptionHandle = m_sessionPtr->RegisterOnAcknowledgeSubscriptionHandler(
        [this](bool isSubscription, sfObject::SPtr objPtr) { OnAcknowledgeSubscription(isSubscription, objPtr); });

    RegisterLevelEvents();

    m_uploadUnsyncedLevels = !SceneFusion::IsSessionCreator;

    if (SceneFusion::IsSessionCreator)
    {
        // Upload levels
        RequestLock();
        m_levelsToUpload.emplace(m_worldPtr->PersistentLevel); // Upload persistent level first
        for (FConstLevelIterator iter = m_worldPtr->GetLevelIterator(); iter; ++iter)
        {
            if (!(*iter)->IsPersistentLevel())
            {
                m_levelsToUpload.emplace(*iter);
            }
        }
    }

    m_worldSettingsDirty = false;
    m_hierarchicalLODSetupDirty = false;

    m_initialized = true;
}

void sfLevelManager::CleanUp()
{
    if (!m_initialized)
    {
        return;
    }

    m_onAcknowledgeSubscriptionHandle.reset();
    UnregisterLevelEvents();

    m_lockObject = nullptr;
    m_levelsToUpload.clear();
    m_levelToObjectMap.Empty();
    m_objectToLevelMap.clear();
    m_objectToProperty.clear();
    m_propertyToObject.clear();
    m_movedLevels.clear();
    m_levelsNeedToBeLoaded.clear();
    m_unloadedLevelObjects.Empty();
    m_levelsWaitingForChildren.clear();
    m_dirtyStreamingLevels.Empty();
    m_dirtyParentLevels.Empty();
    m_uninitializedLevels.Empty();
    m_onLevelTransformChangeHandles.Empty();

    if (m_worldPtr != nullptr &&
        m_worldPtr->GetWorldSettings()->bEnableWorldComposition
        && m_worldPtr->WorldComposition != nullptr)
    {
        m_worldPtr->WorldComposition->bLockTilesLocation = false;
    }

    m_initialized = false;
}

void sfLevelManager::Tick()
{
    // After joining a session, uploads levels that don't exist on the server
    if (m_uploadUnsyncedLevels && m_levelToObjectMap.Num() > 0)
    {
        m_uploadUnsyncedLevels = false;
        UploadUnsyncedLevels();
    }

    // Upload levels when the level lock is acquired
    if (m_lockObject != nullptr && m_lockObject->LockOwner() == m_sessionPtr->LocalUser())
    {
        for (auto iter = m_levelsToUpload.begin(); iter != m_levelsToUpload.end(); iter++)
        {
            if (!m_levelToObjectMap.Contains(*iter))
            {
                UploadLevel(*iter);
            }
        }
        m_levelsToUpload.clear();
        m_lockObject->ReleaseLock();
    }

    // Send level transform change
    for (auto& levelPtr : m_movedLevels)
    {
        SendTransformUpdate(levelPtr);
    }
    m_movedLevels.clear();

    // Send level folder change
    for (ULevelStreaming* streamingLevelPtr : m_dirtyStreamingLevels)
    {
        SendFolderChange(streamingLevelPtr);
    }
    m_dirtyStreamingLevels.Empty();

    // Send level parent change
    for (ULevel* levelPtr : m_dirtyParentLevels)
    {
        if (!m_levelToObjectMap.Contains(levelPtr))
        {
            continue;
        }

        auto iter = m_objectToProperty.find(m_levelToObjectMap[levelPtr]);
        UObject* uobjPtr = FindWorldTileDetailsObject(levelPtr->GetOutermost()->GetName());
        if (uobjPtr != nullptr && iter != m_objectToProperty.end())
        {
            sfDictionaryProperty::SPtr levelPropertiesPtr = iter->second->Property()->AsDict();
            sfPropertyUtil::SendPropertyChanges(uobjPtr, levelPropertiesPtr);
        }
    }
    m_dirtyParentLevels.Empty();

    // Load levels that were removed but locked by other users
    for (sfObject::SPtr levelObjPtr : m_levelsNeedToBeLoaded)
    {
        OnCreateLevelObject(levelObjPtr);
    }
    m_levelsNeedToBeLoaded.clear();

    // Lock all tiles location if the selected levels contains locked level
    if (m_worldPtr->GetWorldSettings()->bEnableWorldComposition && m_worldPtr->WorldComposition != nullptr)
    {
        m_worldPtr->WorldComposition->bLockTilesLocation = false;
        for (ULevel* levelPtr : m_worldPtr->GetSelectedLevels())
        {
            sfObject::SPtr levelObjPtr = m_levelToObjectMap.FindRef(levelPtr);
            if (levelObjPtr != nullptr && levelObjPtr->IsLocked())
            {
                m_worldPtr->WorldComposition->bLockTilesLocation = true;
            }
        }
    }

    // Refresh world settings tab if world settings was changed
    if (m_worldSettingsDirty)
    {
        m_worldSettingsDirty = false;
        RefreshWorldSettingsTab();
    }

    // Apply HierarchicalLODSetupDirty because Unreal set it to a differnet value
    if (m_hierarchicalLODSetupDirty)
    {
        m_hierarchicalLODSetupDirty = false;
        sfPropertyUtil::SyncProperty(
            m_worldSettingsObjPtr,
            m_worldPtr->GetWorldSettings(),
            "HierarchicalLODSetup",
            true);
    }
}

sfObject::SPtr sfLevelManager::GetLevelObject(ULevel* levelPtr)
{
    return levelPtr == nullptr ? nullptr : m_levelToObjectMap.FindRef(levelPtr);
}

ULevel* sfLevelManager::FindLevelByObject(sfObject::SPtr levelObjPtr)
{
    if (levelObjPtr->Type() != sfType::Level)
    {
        return nullptr;
    }

    auto iter = m_objectToLevelMap.find(levelObjPtr);
    if (iter != m_objectToLevelMap.end())
    {
        return iter->second;
    }

    sfDictionaryProperty::SPtr propertiesPtr = levelObjPtr->Property()->AsDict();
    KS::Log::Warning("Could not find level " + propertiesPtr->Get(sfProp::Name)->ToString(), LOG_CHANNEL);
    return m_worldPtr->PersistentLevel;
}

void sfLevelManager::OnCreate(sfObject::SPtr objPtr, int childIndex)
{
    if (objPtr->Type() == sfType::Level)
    {
        OnCreateLevelObject(objPtr);
    }
    else if (objPtr->Type() == sfType::LevelLock)
    {
        OnCreateLevelLockObject(objPtr);
    }
    else if (objPtr->Type() == sfType::GameMode)
    {
        OnCreateGameModeObject(objPtr);
    }
}

void sfLevelManager::OnCreateLevelObject(sfObject::SPtr objPtr)
{
    sfDictionaryProperty::SPtr propertiesPtr = objPtr->Property()->AsDict();
    FString levelPath = *sfPropertyUtil::ToString(propertiesPtr->Get(sfProp::Name));
    bool isPersistentLevel = propertiesPtr->Get(sfProp::IsPersistentLevel)->AsValue()->GetValue();
    uint32_t propertyObjId = propertiesPtr->Get(sfProp::LevelPropertyId)->AsReference()->GetObjectId();
    sfObject::SPtr propertyObjPtr = m_sessionPtr->GetObject(propertyObjId);
    m_objectToProperty[objPtr] = propertyObjPtr;
    m_propertyToObject[propertyObjPtr] = objPtr;

    UnregisterLevelEvents();

    ULevel* levelPtr = FindLevelInLoadedLevels(levelPath, isPersistentLevel);
    if (levelPtr == nullptr && (isPersistentLevel || !GetWorldCompositionOnServer()))
    {
        levelPtr = LoadOrCreateMap(levelPath, isPersistentLevel);
        if (levelPtr == nullptr)
        {
            return;
        }
    }

    sfProperty::SPtr propPtr;
    if (!isPersistentLevel)
    {
        sfDictionaryProperty::SPtr levelPropertiesPtr = propertyObjPtr->Property()->AsDict();
        sfProperty::SPtr parentPropPtr = nullptr;
        levelPropertiesPtr->TryGet(sfProp::ParentPackageName, parentPropPtr);

        if (levelPtr != nullptr)
        {
            // If it is a new level object and the level is loaded, requests the server for all children.
            // If the level object needs to be loaded, that means the user tried to remove the level
            // while another user had some actors in that level locked. So we want to load the level back.
            // In this case, we have all the children already.
            if (m_objectToLevelMap.find(objPtr) == m_objectToLevelMap.end() &&
                m_levelsNeedToBeLoaded.find(objPtr) == m_levelsNeedToBeLoaded.end())
            {
                m_sessionPtr->SubscribeToChildren(objPtr);
                m_levelsWaitingForChildren.emplace(objPtr);
            }

            ULevelStreaming* streamingLevelPtr = FLevelUtils::FindStreamingLevel(levelPtr);
            if (streamingLevelPtr != nullptr)
            {
                sfProperty::SPtr propPtr = nullptr;
                if (m_worldPtr->GetWorldSettings()->bEnableWorldComposition)
                {
                    // Set tile details
                    UObject* worldTileDetailsObjPtr = FindWorldTileDetailsObject(levelPath);
                    if (worldTileDetailsObjPtr != nullptr)
                    {
                        sfPropertyUtil::ApplyProperties(worldTileDetailsObjPtr, levelPropertiesPtr);
                    }
                }

                // Set level transform
                if (propertiesPtr->TryGet(sfProp::Location, propPtr))
                {
                    FTransform transform = streamingLevelPtr->LevelTransform;
                    transform.SetLocation(sfPropertyUtil::ToVector(propPtr));
                    FRotator rotation = transform.Rotator();
                    rotation.Yaw = propertiesPtr->Get(sfProp::Rotation)->AsValue()->GetValue();
                    transform.SetRotation(rotation.Quaternion());
                    sfUtils::PreserveUndoStack([streamingLevelPtr, transform]()
                    {
                        FLevelUtils::SetEditorTransform(streamingLevelPtr, transform);
                    });
                    FDelegateHandle handle = levelPtr->OnApplyLevelTransform.AddLambda(
                        [this, levelPtr](const FTransform& transform) {
                        m_movedLevels.emplace(levelPtr);
                    });
                    m_onLevelTransformChangeHandles.Add(levelPtr, handle);
                }

                // Set folder path
                if (levelPropertiesPtr->TryGet(sfProp::Folder, propPtr))
                {
                    sfUtils::PreserveUndoStack([streamingLevelPtr, propPtr]()
                    {
                        streamingLevelPtr->SetFolderPath(*sfPropertyUtil::ToString(propPtr));
                    });
                }

                sfPropertyUtil::ApplyProperties(streamingLevelPtr, levelPropertiesPtr, &PROPERTY_BLACKLIST);
                sfObjectMap::Add(propertyObjPtr, streamingLevelPtr);
            }
        }
        else
        {
            m_unloadedLevelObjects.Add(levelPath, objPtr);
            RegisterLevelEvents();
            return;
        }
    }

    m_levelToObjectMap.Add(levelPtr, objPtr);
    m_objectToLevelMap[objPtr] = levelPtr;
    if (m_levelsWaitingForChildren.find(objPtr) == m_levelsWaitingForChildren.end())
    {
        SceneFusion::ActorManager->OnSFLevelObjectCreate(objPtr, levelPtr);
    }
    m_levelsToUpload.erase(levelPtr);

    RegisterLevelEvents();

    if (isPersistentLevel)
    {
        OnCreateWorldSettingsObject(propertyObjPtr);
    }

    // Refresh levels window and viewport
    FEditorDelegates::RefreshLevelBrowser.Broadcast();
    SceneFusion::RedrawActiveViewport();
}

void sfLevelManager::OnCreateLevelLockObject(sfObject::SPtr objPtr)
{
    m_lockObject = objPtr;
    if (m_levelsToUpload.size() > 0)
    {
        m_lockObject->RequestLock();
    }
}

void sfLevelManager::OnCreateWorldSettingsObject(sfObject::SPtr worldSettingsObjPtr)
{
    if (worldSettingsObjPtr == nullptr || worldSettingsObjPtr->Type() != sfType::LevelProperties)
    {
        KS::Log::Error("Could not find sfObject for world settings. Leaving session.", LOG_CHANNEL);
        SceneFusion::Service->LeaveSession();
        return;
    }

    m_worldSettingsObjPtr = worldSettingsObjPtr;

    AWorldSettings* worldSettingsPtr = m_worldPtr->GetWorldSettings();
    sfPropertyUtil::ApplyProperties(
        worldSettingsPtr,
        worldSettingsObjPtr->Property()->AsDict(),
        &WORLD_SETTINGS_BLACKLIST);
    m_worldSettingsDirty = true;
    m_hierarchicalLODSetupDirty = true;
    sfObjectMap::Add(worldSettingsObjPtr, worldSettingsPtr);

    TryToggleWorldComposition(GetWorldCompositionOnServer());
    
    OnCreateGameModeObject(m_worldSettingsObjPtr->Child(0));
}

void sfLevelManager::OnCreateGameModeObject(sfObject::SPtr objPtr)
{
    UClass* gameModePtr = m_worldPtr->GetWorldSettings()->DefaultGameMode;
    if (objPtr != nullptr && gameModePtr != nullptr && gameModePtr->IsInBlueprint())
    {
        AGameModeBase* defaultObjectPtr = gameModePtr->GetDefaultObject<AGameModeBase>();
        sfPropertyUtil::ApplyProperties(defaultObjectPtr, objPtr->Property()->AsDict());
        sfObjectMap::Add(objPtr, defaultObjectPtr);
    }
}

void sfLevelManager::OnDelete(sfObject::SPtr objPtr)
{
    sfObjectMap::Remove(objPtr);
    if (objPtr->Type() != sfType::Level)
    {
        return;
    }

    sfDictionaryProperty::SPtr propertiesPtr = objPtr->Property()->AsDict();
    FString levelPath = *sfPropertyUtil::ToString(propertiesPtr->Get(sfProp::Name));
    m_unloadedLevelObjects.Remove(levelPath);

    auto iter = m_objectToLevelMap.find(objPtr);
    if (iter == m_objectToLevelMap.end())
    {
        return;
    }
    ULevel* levelPtr = iter->second;
    m_objectToLevelMap.erase(iter);
    m_levelToObjectMap.Remove(levelPtr);

    sfObject::SPtr propertyObjPtr = m_objectToProperty[objPtr];
    sfObjectMap::Remove(propertyObjPtr);
    m_propertyToObject.erase(propertyObjPtr);
    m_objectToProperty.erase(objPtr);

    FDelegateHandle handle;
    if (m_onLevelTransformChangeHandles.RemoveAndCopyValue(levelPtr, handle))
    {
        levelPtr->OnApplyLevelTransform.Remove(handle);
    }

    // Temporarily remove PrepareToCleanseEditorObject event handler
    FEditorSupportDelegates::PrepareToCleanseEditorObject.Remove(m_onPrepareToCleanseEditorObjectHandle);

    SceneFusion::ActorManager->OnRemoveLevel(objPtr, levelPtr); // Remove actors in this level from actor manager

    // When a level is unloaded, any actors you had selected will be unselected.
    // We need to record those actors that are not in the level to be unloaded and reselect them after.
    TArray<AActor*> selectedActors;
    for (auto iter = GEditor->GetSelectedActorIterator(); iter; ++iter)
    {
        AActor* actorPtr = Cast<AActor>(*iter);
        if (actorPtr && actorPtr->GetLevel() != levelPtr)
        {
            selectedActors.Add(actorPtr);
        }
    }

    FEdMode* activeMode = GLevelEditorModeTools().GetActiveMode(FBuiltinEditorModes::EM_StreamingLevel);
    ULevelStreaming* streamingLevelPtr = FLevelUtils::FindStreamingLevel(levelPtr);
    if (activeMode != nullptr && streamingLevelPtr != nullptr)
    {
        // Toggle streaming level viewport transform editing off
        GLevelEditorModeTools().DeactivateMode(FBuiltinEditorModes::EM_StreamingLevel);
    }

    // Prompt to save level
    FEditorFileUtils::PromptForCheckoutAndSave(
        TArray<UPackage*>{levelPtr->GetOutermost()}, true, true, nullptr, false, false);
    UEditorLevelUtils::RemoveLevelFromWorld(levelPtr); // Remove level from world

    // Reselect actors
    for (AActor* actorPtr : selectedActors)
    {
        GEditor->SelectActor(actorPtr, true, true, true);
    }

    // Add PrepareToCleanseEditorObject event handler back
    m_onPrepareToCleanseEditorObjectHandle = FEditorSupportDelegates::PrepareToCleanseEditorObject.AddRaw(
        this,
        &sfLevelManager::OnPrepareToCleanseEditorObject);

    // Refresh levels window
    FEditorDelegates::RefreshLevelBrowser.Broadcast();

    // Notify the Scene Outliner
    GEngine->BroadcastLevelActorListChanged();
}

void sfLevelManager::OnListAdd(sfListProperty::SPtr listPtr, int index, int count)
{
    if (listPtr->GetPath() == *sfProp::HierarchicalLODSetup)
    {
        m_hierarchicalLODSetupDirty = true;
    }
    sfBaseUObjectManager::OnListAdd(listPtr, index, count);
}

ULevel* sfLevelManager::FindLevelInLoadedLevels(FString levelPath, bool isPersistentLevel)
{
    // Try to find level in loaded levels
    if (isPersistentLevel)
    {
        if (m_worldPtr->PersistentLevel->GetOutermost()->GetName() == levelPath)
        {
            return m_worldPtr->PersistentLevel;
        }
    }
    else
    {
        ULevelStreaming* streamingLevelPtr = FLevelUtils::FindStreamingLevel(m_worldPtr, *levelPath);
        if (streamingLevelPtr != nullptr)
        {
            return streamingLevelPtr->GetLoadedLevel();
        }
    }
    return nullptr;
}

ULevel* sfLevelManager::LoadOrCreateMap(const FString& levelPath, bool isPersistentLevel)
{
    ULevel* levelPtr = nullptr;
    if (!levelPath.StartsWith("/Temp/") && FPackageName::DoesPackageExist(levelPath))
    {
        levelPtr = TryLoadLevelFromFile(levelPath, isPersistentLevel);
    }

    if (levelPtr == nullptr)
    {
        if (!levelPath.StartsWith("/Temp/"))
        {
            KS::Log::Warning("Could not find level " + std::string(TCHAR_TO_UTF8(*levelPath)) +
                ". Please make sure that your project is up to date.", LOG_CHANNEL);
        }
        levelPtr = CreateMap(levelPath, isPersistentLevel);
    }

    if (levelPtr == nullptr)
    {
        KS::Log::Error("Failed to load or create level " + std::string(TCHAR_TO_UTF8(*levelPath)) +
            ". Disconnect.", LOG_CHANNEL);
        SceneFusion::Service->LeaveSession();
    }

    return levelPtr;
}

ULevel* sfLevelManager::TryLoadLevelFromFile(FString levelPath, bool isPersistentLevel)
{
    // Load map if the level is the persistent level
    if (isPersistentLevel)
    {
        // Loading a level triggers attach events we want to ignore.
        SceneFusion::ActorManager->DisableParentChangeHandler();
        // Prompts the user to save the dirty levels before load map
        if (FEditorFileUtils::SaveDirtyPackages(true, true, false) &&
            FEditorFileUtils::LoadMap(levelPath, false, true))
        {
            SceneFusion::ActorManager->EnableParentChangeHandler();
            // When a new map was loaded as the persistent level, all avatar actors were destroyed.
            // We need to recreate them.
            SceneFusion::AvatarManager->RecreateAllAvatars();
            SceneFusion::ActorManager->ClearActorCollections();
            m_worldPtr = GEditor->GetEditorWorldContext().World();
            return m_worldPtr->PersistentLevel;
        }
        SceneFusion::ActorManager->EnableParentChangeHandler();
    }
    else // Add level to world if it is a streaming level
    {
        ULevelStreaming* streamingLevelPtr = UEditorLevelUtils::AddLevelToWorld(m_worldPtr,
            *levelPath,
            GetDefault<ULevelEditorMiscSettings>()->DefaultLevelStreamingClass);
        if (streamingLevelPtr)
        {
            return streamingLevelPtr->GetLoadedLevel();
        }
    }
    return nullptr;
}

ULevel* sfLevelManager::CreateMap(FString levelPath, bool isPersistentLevel)
{
    if (isPersistentLevel)
    {
        // Prompts the user to save the dirty levels before load map
        if (FEditorFileUtils::SaveDirtyPackages(true, true, false))
        {
            m_worldPtr = GUnrealEd->NewMap();
            if (!levelPath.StartsWith("/Temp/"))
            {
                FEditorFileUtils::SaveLevel(m_worldPtr->PersistentLevel, levelPath);
            }
            // When the new map was created as the persistent level, all avatar actors were destroyed.
            // We need to recreate them.
            SceneFusion::AvatarManager->RecreateAllAvatars();
            SceneFusion::ActorManager->ClearActorCollections();
            return m_worldPtr->PersistentLevel;
        }
    }
    else
    {
        ULevelStreaming* streamingLevelPtr = UEditorLevelUtils::CreateNewStreamingLevel(
            GetDefault<ULevelEditorMiscSettings>()->DefaultLevelStreamingClass, levelPath, false);
        if (streamingLevelPtr)
        {
            return streamingLevelPtr->GetLoadedLevel();
        }
    }
    return nullptr;
}

void sfLevelManager::UploadLevel(ULevel* levelPtr)
{
    // Ignore buffer level. The buffer level is a temporary level used when moving actors to a different level.
    if (levelPtr == nullptr ||
        levelPtr->GetOutermost() == GetTransientPackage() ||
        m_levelToObjectMap.Contains(levelPtr))
    {
        return;
    }

    // Get level path
    FString levelPath = levelPtr->GetOutermost()->GetName();

    bool worldCompositionEnabled = m_worldPtr->GetWorldSettings()->bEnableWorldComposition;
    if (!levelPtr->IsPersistentLevel())
    {
        // Upload persistent level first
        if (SceneFusion::IsSessionCreator && m_worldSettingsObjPtr == nullptr)
        {
            UploadLevel(m_worldPtr->PersistentLevel);
        }

        // Upload parent level first
        if (worldCompositionEnabled)
        {
            FWorldTileInfo tileInfo = m_worldPtr->WorldComposition->GetTileInfo(FName(*levelPath));
            if (tileInfo.ParentTilePackageName != "None")
            {
                ULevelStreaming* streamingLevelPtr = FLevelUtils::FindStreamingLevel(
                    m_worldPtr, *tileInfo.ParentTilePackageName);
                if (streamingLevelPtr != nullptr && streamingLevelPtr->GetLoadedLevel() != nullptr)
                {
                    UploadLevel(streamingLevelPtr->GetLoadedLevel());
                }
            }
        }
    }

    // Create level object
    sfDictionaryProperty::SPtr propertiesPtr = sfDictionaryProperty::Create();
    sfObject::ObjectFlags flags = levelPtr->IsPersistentLevel() ? sfObject::NoFlags : sfObject::OptionalChildren;
    sfObject::SPtr levelObjPtr = sfObject::Create(sfType::Level, propertiesPtr, flags);

    propertiesPtr->Set(sfProp::Name, sfPropertyUtil::FromString(levelPath));
    propertiesPtr->Set(sfProp::IsPersistentLevel, sfValueProperty::Create(levelPtr->IsPersistentLevel()));

    sfDictionaryProperty::SPtr levelPropertiesPtr = sfDictionaryProperty::Create();
    sfObject::SPtr propertyObjPtr = sfObject::Create(sfType::LevelProperties, levelPropertiesPtr);

    if (levelPtr->IsPersistentLevel())
    {
        levelPropertiesPtr->Set(sfProp::WorldComposition, sfValueProperty::Create(worldCompositionEnabled));
        m_worldSettingsObjPtr = propertyObjPtr;

        AWorldSettings* worldSettingsPtr = m_worldPtr->GetWorldSettings();
        sfPropertyUtil::CreateProperties(worldSettingsPtr, levelPropertiesPtr, &WORLD_SETTINGS_BLACKLIST);
        sfObjectMap::Add(propertyObjPtr, worldSettingsPtr);

        // Create sfObject for the game mode
        UClass* gameModePtr = worldSettingsPtr->DefaultGameMode;
        if (gameModePtr != nullptr && gameModePtr->IsInBlueprint())
        {
            AGameModeBase* defaultObjectPtr = gameModePtr->GetDefaultObject<AGameModeBase>();
            sfDictionaryProperty::SPtr gameModePropertiesPtr = sfDictionaryProperty::Create();
            sfObject::SPtr gameModeObjPtr = sfObject::Create(sfType::GameMode, gameModePropertiesPtr);
            sfPropertyUtil::CreateProperties(defaultObjectPtr, gameModePropertiesPtr);
            sfObjectMap::Add(gameModeObjPtr, defaultObjectPtr);
            m_worldSettingsObjPtr->AddChild(gameModeObjPtr);
        }
    }
    else
    {
        // Sublevel properties
        if (worldCompositionEnabled)
        {
            // Other tile detail properties
            UObject* worldTileDetailsObjPtr = FindWorldTileDetailsObject(levelPath);
            if (worldTileDetailsObjPtr != nullptr)
            {
                sfPropertyUtil::CreateProperties(worldTileDetailsObjPtr, levelPropertiesPtr);
            }
        }

        // Sublevel transform properties
        ULevelStreaming* streamingLevelPtr = FLevelUtils::FindStreamingLevel(levelPtr);
        if (streamingLevelPtr != nullptr)
        {
            // Set transform properties
            FTransform transform = streamingLevelPtr->LevelTransform;
            propertiesPtr->Set(sfProp::Location, sfPropertyUtil::FromVector(transform.GetLocation()));
            propertiesPtr->Set(sfProp::Rotation, sfValueProperty::Create(transform.Rotator().Yaw));
            FDelegateHandle handle = levelPtr->OnApplyLevelTransform.AddLambda(
                [this, levelPtr](const FTransform& transform) {
                m_movedLevels.emplace(levelPtr);
            });

            m_onLevelTransformChangeHandles.Add(levelPtr, handle);// Add transform change handler on level

            // Set folder property
            levelPropertiesPtr->Set(sfProp::Folder,
                sfPropertyUtil::FromString(streamingLevelPtr->GetFolderPath().ToString()));

            sfPropertyUtil::CreateProperties(streamingLevelPtr, levelPropertiesPtr, &PROPERTY_BLACKLIST);
            sfObjectMap::Add(propertyObjPtr, streamingLevelPtr);
        }
    }

    // Create level property object first
    m_sessionPtr->Create(propertyObjPtr);

    // Set reference to property object
    propertiesPtr->Set(sfProp::LevelPropertyId, sfReferenceProperty::Create(propertyObjPtr->Id()));
    m_objectToProperty[levelObjPtr] = propertyObjPtr;
    m_propertyToObject[propertyObjPtr] = levelObjPtr;

    // Add level to maps
    m_levelToObjectMap.Add(levelPtr, levelObjPtr);
    m_objectToLevelMap[levelObjPtr] = levelPtr;

    for (AActor* actorPtr : levelPtr->Actors)
    {
        if (SceneFusion::ActorManager->IsSyncable(actorPtr) && actorPtr->GetAttachParentActor() == nullptr)
        {
            sfObject::SPtr objPtr = SceneFusion::ActorManager->CreateObject(actorPtr);
            if (objPtr != nullptr)
            {
                levelObjPtr->AddChild(objPtr);
            }
        }
    }

    // Create
    m_sessionPtr->Create(levelObjPtr);
}

void sfLevelManager::OnAddLevelToWorld(ULevel* newLevelPtr, UWorld* worldPtr)
{
    if (worldPtr != m_worldPtr || m_levelToObjectMap.Contains(newLevelPtr))
    {
        return;
    }

    FString levelPath = newLevelPtr->GetOutermost()->GetName();
    sfObject::SPtr levelObjPtr;
    if (m_unloadedLevelObjects.RemoveAndCopyValue(levelPath, levelObjPtr))
    {
        // Set tile details
        sfObject::SPtr propertyObjPtr = m_objectToProperty[levelObjPtr];
        sfDictionaryProperty::SPtr levelPropertiesPtr = propertyObjPtr->Property()->AsDict();

        // Load parent level if it is not already loaded
        sfProperty::SPtr parentPropPtr;
        if (levelPropertiesPtr->TryGet(sfProp::ParentPackageName, parentPropPtr))
        {
            FString parentPath = sfPropertyUtil::ToString(parentPropPtr);
            if (FindLevelInLoadedLevels(parentPath, false) == nullptr)
            {
                LoadOrCreateMap(parentPath, false);
            }
        }

        // Set tile details in next tick because if we set them here then they will be reverted
        m_uninitializedLevels.Add(newLevelPtr);
        FTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateLambda([this, newLevelPtr, levelPath, levelPropertiesPtr](float deltaTime)
        {
            UObject* worldTileDetailsObjPtr = FindWorldTileDetailsObject(levelPath);
            if (worldTileDetailsObjPtr != nullptr)
            {
                sfPropertyUtil::ApplyProperties(worldTileDetailsObjPtr, levelPropertiesPtr);
            }
            m_uninitializedLevels.Remove(newLevelPtr);
            return false;
        }));

        m_sessionPtr->SubscribeToChildren(levelObjPtr);
        m_levelToObjectMap.Add(newLevelPtr, levelObjPtr);
        m_objectToLevelMap[levelObjPtr] = newLevelPtr;
        m_levelsWaitingForChildren.emplace(levelObjPtr);
        ULevelStreaming* streamingLevelPtr = FLevelUtils::FindStreamingLevel(newLevelPtr);
        if (streamingLevelPtr)
        {
            sfObjectMap::Add(propertyObjPtr, streamingLevelPtr);
        }
        else
        {
            KS::Log::Error("Cannot find ULevelStreaming object for level " + sfUtils::FToStdString(levelPath),
                LOG_CHANNEL);
        }
    }
    else
    {
        RequestLock();
        m_levelsToUpload.emplace(newLevelPtr);
    }
}

void sfLevelManager::OnPrepareToCleanseEditorObject(UObject* uobjPtr)
{
    // Disconnect if the world is going to be destroyed
    UWorld* worldPtr = Cast<UWorld>(uobjPtr);

    if (worldPtr == m_worldPtr)
    {
        KS::Log::Info("World destroyed. Disconnect from server.", LOG_CHANNEL);
        m_worldPtr = nullptr;
        SceneFusion::Service->LeaveSession();
        return;
    }

    // If the object is a level, unregister the local player from its whitelist on the server side
    ULevel* levelPtr = Cast<ULevel>(uobjPtr);
    if (levelPtr == nullptr)
    {
        return;
    }

    m_levelsToUpload.erase(levelPtr);
    m_dirtyParentLevels.Remove(levelPtr);
    FDelegateHandle handle;
    if (m_onLevelTransformChangeHandles.RemoveAndCopyValue(levelPtr, handle))
    {
        levelPtr->OnApplyLevelTransform.Remove(handle);
    }

    sfObject::SPtr levelObjPtr = m_levelToObjectMap.FindRef(levelPtr);
    SceneFusion::ActorManager->OnRemoveLevel(levelObjPtr, levelPtr); // Clear raw pointers to actors in this level
    if (levelObjPtr)
    {
        ULevelStreaming* streamingLevelPtr = FLevelUtils::FindStreamingLevel(levelPtr);
        sfObjectMap::Remove(streamingLevelPtr);

        m_levelToObjectMap.Remove(levelPtr);
        m_objectToLevelMap.erase(levelObjPtr);
        m_levelsWaitingForChildren.erase(levelObjPtr);

        if (m_worldPtr->GetWorldSettings()->bEnableWorldComposition)
        {
            FString levelPath = levelPtr->GetOutermost()->GetName();
            m_unloadedLevelObjects.Add(levelPath, levelObjPtr);
            m_sessionPtr->UnsubscribeFromChildren(levelObjPtr);
        }
        else if (levelObjPtr->IsLocked())
        {
            m_levelsNeedToBeLoaded.emplace(levelObjPtr);
        }
        else
        {
            sfObject::SPtr propertyObjPtr = m_objectToProperty[levelObjPtr];
            m_objectToProperty.erase(levelObjPtr);
            m_propertyToObject.erase(propertyObjPtr);

            m_sessionPtr->Delete(propertyObjPtr);
            m_sessionPtr->Delete(levelObjPtr);
        }
    }
}

void sfLevelManager::UploadUnsyncedLevels()
{
    for (FConstLevelIterator iter = m_worldPtr->GetLevelIterator(); iter; ++iter)
    {
        if (!m_levelToObjectMap.Contains(*iter))
        {
            RequestLock();
            m_levelsToUpload.emplace(*iter);
        }
    }

    // Refresh levels window
    FEditorDelegates::RefreshLevelBrowser.Broadcast();
}

void sfLevelManager::SendTransformUpdate(ULevel* levelPtr)
{
    sfObject::SPtr objPtr = m_levelToObjectMap.FindRef(levelPtr);
    if (objPtr == nullptr)
    {
        return;
    }

    ULevelStreaming* streamingLevelPtr = FLevelUtils::FindStreamingLevel(levelPtr);
    if (streamingLevelPtr == nullptr)
    {
        return;
    }

    FTransform transform = streamingLevelPtr->LevelTransform;
    sfDictionaryProperty::SPtr propertiesPtr = objPtr->Property()->AsDict();
    sfProperty::SPtr oldPropPtr;

    if (objPtr->IsLocked())
    {
        // Revert level offset transform
        if (!propertiesPtr->TryGet(sfProp::Location, oldPropPtr) ||
            transform.GetLocation() != sfPropertyUtil::ToVector(oldPropPtr))
        {
            transform.SetLocation(sfPropertyUtil::ToVector(oldPropPtr));
            ModifyLevelWithoutTriggerEvent(levelPtr, [streamingLevelPtr, transform]()
            {
                FLevelUtils::SetEditorTransform(streamingLevelPtr, transform);
            });
        }
        if (!propertiesPtr->TryGet(sfProp::Rotation, oldPropPtr) ||
            transform.Rotator().Yaw != oldPropPtr->AsValue()->GetValue().GetFloat())
        {
            FRotator rotation = transform.Rotator();
            rotation.Yaw = oldPropPtr->AsValue()->GetValue();
            transform.SetRotation(rotation.Quaternion());
            ModifyLevelWithoutTriggerEvent(levelPtr, [streamingLevelPtr, transform]()
            {
                FLevelUtils::SetEditorTransform(streamingLevelPtr, transform);
            });
        }

        // Unreal may set the transform again after we reverted it. In that case, we need to revert it again.
        if (!streamingLevelPtr->LevelTransform.Equals(transform))
        {
            ModifyLevelWithoutTriggerEvent(levelPtr, [streamingLevelPtr, transform]()
            {
                FLevelUtils::SetEditorTransform(streamingLevelPtr, transform);
            });
        }
    }
    else
    {
        if (!propertiesPtr->TryGet(sfProp::Location, oldPropPtr) ||
            transform.GetLocation() != sfPropertyUtil::ToVector(oldPropPtr))
        {
            propertiesPtr->Set(sfProp::Location, sfPropertyUtil::FromVector(transform.GetLocation()));
        }

        if (!propertiesPtr->TryGet(sfProp::Rotation, oldPropPtr) ||
            transform.Rotator().Yaw != oldPropPtr->AsValue()->GetValue().GetFloat())
        {
            propertiesPtr->Set(sfProp::Rotation, sfValueProperty::Create(transform.Rotator().Yaw));
        }
    }
}

void sfLevelManager::RegisterPropertyChangeHandlers()
{
    m_propertyChangeHandlers[sfProp::Location] = [this](UObject* uobjPtr, sfProperty::SPtr propertyPtr)
    {
        ULevelStreaming* streamingLevelPtr = Cast<ULevelStreaming>(uobjPtr);
        if (streamingLevelPtr == nullptr)
        {
            return true;
        }
        FTransform transform = streamingLevelPtr->LevelTransform;
        transform.SetLocation(sfPropertyUtil::ToVector(propertyPtr));
        ModifyLevelWithoutTriggerEvent(streamingLevelPtr->GetLoadedLevel(), [streamingLevelPtr, transform]()
        {
            FLevelUtils::SetEditorTransform(streamingLevelPtr, transform);
        });
        SceneFusion::RedrawActiveViewport();
        return true;
    };

    m_propertyChangeHandlers[sfProp::Rotation] = [this](UObject* uobjPtr, sfProperty::SPtr propertyPtr)
    {
        ULevelStreaming* streamingLevelPtr = Cast<ULevelStreaming>(uobjPtr);
        if (streamingLevelPtr == nullptr)
        {
            return true;
        }
        FTransform transform = streamingLevelPtr->LevelTransform;
        FRotator rotation = transform.Rotator();
        rotation.Yaw = propertyPtr->AsValue()->GetValue();
        transform.SetRotation(rotation.Quaternion());
        ModifyLevelWithoutTriggerEvent(streamingLevelPtr->GetLoadedLevel(), [streamingLevelPtr, transform]()
        {
            FLevelUtils::SetEditorTransform(streamingLevelPtr, transform);
        });
        SceneFusion::RedrawActiveViewport();
        return true;
    };

    m_propertyChangeHandlers[sfProp::Folder] = [this](UObject* uobjPtr, sfProperty::SPtr propertyPtr)
    {
        ULevelStreaming* streamingLevelPtr = Cast<ULevelStreaming>(uobjPtr);
        if (streamingLevelPtr == nullptr)
        {
            return true;
        }
        ModifyLevelWithoutTriggerEvent(streamingLevelPtr->GetLoadedLevel(), [streamingLevelPtr, propertyPtr]()
        {
            streamingLevelPtr->SetFolderPath(*sfPropertyUtil::ToString(propertyPtr));
        });
        FEditorDelegates::RefreshLevelBrowser.Broadcast();
        return true;
    };

    m_propertyChangeHandlers[sfProp::WorldComposition] = [this](UObject* uobjPtr, sfProperty::SPtr propertyPtr)
    {
        TryToggleWorldComposition(propertyPtr->AsValue()->GetValue());
        return true;
    };

    m_propertyChangeHandlers[sfProp::DefaultGameMode] = [this](UObject* uobjPtr, sfProperty::SPtr propertyPtr)
    {
        if (m_worldSettingsObjPtr != nullptr)
        {
            AWorldSettings* worldSettingsPtr = m_worldPtr->GetWorldSettings();
            sfUPropertyInstance upropInstance = sfPropertyUtil::FindUProperty(worldSettingsPtr, propertyPtr);
            if (upropInstance.IsValid())
            {
                sfPropertyUtil::SetValue(worldSettingsPtr, upropInstance, propertyPtr);
            }
            else
            {
                UProperty* upropPtr = uobjPtr->GetClass()->FindPropertyByName(sfProp::DefaultGameMode->c_str());
                if (upropPtr != nullptr)
                {
                    sfPropertyUtil::SetToDefaultValue(uobjPtr, upropPtr);
                }
            }

            sfObject::SPtr gameModeObjPtr = m_worldSettingsObjPtr->Child(0);
            if (gameModeObjPtr != nullptr)
            {
                UClass* gameModePtr = m_worldPtr->GetWorldSettings()->DefaultGameMode;
                sfObjectMap::Remove(gameModeObjPtr);
                if (gameModePtr != nullptr)
                {
                    sfObjectMap::Add(gameModeObjPtr, gameModePtr->GetDefaultObject());
                }
            }
        }
        return true;
    };

    m_propertyChangeHandlers[sfProp::HierarchicalLODSetup] = [this](UObject* uobjPtr, sfProperty::SPtr propertyPtr)
    {
        m_hierarchicalLODSetupDirty = true;
        return false;
    };
}

void sfLevelManager::OnObjectModified(UObject* uobjPtr)
{
    ULevelStreaming* streamingLevelPtr = Cast<ULevelStreaming>(uobjPtr);
    if (streamingLevelPtr != nullptr)
    {
        m_dirtyStreamingLevels.Add(streamingLevelPtr);
    }
}

void sfLevelManager::SendFolderChange(ULevelStreaming* streamingLevelPtr)
{
    if (streamingLevelPtr == nullptr || streamingLevelPtr->GetLoadedLevel() == nullptr)
    {
        return;
    }
    ULevel* levelPtr = streamingLevelPtr->GetLoadedLevel();

    sfObject::SPtr objPtr = m_levelToObjectMap.FindRef(levelPtr);
    if (objPtr == nullptr)
    {
        return;
    }

    sfDictionaryProperty::SPtr propertiesPtr = m_objectToProperty[objPtr]->Property()->AsDict();
    sfProperty::SPtr oldPropPtr;
    FString folder = streamingLevelPtr->GetFolderPath().ToString();

    if (!propertiesPtr->TryGet(sfProp::Folder, oldPropPtr) ||
        folder != sfPropertyUtil::ToString(oldPropPtr))
    {
        propertiesPtr->Set(sfProp::Folder, sfPropertyUtil::FromString(folder));
    }
}

bool sfLevelManager::OnUndoRedo(sfObject::SPtr objPtr, UObject* uobjPtr)
{
    if (uobjPtr->GetClass()->GetName() == "WorldTileDetails")
    {
        for (TFieldIterator<UProperty> iter(uobjPtr->GetClass()); iter; ++iter)
        {
            OnTileDetailsChange(uobjPtr, *iter);
        }
        return true;
    }
    if (objPtr != nullptr)
    {
        ULevelStreaming* streamingLevelPtr = Cast<ULevelStreaming>(uobjPtr);
        if (streamingLevelPtr != nullptr)
        {
            SendFolderChange(streamingLevelPtr);
            sfPropertyUtil::SendPropertyChanges(streamingLevelPtr, objPtr->Property()->AsDict(), &PROPERTY_BLACKLIST);
            return true;
        }
        AWorldSettings* worldSettingsPtr = Cast<AWorldSettings>(uobjPtr);
        if (worldSettingsPtr != nullptr)
        {
            sfPropertyUtil::SendPropertyChanges(worldSettingsPtr, objPtr->Property()->AsDict(), &WORLD_SETTINGS_BLACKLIST);
            return true;
        }
    }
    return false;
}

void sfLevelManager::ModifyLevelWithoutTriggerEvent(ULevel* levelPtr, Callback callback)
{
    // Temporarily remove event handlers
    FDelegateHandle handle;
    if (m_onLevelTransformChangeHandles.RemoveAndCopyValue(levelPtr, handle))
    {
        levelPtr->OnApplyLevelTransform.Remove(handle);
    }
    FCoreUObjectDelegates::OnObjectModified.Remove(m_onObjectModifiedHandle);

    // Invoke callback function and prevents any changes to the undo stack during the call.
    sfUtils::PreserveUndoStack(callback);

    // Add event handlers back
    handle = levelPtr->OnApplyLevelTransform.AddLambda(
        [this, levelPtr](const FTransform& transform) {
        m_movedLevels.emplace(levelPtr);
    });
    m_onLevelTransformChangeHandles.Add(levelPtr, handle);
    m_onObjectModifiedHandle
        = FCoreUObjectDelegates::OnObjectModified.AddRaw(this, &sfLevelManager::OnObjectModified);
}

void sfLevelManager::RequestLock()
{
    if (m_lockObject == nullptr && SceneFusion::IsSessionCreator)
    {
        m_lockObject = sfObject::Create(sfType::LevelLock);
        m_sessionPtr->Create(m_lockObject);
    }

    if (m_lockObject != nullptr &&
        (m_sessionPtr->LocalUser() == nullptr ||
        m_lockObject->LockOwner() != m_sessionPtr->LocalUser()))
    {
        m_lockObject->RequestLock();
    }
}

void sfLevelManager::RegisterLevelEvents()
{
    m_onAddLevelToWorldHandle = FWorldDelegates::LevelAddedToWorld.AddRaw(this, &sfLevelManager::OnAddLevelToWorld);
    m_onPrepareToCleanseEditorObjectHandle = FEditorSupportDelegates::PrepareToCleanseEditorObject.AddRaw(
        this,
        &sfLevelManager::OnPrepareToCleanseEditorObject);
    m_onObjectModifiedHandle = FCoreUObjectDelegates::OnObjectModified.AddRaw(this, &sfLevelManager::OnObjectModified);
    m_onWorldCompositionChangeHandle
        = UWorldComposition::WorldCompositionChangedEvent.AddRaw(this, &sfLevelManager::SetWorldCompositionOnServer);
    m_onPackageMarkedDirtyHandle = UPackage::PackageMarkedDirtyEvent.AddRaw(
        this, &sfLevelManager::OnPackageMarkedDirty);
    sfPropertyUtil::RegisterPropertyChangeHandlerForClass("WorldTileDetails",
        [this](UObject* uobjPtr, UProperty* upropPtr) { OnTileDetailsChange(uobjPtr, upropPtr); });
    m_onPropertyChangeHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(
        this, &sfLevelManager::OnUPropertyChange);
}

void sfLevelManager::UnregisterLevelEvents()
{
    FWorldDelegates::LevelAddedToWorld.Remove(m_onAddLevelToWorldHandle);
    FEditorSupportDelegates::PrepareToCleanseEditorObject.Remove(m_onPrepareToCleanseEditorObjectHandle);
    FCoreUObjectDelegates::OnObjectModified.Remove(m_onObjectModifiedHandle);
    UWorldComposition::WorldCompositionChangedEvent.Remove(m_onWorldCompositionChangeHandle);
    UPackage::PackageMarkedDirtyEvent.Remove(m_onPackageMarkedDirtyHandle);
    sfPropertyUtil::UnregisterPropertyChangeHandlerForClass("WorldTileDetails");
    FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(m_onPropertyChangeHandle);
}

void sfLevelManager::OnAcknowledgeSubscription(bool isSubscription, sfObject::SPtr objPtr)
{
    if (!isSubscription || objPtr->Type() != sfType::Level)
    {
        return;
    }

    m_levelsWaitingForChildren.erase(objPtr);

    sfDictionaryProperty::SPtr propertiesPtr = objPtr->Property()->AsDict();
    FString levelPath = *sfPropertyUtil::ToString(propertiesPtr->Get(sfProp::Name));
    bool isPersistentLevel = propertiesPtr->Get(sfProp::IsPersistentLevel)->AsValue()->GetValue();
    ULevel* levelPtr = FindLevelInLoadedLevels(levelPath, isPersistentLevel);
    if (levelPtr != nullptr)
    {
        SceneFusion::ActorManager->DestroyUnsyncedActorsInLevel(levelPtr);
    }
}

void sfLevelManager::TryToggleWorldComposition(bool enableWorldComposition)
{
    if (enableWorldComposition == m_worldPtr->GetWorldSettings()->bEnableWorldComposition)
    {
        return;
    }

    UWorldComposition::WorldCompositionChangedEvent.Remove(m_onWorldCompositionChangeHandle);
    ToggleWorldComposition(enableWorldComposition);
    m_onWorldCompositionChangeHandle
        = UWorldComposition::WorldCompositionChangedEvent.AddRaw(this, &sfLevelManager::SetWorldCompositionOnServer);

    if (enableWorldComposition == m_worldPtr->GetWorldSettings()->bEnableWorldComposition)
    {
        if (!enableWorldComposition)
        {
            // Load all sublevels
            for (auto pair : m_unloadedLevelObjects)
            {
                OnCreateLevelObject(pair.Value);
            }
            m_unloadedLevelObjects.Empty();
        }

        // Refresh levels window and viewport
        FEditorDelegates::RefreshLevelBrowser.Broadcast();
        SceneFusion::RedrawActiveViewport();
    }
    else
    {
        KS::Log::Error("Failed to " +
            std::string(enableWorldComposition ? "enable" : "disable") +
            " world composition. Leaving session.", LOG_CHANNEL);
        SceneFusion::Service->LeaveSession();
    }
}

void sfLevelManager::ToggleWorldComposition(bool enableWorldComposition)
{
    if (!UWorldComposition::EnableWorldCompositionEvent.IsBound())
    {
        return;
    }

    TArray<FString> temporarilyUnloadedLevel;
    if (enableWorldComposition)
    {
        // Save dirty packages before unloading streaming levels
        TArray<UPackage*> packagesToSave;
        FEditorFileUtils::GetDirtyWorldPackages(packagesToSave);
        packagesToSave.Remove(m_worldPtr->PersistentLevel->GetOutermost());
        FEditorFileUtils::EPromptReturnCode result = FEditorFileUtils::PromptForCheckoutAndSave(
            packagesToSave, false, true, nullptr, false, true);
        if (result == FEditorFileUtils::EPromptReturnCode::PR_Cancelled ||
            result == FEditorFileUtils::EPromptReturnCode::PR_Failure)
        {
            return;
        }

        // Unload streaming levels
        TSet<ULevel*> levels(m_worldPtr->GetLevels());
        for (ULevel* levelPtr : levels)
        {
            if (!levelPtr->IsPersistentLevel())
            {
                sfObject::SPtr levelObjPtr = m_levelToObjectMap.FindRef(levelPtr);
                if (levelObjPtr != nullptr)
                {
                    m_levelToObjectMap.Remove(levelPtr);
                    m_objectToLevelMap.erase(levelObjPtr);
                }
                temporarilyUnloadedLevel.Add(levelPtr->GetOutermost()->GetName());
                UEditorLevelUtils::RemoveLevelFromWorld(levelPtr);
            }
        }
    }

    // Set bEnableWorldComposition so when the world composition event is broadcast,
    // the event handlers can get the new value.
    m_worldPtr->GetWorldSettings()->bEnableWorldComposition = enableWorldComposition;
    bool result = UWorldComposition::EnableWorldCompositionEvent.Execute(m_worldPtr, enableWorldComposition);
    m_worldPtr->GetWorldSettings()->bEnableWorldComposition = result; // In case we failed to enable it

    // Load levels back
    for (FString levelPath : temporarilyUnloadedLevel)
    {
        TryLoadLevelFromFile(levelPath, false);
    }
}

void sfLevelManager::SetWorldCompositionOnServer(UWorld* worldPtr)
{
    if (m_worldSettingsObjPtr == nullptr)
    {
        return;
    }

    bool worldCompositionEnabled = worldPtr->GetWorldSettings()->bEnableWorldComposition;
    sfDictionaryProperty::SPtr worldSettingsPropertiesPtr = m_worldSettingsObjPtr->Property()->AsDict();
    sfProperty::SPtr oldPropPtr;
    if (!worldSettingsPropertiesPtr->TryGet(sfProp::WorldComposition, oldPropPtr) ||
        worldCompositionEnabled != oldPropPtr->AsValue()->GetValue().GetBool())
    {
        worldSettingsPropertiesPtr->Set(sfProp::WorldComposition, sfValueProperty::Create(worldCompositionEnabled));

        if (!worldCompositionEnabled)
        {
            // Load all sublevels
            for (auto iter = m_unloadedLevelObjects.CreateConstIterator(); iter; ++iter)
            {
                OnCreateLevelObject(iter.Value());
            }
            m_unloadedLevelObjects.Empty();
        }
    }
}

bool sfLevelManager::GetWorldCompositionOnServer()
{
    sfDictionaryProperty::SPtr worldSettingsPropertiesPtr = m_worldSettingsObjPtr->Property()->AsDict();
    return worldSettingsPropertiesPtr->Get(sfProp::WorldComposition)->AsValue()->GetValue();
}

bool sfLevelManager::IsLevelObjectInitialized(ULevel* levelPtr)
{
    sfObject::SPtr levelObjPtr = GetLevelObject(levelPtr);
    if (levelObjPtr != nullptr &&
        m_levelsWaitingForChildren.find(levelObjPtr) == m_levelsWaitingForChildren.end())
    {
        return true;
    }
    return false;
}

void sfLevelManager::OnPackageMarkedDirty(UPackage* packagePtr, bool wasDirty)
{
    if (packagePtr == nullptr || !packagePtr->ContainsMap())
    {
        return;
    }

    // Sublevel can have parent only when world composition is enabled
    if (m_worldPtr->GetWorldSettings()->bEnableWorldComposition)
    {
        // Find level in loaded streaming levels
        ULevel* levelPtr = FindLevelInLoadedLevels(packagePtr->GetName(), false);
        if (levelPtr != nullptr && !m_uninitializedLevels.Contains(levelPtr))
        {
            m_dirtyParentLevels.Add(levelPtr);
        }
    }
}

bool sfLevelManager::OnUPropertyChange(sfObject::SPtr objPtr, UObject* uobjPtr, UProperty* upropPtr)
{
    if (PROPERTY_BLACKLIST.Contains(upropPtr->GetName()))
    {
        return true;
    }

    if (upropPtr->GetFName() == sfProp::DefaultGameMode->c_str())
    {
        FTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateLambda([this](float deltaTime)
        {
            UClass* gameModePtr = m_worldPtr->GetWorldSettings()->DefaultGameMode;
            sfObject::SPtr gameModeObjPtr = m_worldSettingsObjPtr->Child(0);
            if (gameModePtr != nullptr && gameModePtr->IsInBlueprint())
            {
                AGameModeBase* defaultObjectPtr = gameModePtr->GetDefaultObject<AGameModeBase>();
                if (gameModeObjPtr == nullptr)
                {
                    sfDictionaryProperty::SPtr gameModePropertiesPtr = sfDictionaryProperty::Create();
                    sfObject::SPtr gameModeObjPtr = sfObject::Create(sfType::GameMode, gameModePropertiesPtr);
                    sfPropertyUtil::CreateProperties(defaultObjectPtr, gameModePropertiesPtr);
                    sfObjectMap::Add(gameModeObjPtr, defaultObjectPtr);
                    m_worldSettingsObjPtr->AddChild(gameModeObjPtr);
                    m_sessionPtr->Create(gameModeObjPtr, m_worldSettingsObjPtr, 0);
                }
                else
                {
                    sfPropertyUtil::SendPropertyChanges(defaultObjectPtr, gameModeObjPtr->Property()->AsDict());
                }
            }
            else if (gameModeObjPtr != nullptr)
            {
                sfObjectMap::Remove(gameModeObjPtr);
                m_sessionPtr->Delete(gameModeObjPtr);
            }
            return false;
        }));
    }

    return false;
}

void sfLevelManager::OnTileDetailsChange(UObject* uobjPtr, UProperty* upropPtr)
{
    sfObject::SPtr levelObjPtr;
    sfDictionaryProperty::SPtr levelPropertiesPtr;
    if (!TryGetLevelObjectAndPropertyForTileDetailObject(uobjPtr, levelObjPtr, levelPropertiesPtr))
    {
        return;
    }

    bool applyServerValue = levelObjPtr->IsLocked() && upropPtr->GetFName() == sfProp::TilePosition->c_str();
    sfPropertyUtil::SyncProperty(levelPropertiesPtr->GetContainerObject(), uobjPtr, upropPtr, applyServerValue);
}

UObject* sfLevelManager::FindWorldTileDetailsObject(FString levelPath)
{
    if (m_worldTileDetailsClassPtr == nullptr || m_packageNamePropertyPtr == nullptr)
    {
        return nullptr;
    }

    TArray<UObject*> worldDetails;
    GetObjectsOfClass(
        m_worldTileDetailsClassPtr,
        worldDetails,
        false,
        RF_ClassDefaultObject,
        EInternalObjectFlags::PendingKill);

    for (auto iter = worldDetails.CreateConstIterator(); iter; ++iter)
    {
        if (levelPath == m_packageNamePropertyPtr->ContainerPtrToValuePtr<FName>(*iter)->ToString())
        {
            return *iter;
        }
    }

    return nullptr;
}

void sfLevelManager::OnUPropertyChange(UObject* uobjPtr, FPropertyChangedEvent& ev)
{
    if (!sfPropertyUtil::ListeningForPropertyChanges())
    {
        return;
    }

    if (ev.MemberProperty == nullptr)
    {
        if (!uobjPtr->IsA<UBlueprint>() && !uobjPtr->IsInBlueprint())
        {
            return;
        }

        // Send game mode changes
        UClass* classPtr = nullptr;
        UBlueprint* blueprintPtr = Cast<UBlueprint>(uobjPtr);
        if (blueprintPtr != nullptr)
        {
            classPtr = blueprintPtr->GeneratedClass;
        }
        else
        {
            classPtr = uobjPtr->GetClass();
            if (uobjPtr != classPtr->GetDefaultObject())
            {
                return;
            }
        }
        if (classPtr == nullptr || classPtr != m_worldPtr->GetWorldSettings()->DefaultGameMode)
        {
            return;
        }
        sfObject::SPtr gameModeObjPtr = m_worldSettingsObjPtr->Child(0);
        if (classPtr != nullptr && gameModeObjPtr != nullptr)
        {
            sfPropertyUtil::SendPropertyChanges(classPtr->GetDefaultObject(),
                gameModeObjPtr->Property()->AsDict());
        }
        return;
    }

    if (uobjPtr->GetClass()->GetFName() == "WorldTileDetails" &&
        ev.MemberProperty->GetFName() == sfProp::TilePosition->c_str())
    {
        sfObject::SPtr levelObjPtr;
        sfDictionaryProperty::SPtr levelPropertiesPtr;
        if (!TryGetLevelObjectAndPropertyForTileDetailObject(uobjPtr, levelObjPtr, levelPropertiesPtr))
        {
            return;
        }

        if (levelObjPtr->IsLocked())
        {
            sfProperty::SPtr oldPropPtr;
            if (levelPropertiesPtr->TryGet(sfProp::TilePosition, oldPropPtr))
            {
                sfUPropertyInstance upropInstance = sfPropertyUtil::FindUProperty(uobjPtr, oldPropPtr);
                if (upropInstance.IsValid())
                {
                    sfPropertyUtil::SetValue(uobjPtr, upropInstance, oldPropPtr);
                }
            }
        }
    }
}

bool sfLevelManager::TryGetLevelObjectAndPropertyForTileDetailObject(
    UObject* worldTileDetailPtr,
    sfObject::SPtr& levelObjPtr,
    sfDictionaryProperty::SPtr& levelPropertiesPtr)
{
    // Get level path
    UProperty* packageNamePropPtr = worldTileDetailPtr->GetClass()->FindPropertyByName(sfProp::PackageName->c_str());
    FString levelPath = packageNamePropPtr->ContainerPtrToValuePtr<FName>(worldTileDetailPtr)->ToString();

    // Get level
    ULevel* levelPtr = FindLevelInLoadedLevels(levelPath, false);
    levelObjPtr = m_levelToObjectMap.FindRef(levelPtr);
    if (levelObjPtr == nullptr)
    {
        return false;
    }
    levelPropertiesPtr = m_objectToProperty[levelObjPtr]->Property()->AsDict();
    return true;
}

void sfLevelManager::RefreshWorldSettingsTab()
{
    FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
    TSharedPtr<FTabManager> tabManager = LevelEditorModule.GetLevelEditorTabManager();
    if (tabManager.IsValid())
    {
        TSharedPtr<SDockTab> worldSettingsTab = tabManager->FindExistingLiveTab(FName("WorldSettingsTab"));
        if (worldSettingsTab.IsValid())
        {
            TSharedPtr<IDetailsView> detailView = StaticCastSharedPtr<IDetailsView>(
                sfUtils::FindWidget(worldSettingsTab->GetContent(), "SDetailsView"));
            if (detailView.IsValid())
            {
                detailView->ForceRefresh();
            }
        }
    }
}

UObject* sfLevelManager::GetUObject(sfObject::SPtr objPtr)
{
    if (objPtr == m_worldSettingsObjPtr)
    {
        m_worldSettingsDirty = true;
    }
    else if (objPtr->Type() == sfType::LevelProperties && GetWorldCompositionOnServer())
    {
        sfObject::SPtr levelObjPtr = m_propertyToObject[objPtr];
        auto levelIter = m_objectToLevelMap.find(levelObjPtr);
        if (levelIter != m_objectToLevelMap.end())
        {
            return FindWorldTileDetailsObject(levelIter->second->GetOutermost()->GetName());
        }
        return nullptr;
    }
    else if (objPtr->Type() == sfType::Level)
    {
        auto levelIter = m_objectToLevelMap.find(objPtr);
        if (levelIter == m_objectToLevelMap.end())
        {
            return nullptr;
        }
        return FLevelUtils::FindStreamingLevel(levelIter->second);
    }
    return sfObjectMap::GetUObject(objPtr);
}

#undef LOG_CHANNEL