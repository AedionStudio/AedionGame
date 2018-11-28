#include "sfActorManager.h"
#include "../Components/sfLockComponent.h"
#include "../Actors/sfMissingActor.h"
#include "../sfPropertyUtil.h"
#include "../sfActorUtil.h"
#include "../SceneFusion.h"
#include "../sfObjectMap.h"
#include "../sfUtils.h"
#include "../sfLoader.h"
#include "../UI/sfDetailsPanelManager.h"

#include <Editor.h>
#include <EngineUtils.h>
#include <ActorEditorUtils.h>
#include <Engine/StaticMeshActor.h>
#include <Engine/Selection.h>
#include <Materials/MaterialInstanceDynamic.h>
#include <Particles/Emitter.h>
#include <Particles/ParticleSystemComponent.h>
#include <EditorActorFolders.h>
#include <Animation/SkeletalMeshActor.h>
#include <Components/SkeletalMeshComponent.h>
#include <UObject/ObjectMacros.h>
#include <LevelEditor.h>
#include <LevelEditorViewport.h>
#include <UObject/GarbageCollection.h>
#include <Landscape.h>

// In seconds
#define BSP_REBUILD_DELAY 2.0f;
#define LOG_CHANNEL "sfObjectManager"

sfActorManager::sfActorManager(TSharedPtr<sfLevelManager> levelManagerPtr) :
    m_levelManagerPtr { levelManagerPtr }
{
    RegisterPropertyChangeHandlers();
}

sfActorManager::~sfActorManager()
{
    
}

void sfActorManager::Initialize()
{
    m_sessionPtr = SceneFusion::Service->Session();
    m_onActorAddedHandle = GEngine->OnLevelActorAdded().AddRaw(this, &sfActorManager::OnActorAdded);
    m_onActorDeletedHandle = GEngine->OnLevelActorDeleted().AddRaw(this, &sfActorManager::OnActorDeleted);
    m_onActorAttachedHandle = GEngine->OnLevelActorAttached().AddRaw(this, &sfActorManager::OnAttachDetach);
    m_onActorDetachedHandle = GEngine->OnLevelActorDetached().AddRaw(this, &sfActorManager::OnAttachDetach);
    m_onFolderChangeHandle = GEngine->OnLevelActorFolderChanged().AddRaw(this, &sfActorManager::OnFolderChange);
    m_onLabelChangeHandle = FCoreDelegates::OnActorLabelChanged.AddRaw(this, &sfActorManager::OnLabelChanged);
    m_onMoveStartHandle = GEditor->OnBeginObjectMovement().AddRaw(this, &sfActorManager::OnMoveStart);
    m_onMoveEndHandle = GEditor->OnEndObjectMovement().AddRaw(this, &sfActorManager::OnMoveEnd);
    m_onActorMovedHandle = GEditor->OnActorMoved().AddRaw(this, &sfActorManager::OnActorMoved);
    m_numSyncedActors = 0;
    m_movingActors = false;
    m_collectGarbage = false;
    m_bspRebuildDelay = -1.0f;
}

void sfActorManager::CleanUp()
{
    GEngine->OnLevelActorAdded().Remove(m_onActorAddedHandle);
    GEngine->OnLevelActorDeleted().Remove(m_onActorDeletedHandle);
    GEngine->OnLevelActorAttached().Remove(m_onActorAttachedHandle);
    GEngine->OnLevelActorDetached().Remove(m_onActorDetachedHandle);
    GEngine->OnLevelActorFolderChanged().Remove(m_onFolderChangeHandle);
    FCoreDelegates::OnActorLabelChanged.Remove(m_onLabelChangeHandle);
    GEditor->OnBeginObjectMovement().Remove(m_onMoveStartHandle);
    GEditor->OnEndObjectMovement().Remove(m_onMoveEndHandle);
    GEditor->OnActorMoved().Remove(m_onActorMovedHandle);

    UWorld* world = GEditor->GetEditorWorldContext().World();
    for (TActorIterator<AActor> iter(world); iter; ++iter)
    {
        sfObject::SPtr objPtr = sfObjectMap::GetSFObject(*iter);
        if (objPtr != nullptr && objPtr->IsLocked())
        {
            Unlock(*iter);
        }
    }

    m_uploadList.Empty();
    m_recreateQueue.Empty();
    m_revertFolderQueue.Empty();
    m_syncParentList.Empty();
    m_foldersToCheck.Empty();
    m_selectedActors.clear();
    m_movedActors.Empty();
}

void sfActorManager::Tick(float deltaTime)
{
    // Create server objects for actors in the upload list
    if (m_uploadList.Num() > 0)
    {
        UploadActors(m_uploadList);
        m_uploadList.Empty();
    }

    // Check for selection changes and request locks/unlocks
    UpdateSelection();

    // Send actor transform changes for moved actors
    for (AActor* actorPtr : m_movedActors)
    {
        SyncComponentTransforms(actorPtr);
    }
    m_movedActors.Empty();

    // Revert folders to server values for actors whose folder changed while locked
    if (!m_revertFolderQueue.IsEmpty())
    {
        sfUtils::PreserveUndoStack([this]()
        {
            RevertLockedFolders();
        });
    }

    // Recreate actors that were deleted while locked.
    RecreateLockedActors();

    // Send parent changes for attached/detached actors or reset them to server values if they are locked
    for (AActor* actorPtr : m_syncParentList)
    {
        sfObject::SPtr objPtr = sfObjectMap::GetSFObject(actorPtr);
        if (objPtr != nullptr && objPtr->IsSyncing())
        {
            SyncParent(actorPtr, objPtr);
        }
    }
    m_syncParentList.Empty();

    // Empty folders are gone when you reload a level, so we delete folders that become empty
    if (m_foldersToCheck.Num() > 0)
    {
        sfUtils::PreserveUndoStack([this]()
        {
            DeleteEmptyFolders();
        });
    }

    // Garbage collection
    if (m_collectGarbage)
    {
        m_collectGarbage = false;
        CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
    }

    // Rebuild BSP
    RebuildBSPIfNeeded(deltaTime);
}

void sfActorManager::UpdateSelection()
{
    TSet<AActor*> selectedActors = sfDetailsPanelManager::Get().GetSelectedActors();
    // Unreal doesn't have deselect events and doesn't fire select events when selecting through the World Outliner so
    // we have to iterate the selection to check for changes
    for (auto iter = m_selectedActors.cbegin(); iter != m_selectedActors.cend();)
    {
        if (m_movingActors)
        {
            SyncComponentTransforms(iter->first);
            m_movedActors.Remove(iter->first);
        }
        SceneFusion::ComponentManager->SyncComponents(iter->first, iter->second);
        if (!selectedActors.Contains(iter->first))
        {
            iter->second->ReleaseLock();
            m_selectedActors.erase(iter++);
        }
        else
        {
            ++iter;
        }
    }

    for (AActor* actorPtr : selectedActors)
    {
        if (m_selectedActors.find(actorPtr) != m_selectedActors.end())
        {
            continue;
        }
        sfObject::SPtr objPtr = sfObjectMap::GetSFObject(actorPtr);
        if (objPtr != nullptr && objPtr->IsSyncing())
        {
            objPtr->RequestLock();
            m_selectedActors[actorPtr] = objPtr;
            if (m_movingActors)
            {
                sfLoader::Get().LoadAssetsFor(objPtr);
            }
        }
    }
}

void sfActorManager::DestroyActor(AActor* actorPtr)
{
    if (actorPtr->IsA<ABrush>())
    {
        m_bspRebuildDelay = BSP_REBUILD_DELAY;
    }
    if (actorPtr->IsSelected())
    {
        // Unselect the actor before deleting it to avoid UI bugs/crashes
        GEditor->SelectActor(actorPtr, false, true);
        // We need to update the SSCEditor tree in the details panel to avoid a crash if the user was renaming a
        // component of the deleted actor. The crash occurs the next time the user selects something.
        sfDetailsPanelManager::Get().UpdateDetailsPanelTree();
    }
    UWorld* worldPtr = GEditor->GetEditorWorldContext().World();
    GEngine->OnLevelActorDeleted().Remove(m_onActorDeletedHandle);
    worldPtr->EditorDestroyActor(actorPtr, true);
    m_collectGarbage = true;// Collect garbage to set references to this actor to nullptr
    m_onActorDeletedHandle = GEngine->OnLevelActorDeleted().AddRaw(this,
        &sfActorManager::OnActorDeleted);
    SceneFusion::RedrawActiveViewport();
}

void sfActorManager::DestroyUnsyncedActorsInLevel(ULevel* levelPtr)
{
    for (AActor* actorPtr : levelPtr->Actors)
    {
        if (IsSyncable(actorPtr) && !sfObjectMap::Contains(actorPtr))
        {
            DestroyActor(actorPtr);
        }
    }
}

void sfActorManager::DestroyUnsyncedComponents(AActor* actorPtr)
{
    TInlineComponentArray<UActorComponent*> components;
    actorPtr->GetComponents(components);
    for (UActorComponent* componentPtr : components)
    {
        if (!sfObjectMap::Contains(componentPtr) && SceneFusion::ComponentManager->IsSyncable(componentPtr))
        {
            componentPtr->DestroyComponent();
            SceneFusion::RedrawActiveViewport();
        }
    }
}

void sfActorManager::RevertLockedFolders()
{
    while (!m_revertFolderQueue.IsEmpty())
    {
        AActor* actorPtr;
        m_revertFolderQueue.Dequeue(actorPtr);
        sfObject::SPtr objPtr = sfObjectMap::GetSFObject(actorPtr);
        if (objPtr != nullptr && objPtr->IsSyncing())
        {
            sfDictionaryProperty::SPtr propertiesPtr = objPtr->Property()->AsDict();
            GEngine->OnLevelActorFolderChanged().Remove(m_onFolderChangeHandle);
            actorPtr->SetFolderPath(FName(*sfPropertyUtil::ToString(propertiesPtr->Get(sfProp::Folder))));
            m_onFolderChangeHandle = GEngine->OnLevelActorFolderChanged().AddRaw(
                this, &sfActorManager::OnFolderChange);
        }
    }
}

void sfActorManager::RecreateLockedActors()
{
    while (!m_recreateQueue.IsEmpty())
    {
        sfObject::SPtr objPtr;
        m_recreateQueue.Dequeue(objPtr);
        if (!sfObjectMap::Contains(objPtr))
        {
            OnCreate(objPtr, 0);
        }
    }
}

void sfActorManager::DeleteEmptyFolders()
{
    // The only way to tell if a folder is empty is to iterate all the actors
    if (m_foldersToCheck.Num() > 0 && FActorFolders::IsAvailable())
    {
        UWorld* world = GEditor->GetEditorWorldContext().World();
        for (TActorIterator<AActor> iter(world); iter && m_foldersToCheck.Num() > 0; ++iter)
        {
            FString folder = iter->GetFolderPath().ToString();
            for (int i = m_foldersToCheck.Num() - 1; i >= 0; i--)
            {
                if (folder == m_foldersToCheck[i] || FActorFolders::Get().PathIsChildOf(folder, m_foldersToCheck[i]))
                {
                    m_foldersToCheck.RemoveAt(i);
                    break;
                }
            }
        }
        for (int i = 0; i < m_foldersToCheck.Num(); i++)
        {
            FActorFolders::Get().DeleteFolder(*world, FName(*m_foldersToCheck[i]));
        }
        m_foldersToCheck.Empty();
    }
}

void sfActorManager::RebuildBSPIfNeeded(float deltaTime)
{
    if (m_bspRebuildDelay >= 0.0f)
    {
        m_bspRebuildDelay -= deltaTime;
        if (m_bspRebuildDelay < 0.0f)
        {
            SceneFusion::RedrawActiveViewport();
            GEditor->RebuildAlteredBSP();
        }
    }
}

void sfActorManager::MarkBSPStale(AActor* actorPtr)
{
    ABrush::SetNeedRebuild(actorPtr->GetLevel());
    m_bspRebuildDelay = BSP_REBUILD_DELAY;
}

bool sfActorManager::IsSyncable(AActor* actorPtr)
{
    return actorPtr != nullptr && actorPtr->GetWorld() == GEditor->GetEditorWorldContext().World() &&
        !actorPtr->bHiddenEdLayer && actorPtr->IsEditable() && actorPtr->IsListedInSceneOutliner() &&
        !actorPtr->IsPendingKill() && (actorPtr->GetFlags() & EObjectFlags::RF_Transient) == 0 &&
        !FActorEditorUtils::IsABuilderBrush(actorPtr) &&
        !actorPtr->IsA(AWorldSettings::StaticClass());
}

void sfActorManager::OnActorAdded(AActor* actorPtr)
{
    // Ignore actors in the buffer level.
    // The buffer level is a temporary level used when moving actors to a different level.
    if (actorPtr->GetOutermost() == GetTransientPackage())
    {
        return;
    }

    // We add this to a list for processing later because the actor's properties may not be initialized yet.
    m_uploadList.Add(actorPtr);
}

void sfActorManager::UploadActors(const TArray<AActor*>& actors)
{
    std::list<sfObject::SPtr> objects;
    sfObject::SPtr parentPtr = nullptr;
    sfObject::SPtr currentParentPtr = nullptr;
    for (AActor* actorPtr : actors)
    {
        if (!IsSyncable(actorPtr))
        {
            continue;
        }

        USceneComponent* parentComponentPtr = actorPtr->GetRootComponent() == nullptr ?
            nullptr :actorPtr->GetRootComponent()->GetAttachParent();
        if (parentComponentPtr == nullptr)
        {
            currentParentPtr = m_levelManagerPtr->GetLevelObject(actorPtr->GetLevel());
        }
        else
        {
            currentParentPtr = sfObjectMap::GetSFObject(parentComponentPtr);
        }

        if (currentParentPtr == nullptr || !currentParentPtr->IsSyncing())
        {
            continue;
        }
        else if (currentParentPtr->IsFullyLocked())
        {
            KS::Log::Warning("Failed to attach " + std::string(TCHAR_TO_UTF8(*actorPtr->GetName())) +
                " to " + std::string(TCHAR_TO_UTF8(*parentComponentPtr->GetOwner()->GetName())) +
                " because it is fully locked by another user.",
                LOG_CHANNEL);
            DisableParentChangeHandler();
            actorPtr->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
            EnableParentChangeHandler();
            currentParentPtr = m_levelManagerPtr->GetLevelObject(actorPtr->GetLevel());
        }

        if (parentPtr == nullptr)
        {
            parentPtr = currentParentPtr;
        }

        // All objects in one request must have the same parent, so if we encounter a different parent, send a request
        // for all objects we already processed and clear the objects list to start a new request.
        if (currentParentPtr != parentPtr)
        {
            if (objects.size() > 0)
            {
                m_sessionPtr->Create(objects, parentPtr, 0);
                // Pre-existing child objects can only be attached after calling Create.
                FindAndAttachChildren(objects);
                objects.clear();
            }
            parentPtr = currentParentPtr;
        }
        sfObject::SPtr objPtr = CreateObject(actorPtr);
        if (objPtr != nullptr)
        {
            objects.push_back(objPtr);
        }
    }
    if (objects.size() > 0)
    {
        m_sessionPtr->Create(objects, parentPtr, 0);
        // Pre-existing child objects can only be attached after calling Create.
        FindAndAttachChildren(objects);
    }
}

void sfActorManager::FindAndAttachChildren(const std::list<sfObject::SPtr>& objects)
{
    for (sfObject::SPtr objPtr : objects)
    {
        auto iter = objPtr->SelfAndDescendants();
        while (iter.Value() != nullptr)
        {
            AActor* actorPtr = sfObjectMap::Get<AActor>(iter.Value());
            iter.Next();
            if (actorPtr != nullptr)
            {
                TArray<AActor*> children;
                actorPtr->GetAttachedActors(children);
                for (AActor* childPtr : children)
                {
                    sfObject::SPtr childObjPtr = sfObjectMap::GetSFObject(childPtr);
                    if (childObjPtr == nullptr)
                    {
                        continue;
                    }
                    USceneComponent* childRootPtr = childPtr->GetRootComponent();
                    if (childRootPtr == nullptr)
                    {
                        // This can happen after an undo delete if the child was deleted by another user
                        continue;
                    }
                    sfObject::SPtr parentObjPtr = sfObjectMap::GetSFObject(childRootPtr->GetAttachParent());
                    if (parentObjPtr != nullptr && childObjPtr->Parent() != parentObjPtr)
                    {
                        parentObjPtr->AddChild(childObjPtr);
                        SceneFusion::ComponentManager->SyncTransform(childPtr->GetRootComponent());
                    }
                }
            }
        }
    }
}

sfObject::SPtr sfActorManager::CreateObject(AActor* actorPtr)
{
    if (!m_levelManagerPtr->IsLevelObjectInitialized(actorPtr->GetLevel()))
    {
        return nullptr;
    }
    sfObject::SPtr objPtr = sfObjectMap::GetOrCreateSFObject(actorPtr, sfType::Actor);
    if (objPtr->IsSyncing())
    {
        return nullptr;
    }
    sfDictionaryProperty::SPtr propertiesPtr = objPtr->Property()->AsDict();

    if (actorPtr->IsSelected())
    {
        objPtr->RequestLock();
        m_selectedActors[actorPtr] = objPtr;
    }

    FString className;
    AsfMissingActor* missingActorPtr = Cast<AsfMissingActor>(actorPtr);
    if (missingActorPtr != nullptr)
    {
        // This is a stand-in for a missing actor class.
        SceneFusion::MissingObjectManager->AddStandIn(missingActorPtr);
        className = missingActorPtr->MissingClass();
    }
    else
    {
        className = sfUtils::ClassToFString(actorPtr->GetClass());
    }

    propertiesPtr->Set(sfProp::Name, sfPropertyUtil::FromString(actorPtr->GetName()));
    propertiesPtr->Set(sfProp::Class, sfPropertyUtil::FromString(className));
    propertiesPtr->Set(sfProp::Label, sfPropertyUtil::FromString(actorPtr->GetActorLabel()));
    propertiesPtr->Set(sfProp::Folder, sfPropertyUtil::FromString(actorPtr->GetFolderPath().ToString()));
    sfPropertyUtil::CreateProperties(actorPtr, propertiesPtr);

    USceneComponent* rootComponentPtr = actorPtr->GetRootComponent();
    if (rootComponentPtr != nullptr)
    {
        sfObject::SPtr childPtr = SceneFusion::ComponentManager->CreateObject(rootComponentPtr);
        if (childPtr != nullptr)
        {
            childPtr->Property()->AsDict()->Set(sfProp::IsRoot, sfValueProperty::Create(true));
            objPtr->AddChild(childPtr);
        }
    }

    // Create objects for non-scene components
    for (UActorComponent* componentPtr : actorPtr->GetComponents())
    {
        if (!SceneFusion::ComponentManager->IsSyncable(componentPtr))
        {
            continue;
        }
        sfObject::SPtr childPtr = sfObjectMap::GetSFObject(componentPtr);
        if (childPtr != nullptr && childPtr->Property()->AsDict()->Size() > 0)
        {
            continue;
        }
        childPtr = SceneFusion::ComponentManager->CreateObject(componentPtr);
        if (childPtr != nullptr)
        {
            objPtr->AddChild(childPtr);
        }
    }

    InvokeOnLockStateChange(objPtr, actorPtr);

    m_numSyncedActors++;
    return objPtr;
}

void sfActorManager::OnCreate(sfObject::SPtr objPtr, int childIndex)
{
    sfObject::SPtr levelObjectPtr = objPtr->Parent();
    if (levelObjectPtr == nullptr)
    {
        LogNoParentErrorAndDisconnect(objPtr);
        return;
    }

    while (levelObjectPtr->Parent() != nullptr)
    {
        levelObjectPtr = levelObjectPtr->Parent();
    }

    ULevel* levelPtr = m_levelManagerPtr->FindLevelByObject(levelObjectPtr);
    if (!levelPtr)
    {
        return;
    }
    AActor* actorPtr = InitializeActor(objPtr, levelPtr);
    if (actorPtr == nullptr)
    {
        return;
    }

    if (DetachIfParentIsLevel(objPtr, actorPtr))
    {
        return;
    }
    USceneComponent* parentPtr = sfObjectMap::Get<USceneComponent>(objPtr->Parent());
    if (parentPtr != nullptr)
    {
        DisableParentChangeHandler();
        actorPtr->AttachToComponent(parentPtr, FAttachmentTransformRules::KeepRelativeTransform);
        EnableParentChangeHandler();
    }
}

AActor* sfActorManager::InitializeActor(sfObject::SPtr objPtr, ULevel* levelPtr)
{
    sfDictionaryProperty::SPtr propertiesPtr = objPtr->Property()->AsDict();
    FString className = sfPropertyUtil::ToString(propertiesPtr->Get(sfProp::Class));
    UClass* classPtr = sfUtils::LoadClass(className);
    FString name = sfPropertyUtil::ToString(propertiesPtr->Get(sfProp::Name));
    AActor* actorPtr = sfActorUtil::FindActorWithNameInLevel(levelPtr, name);

    if (actorPtr != nullptr)
    {
        if (actorPtr->IsPendingKill())
        {
            // Rename the deleted actor so we can reuse its name.
            sfUtils::Rename(actorPtr, name + " (deleted)");
            actorPtr = nullptr;
        }
        else if (sfObjectMap::Contains(actorPtr) || (classPtr != nullptr && actorPtr->GetClass() != classPtr))
        {
            actorPtr = nullptr;
        }
    }

    if (actorPtr == nullptr)
    {
        bool isClassMissing = classPtr == nullptr;
        if (isClassMissing)
        {
            classPtr = AsfMissingActor::StaticClass();
        }
        GEngine->OnLevelActorAdded().Remove(m_onActorAddedHandle);
        UWorld* worldPtr = GEditor->GetEditorWorldContext().World();
        FActorSpawnParameters spawnParameters;
        spawnParameters.OverrideLevel = levelPtr;
        actorPtr = worldPtr->SpawnActor<AActor>(classPtr, spawnParameters);
        ALandscape* landscapePtr = Cast<ALandscape>(actorPtr);
        // Create empty landscape
        if (landscapePtr != nullptr)
        {
            landscapePtr->SetLandscapeGuid(FGuid::NewGuid());
        }
        sfActorUtil::UpdateActorVisibilityWithLevel(actorPtr);
        m_onActorAddedHandle = GEngine->OnLevelActorAdded().AddRaw(this, &sfActorManager::OnActorAdded);
        if (isClassMissing)
        {
            AsfMissingActor* missingActorPtr = Cast<AsfMissingActor>(actorPtr);
            missingActorPtr->ClassName = className;
            SceneFusion::MissingObjectManager->AddStandIn(missingActorPtr);
        }
    }
    else
    {
        // Detach from parent to avoid possible loops when we try to attach its children
        DisableParentChangeHandler();
        actorPtr->DetachFromActor(FDetachmentTransformRules::KeepRelativeTransform);
        EnableParentChangeHandler();
        if (actorPtr->IsSelected())
        {
            objPtr->RequestLock();
            m_selectedActors[actorPtr] = objPtr;
        }
        if (actorPtr->IsA<ABrush>())
        {
            ABrush::SetNeedRebuild(actorPtr->GetLevel());
            m_bspRebuildDelay = BSP_REBUILD_DELAY;
        }
    }
    sfObjectMap::Add(objPtr, actorPtr);

    actorPtr->SetFolderPath(FName(*sfPropertyUtil::ToString(propertiesPtr->Get(sfProp::Folder))));

    FString label = sfPropertyUtil::ToString(propertiesPtr->Get(sfProp::Label));
    // Calling SetActorLabel will change the actor's name (id), even if the label doesn't change. So we check first if
    // the label is different
    if (label != actorPtr->GetActorLabel())
    {
        FCoreDelegates::OnActorLabelChanged.Remove(m_onLabelChangeHandle);
        actorPtr->SetActorLabel(sfPropertyUtil::ToString(propertiesPtr->Get(sfProp::Label)));
        m_onLabelChangeHandle = FCoreDelegates::OnActorLabelChanged.AddRaw(this, &sfActorManager::OnLabelChanged);
    }
    // Set name after setting label because setting label changes the name
    sfUtils::TryRename(actorPtr, name);

    sfPropertyUtil::ApplyProperties(actorPtr, propertiesPtr);

    // Set references to this actor
    std::vector<sfReferenceProperty::SPtr> references = m_sessionPtr->GetReferences(objPtr);
    sfPropertyUtil::SetReferences(actorPtr, references);

    SceneFusion::RedrawActiveViewport();

    // Initialize children
    actorPtr->SetRootComponent(nullptr);
    for (sfObject::SPtr childPtr : objPtr->Children())
    {
        UActorComponent* componentPtr = SceneFusion::ComponentManager->InitializeComponent(actorPtr, childPtr);
        sfProperty::SPtr propPtr;
        if (childPtr->Property()->AsDict()->TryGet(sfProp::IsRoot, propPtr) && (bool)propPtr->AsValue()->GetValue())
        {
            USceneComponent* sceneComponentPtr = Cast<USceneComponent>(componentPtr);
            if (sceneComponentPtr != nullptr)
            {
                actorPtr->SetRootComponent(sceneComponentPtr);
            }
        }
    }
    DestroyUnsyncedComponents(actorPtr);

    if (objPtr->IsLocked())
    {
        OnLock(objPtr);
    }
    InvokeOnLockStateChange(objPtr, actorPtr);
   
    sfActorUtil::Reselect(actorPtr);
    m_numSyncedActors++;
    return actorPtr;
}

void sfActorManager::OnActorDeleted(AActor* actorPtr)
{
    // Ignore actors in the buffer level.
    // The buffer level is a temporary level used when moving actors to a different level.
    if (actorPtr->GetOutermost() == GetTransientPackage())
    {
        return;
    }
    sfObject::SPtr objPtr = sfObjectMap::Remove(actorPtr);
    if (objPtr != nullptr && objPtr->IsSyncing())
    {
        m_numSyncedActors--;
        if (objPtr->IsLocked())
        {
            objPtr->ReleaseLock();
            CleanUpChildrenOfDeletedObject(objPtr);
            m_recreateQueue.Enqueue(objPtr);
        }
        else
        {
            // Attach child actor objects to level object before deleting the object
            sfObject::SPtr levelObjPtr = m_levelManagerPtr->GetLevelObject(actorPtr->GetLevel());
            CleanUpChildrenOfDeletedObject(objPtr, levelObjPtr);
            m_sessionPtr->Delete(objPtr);
        }
    }
    m_uploadList.Remove(actorPtr);
    m_selectedActors.erase(actorPtr);
    if (m_selectedActors.size() == 0)
    {
        m_movingActors = false;
    }
    m_movedActors.Remove(actorPtr);
}

void sfActorManager::CleanUpChildrenOfDeletedObject(sfObject::SPtr objPtr, sfObject::SPtr levelObjPtr,
    bool recurseChildActors)
{
    for (int i = objPtr->Children().size() - 1; i >= 0; i--)
    {
        sfObject::SPtr childPtr = objPtr->Child(i);
        if (childPtr->Type() == sfType::Actor)
        {
            AActor* childActorPtr = sfObjectMap::Get<AActor>(childPtr);
            if (recurseChildActors || (childActorPtr != nullptr && childActorPtr->IsPendingKill()))
            {
                // Destroy the actor if it's not already destroyed
                if (childActorPtr != nullptr && !childActorPtr->IsPendingKill())
                {
                    DestroyActor(childActorPtr);
                }
                if (sfObjectMap::Remove(childPtr) != nullptr)
                {
                    m_numSyncedActors--;
                }
                CleanUpChildrenOfDeletedObject(childPtr, levelObjPtr, recurseChildActors);
            }
            else if (levelObjPtr != nullptr)
            {
                // Add the actor's object to the level object and sync the transform
                levelObjPtr->AddChild(childPtr);
                if (childActorPtr != nullptr)
                {
                    SceneFusion::ComponentManager->SyncTransform(childActorPtr->GetRootComponent());
                }
            }
        }
        else
        {
            // Destroy the component if it's not already destroyed
            UActorComponent* componentPtr = sfObjectMap::Get<UActorComponent>(childPtr);
            if (componentPtr != nullptr && !componentPtr->IsPendingKill())
            {
                componentPtr->DestroyComponent();
            }
            sfObjectMap::Remove(childPtr);
            CleanUpChildrenOfDeletedObject(childPtr, levelObjPtr, recurseChildActors);
        }
    }
}

void sfActorManager::OnDelete(sfObject::SPtr objPtr)
{
    AActor* actorPtr = Cast<AActor>(sfObjectMap::Remove(objPtr));
    if (actorPtr == nullptr)
    {
        return;
    }
    m_numSyncedActors--;
    CleanUpChildrenOfDeletedObject(objPtr, nullptr, true);
    DestroyActor(actorPtr);
}

void sfActorManager::OnLock(sfObject::SPtr objPtr)
{
    AActor* actorPtr = sfObjectMap::Get<AActor>(objPtr);
    if (actorPtr == nullptr)
    {
        OnCreate(objPtr, 0);
        return;
    }
    InvokeOnLockStateChange(objPtr, actorPtr);
    Lock(actorPtr, objPtr);
}

void sfActorManager::Lock(AActor* actorPtr, sfObject::SPtr objPtr)
{
    if (actorPtr->bLockLocation)
    {
        // Actor is already locked
        return;
    }
    UMaterialInterface* lockMaterialPtr = SceneFusion::GetLockMaterial(objPtr->LockOwner());
    if (lockMaterialPtr != nullptr)
    {
        TArray<UMeshComponent*> meshes;
        actorPtr->GetComponents(meshes);
        if (meshes.Num() > 0)
        {
            for (int i = 0; i < meshes.Num(); i++)
            {
                UsfLockComponent* lockPtr = NewObject<UsfLockComponent>(actorPtr, *FString("SFLock" + FString::FromInt(i)));
                lockPtr->SetMobility(meshes[i]->Mobility);
                lockPtr->AttachToComponent(meshes[i], FAttachmentTransformRules::KeepRelativeTransform);
                lockPtr->RegisterComponent();
                lockPtr->InitializeComponent();
                lockPtr->DuplicateParentMesh(lockMaterialPtr);
                SceneFusion::RedrawActiveViewport();
            }
            return;
        }
    }
    UsfLockComponent* lockPtr = NewObject<UsfLockComponent>(actorPtr, *FString("SFLock"));
    lockPtr->AttachToComponent(actorPtr->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    lockPtr->RegisterComponent();
    lockPtr->InitializeComponent();
}

void sfActorManager::OnUnlock(sfObject::SPtr objPtr)
{
    AActor* actorPtr = sfObjectMap::Get<AActor>(objPtr);
    if (actorPtr != nullptr)
    {
        Unlock(actorPtr);
        InvokeOnLockStateChange(objPtr, actorPtr);
    }
}

void sfActorManager::Unlock(AActor* actorPtr)
{
    // If you undo the deletion of an actor with lock components, the lock components will not be part of the
    // OwnedComponents set so we have to use our own function to find them instead of AActor->GetComponents.
    // Not sure why this happens. It seems like an Unreal bug.
    TArray<UsfLockComponent*> locks;
    sfActorUtil::GetSceneComponents<UsfLockComponent>(actorPtr, locks);
    if (locks.Num() == 0)
    {
        if (!actorPtr->bLockLocation)
        {
            return;
        }
        actorPtr->bLockLocation = false;
    }
    for (UsfLockComponent* lockPtr : locks)
    {
        lockPtr->DestroyComponent();
        SceneFusion::RedrawActiveViewport();
    }
    // When a selected actor becomes unlocked you have to unselect and reselect it to unlock the handles
    sfActorUtil::Reselect(actorPtr);
}

void sfActorManager::OnLockOwnerChange(sfObject::SPtr objPtr)
{
    AActor* actorPtr = sfObjectMap::Get<AActor>(objPtr);
    if (actorPtr == nullptr)
    {
        return;
    }

    InvokeOnLockStateChange(objPtr, actorPtr);

    UMaterialInterface* lockMaterialPtr = SceneFusion::GetLockMaterial(objPtr->LockOwner());
    if (lockMaterialPtr == nullptr)
    {
        return;
    }
    TArray<UsfLockComponent*> locks;
    sfActorUtil::GetSceneComponents<UsfLockComponent>(actorPtr, locks);
    for (UsfLockComponent* lockPtr : locks)
    {
        lockPtr->SetMaterial(lockMaterialPtr);
    }
}

void sfActorManager::OnAttachDetach(AActor* actorPtr, const AActor* parentPtr)
{
    // Unreal fires the detach event before updating the relative transform, and if we need to change the parent back
    // because of locks Unreal won't let us here, so we queue the actor to be processed later.
    m_syncParentList.AddUnique(actorPtr);
}

void sfActorManager::EnableParentChangeHandler()
{
    m_onActorAttachedHandle = GEngine->OnLevelActorAttached().AddRaw(this, &sfActorManager::OnAttachDetach);
    m_onActorDetachedHandle = GEngine->OnLevelActorDetached().AddRaw(this, &sfActorManager::OnAttachDetach);
}

void sfActorManager::DisableParentChangeHandler()
{
    GEngine->OnLevelActorAttached().Remove(m_onActorAttachedHandle);
    GEngine->OnLevelActorDetached().Remove(m_onActorDetachedHandle);
}

void sfActorManager::OnParentChange(sfObject::SPtr objPtr, int childIndex)
{
    AActor* actorPtr = sfObjectMap::Get<AActor>(objPtr);
    if (actorPtr == nullptr)
    {
        return;
    }
    if (objPtr->Parent() == nullptr)
    {
        LogNoParentErrorAndDisconnect(objPtr);
    }
    else if (!DetachIfParentIsLevel(objPtr, actorPtr))
    {
        USceneComponent* parentPtr = sfObjectMap::Get<USceneComponent>(objPtr->Parent());
        if (parentPtr != nullptr)
        {
            DisableParentChangeHandler();
            actorPtr->AttachToComponent(parentPtr, FAttachmentTransformRules::KeepRelativeTransform);
            EnableParentChangeHandler();
        }
    }
}

void sfActorManager::OnFolderChange(const AActor* actorPtr, FName oldFolder)
{
    sfObject::SPtr objPtr = sfObjectMap::GetSFObject(actorPtr);
    if (objPtr == nullptr || !objPtr->IsSyncing())
    {
        return;
    }
    sfDictionaryProperty::SPtr propertiesPtr = objPtr->Property()->AsDict();
    if (objPtr->IsLocked())
    {
        // Reverting the folder now can break the world outliner, so we queue it to be done on the next tick
        m_revertFolderQueue.Enqueue(const_cast<AActor*>(actorPtr));
    }
    else
    {
        propertiesPtr->Set(sfProp::Folder,
            sfPropertyUtil::FromString(actorPtr->GetFolderPath().ToString()));
    }
}

void sfActorManager::OnMoveStart(UObject& obj)
{
    m_movingActors = GCurrentLevelEditingViewportClient &&
        GCurrentLevelEditingViewportClient->bWidgetAxisControlledByDrag;
}

void sfActorManager::OnMoveEnd(UObject& obj)
{
    m_movingActors = false;
    for (auto iter : m_selectedActors)
    {
        SyncComponentTransforms(iter.first);
    }
}

void sfActorManager::OnActorMoved(AActor* actorPtr)
{
    if (sfPropertyUtil::ListeningForPropertyChanges() &&
        actorPtr->GetWorld() == GEditor->GetEditorWorldContext().World())
    {
        m_movedActors.Add(actorPtr);
    }
}

void sfActorManager::SyncComponentTransforms(AActor* actorPtr)
{
    TArray<USceneComponent*> sceneComponents;
    actorPtr->GetComponents(sceneComponents);
    for (USceneComponent* componentPtr : sceneComponents)
    {
        SceneFusion::ComponentManager->SyncTransform(componentPtr);
    }
}

bool sfActorManager::OnUndoRedo(sfObject::SPtr objPtr, UObject* uobjPtr)
{
    AActor* actorPtr = Cast<AActor>(uobjPtr);
    if (actorPtr == nullptr)
    {
        return false;
    }
    if (actorPtr->IsPendingKill())
    {
        OnActorDeleted(actorPtr);
    }
    else if (objPtr == nullptr)
    {
        OnUndoDelete(actorPtr);
    }
    else
    {
        sfDictionaryProperty::SPtr propertiesPtr = objPtr->Property()->AsDict();
        SyncLabelAndName(actorPtr, objPtr, propertiesPtr);
        SyncFolder(actorPtr, objPtr, propertiesPtr);
        if (objPtr->IsLocked())
        {
            actorPtr->bLockLocation = true;
            sfPropertyUtil::ApplyProperties(actorPtr, propertiesPtr);
        }
        else
        {
            actorPtr->bLockLocation = false;
            sfPropertyUtil::SendPropertyChanges(actorPtr, propertiesPtr);
        }
    }
    return true;
}

void sfActorManager::OnUndoDelete(AActor* actorPtr)
{
    if (!IsSyncable(actorPtr))
    {
        return;
    }
    bool inLevel = false;
    UWorld* worldPtr = GEditor->GetEditorWorldContext().World();
    for (AActor* existActorPtr : actorPtr->GetLevel()->Actors)
    {
        if (existActorPtr == nullptr)
        {
            continue;
        }

        if (existActorPtr == actorPtr)
        {
            inLevel = true;
        }
        else if (existActorPtr->GetFName() == actorPtr->GetFName())
        {
            // An actor with the same name already exists. Rename and delete the one that was just created. Although we
            // will delete it, we still need to rename it because names of deleted actors are still in use.
            sfUtils::Rename(actorPtr, actorPtr->GetName() + " (deleted)");
            DestroyActor(actorPtr);
            return;
        }
    }
    if (!inLevel)
    {
        // The actor is not in the world. This means the actor was deleted by another user and should not be recreated,
        // so we delete it.
        DestroyActor(actorPtr);
        return;
    }
    // If the actor was locked when it was deleted, it will still have a lock component, so we need to unlock it.
    Unlock(actorPtr);
    m_uploadList.AddUnique(actorPtr);
}

void sfActorManager::SyncLabelAndName(
    AActor* actorPtr,
    sfObject::SPtr objPtr,
    sfDictionaryProperty::SPtr propertiesPtr)
{
    if (propertiesPtr != nullptr)
    {
        if (objPtr->IsLocked())
        {
            FCoreDelegates::OnActorLabelChanged.Remove(m_onLabelChangeHandle);
            actorPtr->SetActorLabel(sfPropertyUtil::ToString(propertiesPtr->Get(sfProp::Label)));
            m_onLabelChangeHandle = FCoreDelegates::OnActorLabelChanged.AddRaw(this, &sfActorManager::OnLabelChanged);
            sfUtils::TryRename(actorPtr, sfPropertyUtil::ToString(propertiesPtr->Get(sfProp::Name)));
        }
        else
        {
            propertiesPtr->Set(sfProp::Label, sfPropertyUtil::FromString(actorPtr->GetActorLabel()));
            FString name = actorPtr->GetName();
            if (sfPropertyUtil::ToString(propertiesPtr->Get(sfProp::Name)) != name)
            {
                propertiesPtr->Set(sfProp::Name, sfPropertyUtil::FromString(name));
            }
        }
    }
}

void sfActorManager::SyncFolder(AActor* actorPtr, sfObject::SPtr objPtr, sfDictionaryProperty::SPtr propertiesPtr)
{
    if (propertiesPtr != nullptr)
    {
        FString newFolder = actorPtr->GetFolderPath().ToString();
        if (newFolder != sfPropertyUtil::ToString(propertiesPtr->Get(sfProp::Folder)))
        {
            if (objPtr->IsLocked())
            {
                // Setting folder during a transaction causes a crash, so we queue it to be done on the next tick
                m_revertFolderQueue.Enqueue(actorPtr);
            }
            else
            {
                propertiesPtr->Set(sfProp::Folder, sfPropertyUtil::FromString(newFolder));
            }
        }
    }
}

void sfActorManager::SyncParent(AActor* actorPtr, sfObject::SPtr objPtr)
{
    if (objPtr == nullptr)
    {
        return;
    }

    sfObject::SPtr parentPtr = nullptr;
    if (actorPtr->GetAttachParentActor() != nullptr)
    {
        parentPtr = sfObjectMap::GetSFObject(actorPtr->GetRootComponent()->GetAttachParent());
    }
    if (parentPtr == nullptr || !parentPtr->IsSyncing())
    {
        parentPtr = m_levelManagerPtr->GetLevelObject(actorPtr->GetLevel());
    }
    if (parentPtr == objPtr->Parent())
    {
        return;
    }
    if (objPtr->IsLocked() || (parentPtr != nullptr && parentPtr->IsFullyLocked()))
    {
        if (objPtr->Parent() == nullptr)
        {
            if (objPtr->IsSyncing())
            {
                LogNoParentErrorAndDisconnect(objPtr);
            }
            return;
        }

        if (DetachIfParentIsLevel(objPtr, actorPtr))
        {
            SceneFusion::ComponentManager->SyncTransform(actorPtr->GetRootComponent());
            return;
        }

        USceneComponent* componentPtr = sfObjectMap::Get<USceneComponent>(objPtr->Parent());
        if (componentPtr == nullptr)
        {
            return;
        }
        DisableParentChangeHandler();
        actorPtr->AttachToComponent(componentPtr, FAttachmentTransformRules::KeepRelativeTransform);
        EnableParentChangeHandler();
        SceneFusion::ComponentManager->SyncTransform(actorPtr->GetRootComponent());
    }
    else if (parentPtr != nullptr)
    {
        parentPtr->AddChild(objPtr);
        SceneFusion::ComponentManager->SyncTransform(actorPtr->GetRootComponent());
    }
}

void sfActorManager::OnLabelChanged(AActor* actorPtr)
{
    if (actorPtr == nullptr || actorPtr->GetOutermost() == GetTransientPackage())
    {
        return;
    }
    
    sfObject::SPtr objPtr = sfObjectMap::GetSFObject(actorPtr);
    if (objPtr == nullptr)
    {
        return;
    }
    SyncLabelAndName(actorPtr, objPtr, objPtr->Property()->AsDict());
}

void sfActorManager::RegisterPropertyChangeHandlers()
{
    m_propertyChangeHandlers[sfProp::Name] = 
        [this](UObject* uobjPtr, sfProperty::SPtr propertyPtr)
    {
        AActor* actorPtr = Cast<AActor>(uobjPtr);
        sfUtils::TryRename(actorPtr, sfPropertyUtil::ToString(propertyPtr));
        return true;
    };
    m_propertyChangeHandlers[sfProp::Label] =
        [this](UObject* uobjPtr, sfProperty::SPtr propertyPtr)
    {
        AActor* actorPtr = Cast<AActor>(uobjPtr);
        FCoreDelegates::OnActorLabelChanged.Remove(m_onLabelChangeHandle);
        actorPtr->SetActorLabel(sfPropertyUtil::ToString(propertyPtr));
        m_onLabelChangeHandle = FCoreDelegates::OnActorLabelChanged.AddRaw(this, &sfActorManager::OnLabelChanged);
        return true;
    };
    m_propertyChangeHandlers[sfProp::Folder] = 
        [this](UObject* uobjPtr, sfProperty::SPtr propertyPtr)
    {
        AActor* actorPtr = Cast<AActor>(uobjPtr);
        m_foldersToCheck.AddUnique(actorPtr->GetFolderPath().ToString());
        GEngine->OnLevelActorFolderChanged().Remove(m_onFolderChangeHandle);
        actorPtr->SetFolderPath(FName(*sfPropertyUtil::ToString(propertyPtr)));
        m_onFolderChangeHandle = GEngine->OnLevelActorFolderChanged().AddRaw(this, &sfActorManager::OnFolderChange);
        return true;
    };
}

void sfActorManager::InvokeOnLockStateChange(sfObject::SPtr objPtr, AActor* actorPtr)
{
    LockType lockType = Unlocked;
    if (objPtr->IsFullyLocked())
    {
        lockType = FullyLocked;
    }
    else if (objPtr->IsPartiallyLocked())
    {
        lockType = PartiallyLocked;
    }
    OnLockStateChange.ExecuteIfBound(actorPtr, lockType, objPtr->LockOwner());
}

void sfActorManager::ClearActorCollections()
{
    m_uploadList.Empty();
    m_movedActors.Empty();
    m_revertFolderQueue.Empty();
    m_syncParentList.Empty();
}

void sfActorManager::OnRemoveLevel(sfObject::SPtr levelObjPtr, ULevel* levelPtr)
{
    if (levelObjPtr != nullptr)
    {
        levelObjPtr->ForEachDescendant([this](sfObject::SPtr objPtr)
        {
            UObject* uobjPtr = sfObjectMap::Remove(objPtr);
            AActor* actorPtr = Cast<AActor>(uobjPtr);
            if (actorPtr != nullptr)
            {
                m_numSyncedActors--;
                m_selectedActors.erase(actorPtr);
                m_movedActors.Remove(actorPtr);
            }
            return true;
        });
    }

    for (int i = m_uploadList.Num() - 1; i >= 0; i--)
    {
        if (m_uploadList[i]->GetLevel() == levelPtr)
        {
            m_uploadList.RemoveAt(i);
        }
    }
}

void sfActorManager::OnSFLevelObjectCreate(sfObject::SPtr sfLevelObjPtr, ULevel* levelPtr)
{
    for (sfObject::SPtr childPtr : sfLevelObjPtr->Children())
    {
        if (childPtr->Type() == sfType::Actor)
        {
            OnCreate(childPtr, 0); // Child index does not matter
        }
    }
    DestroyUnsyncedActorsInLevel(levelPtr);
}

int sfActorManager::NumSyncedActors()
{
    return m_numSyncedActors;
}

bool sfActorManager::DetachIfParentIsLevel(sfObject::SPtr objPtr, AActor* actorPtr)
{
    if (objPtr->Parent()->Type() == sfType::Level)
    {
        DisableParentChangeHandler();
        actorPtr->DetachFromActor(FDetachmentTransformRules::KeepRelativeTransform);
        EnableParentChangeHandler();
        return true;
    }
    return false;
}

void sfActorManager::LogNoParentErrorAndDisconnect(sfObject::SPtr objPtr)
{
    sfDictionaryProperty::SPtr propertiesPtr = objPtr->Property()->AsDict();
    KS::Log::Error("Disconnecting because no parent object was found for actor " +
        propertiesPtr->Get(sfProp::Name)->ToString() +
        ". Root actor's parent object should be the level object.");
    SceneFusion::Service->LeaveSession();
}

#undef BSP_REBUILD_DELAY
#undef LOG_CHANNEL