#include "sfComponentManager.h"
#include "../sfObjectMap.h"
#include "../sfPropertyUtil.h"
#include "../Consts.h"
#include "../SceneFusion.h"
#include "../Components/sfLockComponent.h"
#include "../Components/sfMissingComponent.h"
#include "../Components/sfMissingSceneComponent.h"
#include "../Actors/sfMissingActor.h"
#include "../sfActorUtil.h"
#include "../sfUtils.h"
#include <sfDictionaryProperty.h>
#include <Editor.h>

#define LOG_CHANNEL "sfComponentManager"
#define DEFAULT_FLAGS (RF_Transactional | RF_DefaultSubObject | RF_WasLoaded | RF_LoadCompleted)

using namespace KS::SceneFusion2;

sfComponentManager::sfComponentManager()
{
    RegisterPropertyChangeHandlers();
}

sfComponentManager::~sfComponentManager()
{

}

void sfComponentManager::Initialize()
{
    m_sessionPtr = SceneFusion::Service->Session();
    m_onApplyObjectToActorHandle
        = FEditorDelegates::OnApplyObjectToActor.AddRaw(this, &sfComponentManager::OnApplyObjectToActor);
}

void sfComponentManager::CleanUp()
{
    FEditorDelegates::OnApplyObjectToActor.Remove(m_onApplyObjectToActorHandle);
}

bool sfComponentManager::IsSyncable(UActorComponent* componentPtr)
{
    if (componentPtr == nullptr || componentPtr->IsPendingKill() || componentPtr->HasAnyFlags(RF_Transient))
    {
        return false;
    }

    // Skip landscape components for now since we are not supporting landscape yet.
    FName className = componentPtr->GetClass()->GetFName();
    if (className == "LandscapeComponent" || className == "LandscapeHeightfieldCollisionComponent")
    {
        return false;
    }

    return true;
}

void sfComponentManager::SyncComponents(AActor* actorPtr, sfObject::SPtr actorObjPtr)
{
    if (actorObjPtr->IsLocked())
    {
        RestoreDeletedComponents(actorObjPtr);
    }
    TSet<UActorComponent*> components = actorPtr->GetComponents();
    for (UActorComponent* componentPtr : components)
    {
        if (!IsSyncable(componentPtr))
        {
            continue;
        }
        sfObject::SPtr objPtr = sfObjectMap::GetSFObject(componentPtr);
        // Check if the component is new
        if (objPtr == nullptr || !objPtr->IsSyncing())
        {
            if (actorObjPtr->IsLocked())
            {
                if (actorPtr->GetRootComponent() == componentPtr)
                {
                    actorPtr->SetRootComponent(nullptr);
                }
                componentPtr->DestroyComponent();
                sfActorUtil::Reselect(actorPtr);
            }
            else
            {
                Upload(componentPtr);
            }
            continue;
        }

        // Check for a parent change
        SyncParent(actorPtr, componentPtr, objPtr);

        // Check for a name change
        sfDictionaryProperty::SPtr propertiesPtr = objPtr->Property()->AsDict();
        FString name = sfPropertyUtil::ToString(propertiesPtr->Get(sfProp::Name));
        if (componentPtr->GetName() != name)
        {
            if (objPtr->IsLocked())
            {
                sfUtils::TryRename(componentPtr, name);
                sfActorUtil::Reselect(componentPtr->GetOwner());
            }
            else
            {
                propertiesPtr->Set(sfProp::Name, sfPropertyUtil::FromString(componentPtr->GetName()));
            }
        }
    }
    if (!actorObjPtr->IsLocked())
    {
        FindDeletedComponents(actorObjPtr);
    }
}

void sfComponentManager::RestoreDeletedComponents(sfObject::SPtr objPtr)
{
    for (sfObject::SPtr childPtr : objPtr->Children())
    {
        if (childPtr->Type() != sfType::Component)
        {
            continue;
        }
        UActorComponent* componentPtr = sfObjectMap::Get<UActorComponent>(childPtr);
        if (componentPtr != nullptr && componentPtr->IsPendingKill())
        {
            sfObjectMap::Remove(componentPtr);
            OnCreate(childPtr, 0);
        }
        RestoreDeletedComponents(childPtr);
    }
}

void sfComponentManager::FindDeletedComponents(sfObject::SPtr objPtr)
{
    for (int i = objPtr->Children().size() - 1; i >= 0; i--)
    {
        sfObject::SPtr childPtr = objPtr->Child(i);
        if (childPtr->Type() != sfType::Component)
        {
            continue;
        }
        UActorComponent* componentPtr = sfObjectMap::Get<UActorComponent>(childPtr);
        if (componentPtr == nullptr || componentPtr->IsPendingKill() || componentPtr->GetOwner() == nullptr)
        {
            if (componentPtr != nullptr && !componentPtr->IsPendingKill())
            {
                // Unreal has a bug where if you duplicate a component of a blueprint and undo, the component isn't
                // destroyed properly but has it's owner set to nullptr, so we destroy it.
                componentPtr->DestroyComponent();
            }
            sfObjectMap::Remove(componentPtr);
            // Component children are already reparented, but actor children still need to be reparented.
            for (int j = childPtr->Children().size() - 1; j >= 0; j--)
            {
                sfObject::SPtr grandChildPtr = childPtr->Child(j);
                AActor* actorPtr = sfObjectMap::Get<AActor>(grandChildPtr);
                if (actorPtr != nullptr && actorPtr->GetRootComponent() != nullptr)
                {
                    sfObject::SPtr parentPtr = sfObjectMap::GetSFObject(
                        actorPtr->GetRootComponent()->GetAttachParent());
                    if (parentPtr == nullptr)
                    {
                        parentPtr = SceneFusion::LevelManager->GetLevelObject(actorPtr->GetLevel());
                    }
                    if (parentPtr != nullptr)
                    {
                        parentPtr->AddChild(grandChildPtr);
                        SyncTransform(actorPtr->GetRootComponent());
                    }
                }
            }
            m_sessionPtr->Delete(childPtr);
        }
        FindDeletedComponents(childPtr);
    }
}

void sfComponentManager::SyncParent(AActor* actorPtr, UActorComponent* componentPtr, sfObject::SPtr objPtr)
{
    if (actorPtr == nullptr || objPtr == nullptr)
    {
        return;
    }
    USceneComponent* sceneComponentPtr = Cast<USceneComponent>(componentPtr);
    if (sceneComponentPtr == nullptr)
    {
        return;
    }
    UObject* uparentPtr = sceneComponentPtr == actorPtr->GetRootComponent() ?
        actorPtr : (UObject*)sceneComponentPtr->GetAttachParent();
    if (uparentPtr == nullptr)
    {
        uparentPtr = actorPtr;
    }
    sfObject::SPtr parentPtr = sfObjectMap::GetSFObject(uparentPtr);
    if (parentPtr == nullptr || !parentPtr->IsSyncing())
    {
        return;
    }
    if (objPtr->Parent() != parentPtr)
    {
        if (objPtr->IsLocked())
        {
            OnParentChange(objPtr, 0);
        }
        else
        {
            if (actorPtr->GetRootComponent() == componentPtr)
            {
                objPtr->Property()->AsDict()->Set(sfProp::IsRoot, sfValueProperty::Create(true));
                // Unreal has a bug where if you change the root component of a child actor, the actor is no longer
                // attached to the parent but it appears to be in the World Outliner, so we attach it back.
                USceneComponent* parentCompPtr = sfObjectMap::Get<USceneComponent>(parentPtr->Parent());
                if (parentCompPtr != nullptr)
                {
                    SceneFusion::ActorManager->DisableParentChangeHandler();
                    sceneComponentPtr->AttachToComponent(parentCompPtr, FAttachmentTransformRules::KeepWorldTransform);
                    SceneFusion::ActorManager->EnableParentChangeHandler();
                }
            }
            else if (objPtr->Parent()->Type() == sfType::Actor)
            {
                // This component is not longer the root
                objPtr->Property()->AsDict()->Remove(sfProp::IsRoot);
            }
            sfObject::SPtr currentPtr = parentPtr;
            while (parentPtr->IsDescendantOf(objPtr) && currentPtr != objPtr)
            {
                // Adding the child now creates a cyclic loop, so we sync the parent for parentPtr and its ancestors
                // until the loop is broken.
                UActorComponent* childPtr = sfObjectMap::Get<UActorComponent>(currentPtr);
                if (childPtr != nullptr)
                {
                    SyncParent(actorPtr, childPtr, currentPtr);
                }
                currentPtr = currentPtr->Parent();
            }
            parentPtr->AddChild(objPtr);
        }
        SyncTransform(sceneComponentPtr);
    }
    else
    {
        sfProperty::SPtr propPtr;
        bool wasRoot = parentPtr->Type() == sfType::Actor &&
            objPtr->Property()->AsDict()->TryGet(sfProp::IsRoot, propPtr) && (bool)propPtr->AsValue()->GetValue();
        if (wasRoot != (componentPtr == actorPtr->GetRootComponent()))
        {
            if (wasRoot)
            {
                // This component is not longer the root
                objPtr->Property()->AsDict()->Remove(sfProp::IsRoot);
            }
            else
            {
                // This component became the root
                objPtr->Property()->AsDict()->Set(sfProp::IsRoot, sfValueProperty::Create(true));
            }
        }
    }
}

void sfComponentManager::Upload(UActorComponent* componentPtr)
{
    AActor* actorPtr = componentPtr->GetOwner();
    if (actorPtr == nullptr)
    {
        return;
    }

    UObject* uparentPtr = actorPtr;
    if (componentPtr != actorPtr->GetRootComponent())
    {
        USceneComponent* sceneComponentPtr = Cast<USceneComponent>(componentPtr);
        if (sceneComponentPtr != nullptr && sceneComponentPtr->GetAttachParent() != nullptr)
        {
            uparentPtr = sceneComponentPtr->GetAttachParent();
        }
    }
    sfObject::SPtr parentPtr = sfObjectMap::GetSFObject(uparentPtr);
    if (parentPtr == nullptr)
    {
        return;
    }

    sfObject::SPtr objPtr = CreateObject(componentPtr);
    if (objPtr == nullptr)
    {
        return;
    }
    if (componentPtr == actorPtr->GetRootComponent())
    {
        objPtr->Property()->AsDict()->Set(sfProp::IsRoot, sfValueProperty::Create(true));
        // Unreal has a bug where if you change the root component of a child actor, the actor is no longer
        // attached to the parent but it appears to be in the World Outliner, so we attach it back.
        USceneComponent* parentCompPtr = sfObjectMap::Get<USceneComponent>(parentPtr->Parent());
        if (parentCompPtr != nullptr)
        {
            USceneComponent* sceneComponentPtr = Cast<USceneComponent>(componentPtr);
            SceneFusion::ActorManager->DisableParentChangeHandler();
            sceneComponentPtr->AttachToComponent(parentCompPtr, FAttachmentTransformRules::KeepWorldTransform);
            SceneFusion::ActorManager->EnableParentChangeHandler();
            SyncTransform(sceneComponentPtr);
        }
    }
    m_sessionPtr->Create(objPtr, parentPtr, 0);
    // Pre-existing child objects can only be attached after calling Create.
    FindAndAttachChildren(objPtr);
}

void sfComponentManager::FindAndAttachChildren(sfObject::SPtr objPtr)
{
    auto iter = objPtr->SelfAndDescendants();
    while (iter.Value() != nullptr)
    {
        sfObject::SPtr currentPtr = iter.Value();
        iter.Next();
        USceneComponent* componentPtr = sfObjectMap::Get<USceneComponent>(currentPtr);
        if (componentPtr != nullptr)
        {
            for (USceneComponent* childPtr : componentPtr->GetAttachChildren())
            {
                sfObject::SPtr childObjPtr = sfObjectMap::GetSFObject(childPtr);
                if (childObjPtr == nullptr)
                {
                    continue;
                }
                if (childObjPtr->Parent() != nullptr && childObjPtr->Parent()->Type() == sfType::Actor)
                {
                    childObjPtr = childObjPtr->Parent();
                }
                if (childObjPtr->Parent() != currentPtr)
                {
                    currentPtr->AddChild(childObjPtr);
                    SyncTransform(childPtr);

                    sfDictionaryProperty::SPtr propertiesPtr = childObjPtr->Property()->AsDict();
                    sfProperty::SPtr propPtr;
                    if (propertiesPtr->TryGet(sfProp::IsRoot, propPtr) && (bool)propPtr->AsValue()->GetValue())
                    {
                        propPtr->AsValue()->SetValue(false);
                    }
                }
            }
        }
    }
}

sfObject::SPtr sfComponentManager::CreateObject(UActorComponent* componentPtr)
{
    sfObject::SPtr objPtr = sfObjectMap::GetOrCreateSFObject(componentPtr, sfType::Component);
    if (objPtr != nullptr && objPtr->IsSyncing())
    {
        return nullptr;
    }
    sfDictionaryProperty::SPtr propertiesPtr = objPtr->Property()->AsDict();

    FString className;
    IsfMissingObject* missingComponentPtr = Cast<IsfMissingObject>(componentPtr);
    if (missingComponentPtr != nullptr)
    {
        // This is a stand-in for a missing component class.
        SceneFusion::MissingObjectManager->AddStandIn(missingComponentPtr);
        className = missingComponentPtr->MissingClass();
    }
    else
    {
        className = sfUtils::ClassToFString(componentPtr->GetClass());
    }

    propertiesPtr->Set(sfProp::Name, sfPropertyUtil::FromString(componentPtr->GetName()));
    propertiesPtr->Set(sfProp::Class, sfPropertyUtil::FromString(className));

    EComponentCreationMethod creationMethod = componentPtr->CreationMethod;
    // Check if the component should use creation method SimpleConstructionScript but we couldn't set it to that
    // doing so on a non-blueprint stand-in would delete the component.
    AsfMissingActor* missingActorPtr = Cast<AsfMissingActor>(componentPtr->GetOwner());
    if (missingActorPtr != nullptr && missingActorPtr->SimpleConstructionComponents.Contains(componentPtr))
    {
        creationMethod = EComponentCreationMethod::SimpleConstructionScript;
    }
    propertiesPtr->Set(sfProp::CreationMethod, sfValueProperty::Create((uint8_t)creationMethod));

    EObjectFlags flags = componentPtr->GetFlags();
    if (flags != DEFAULT_FLAGS)
    {
        propertiesPtr->Set(sfProp::Flags, sfValueProperty::Create((uint32_t)flags));
    }

    sfPropertyUtil::CreateProperties(componentPtr, propertiesPtr);

    USceneComponent* sceneComponentPtr = Cast<USceneComponent>(componentPtr);
    if (sceneComponentPtr == nullptr)
    {
        return objPtr;
    }
    if (sceneComponentPtr->bVisualizeComponent)
    {
        propertiesPtr->Set(sfProp::Visualize, sfValueProperty::Create(true));
    }
    for (USceneComponent* childComponentPtr : sceneComponentPtr->GetAttachChildren())
    {
        if (!IsSyncable(childComponentPtr))
        {
            continue;
        }
        sfObject::SPtr childPtr;
        if (childComponentPtr->GetOuter() == componentPtr->GetOuter())
        {
            // Child is part of the same actor
            childPtr = CreateObject(childComponentPtr);
        }
        else
        {
            // Child is the root component of a different actor
            AActor* actorPtr = childComponentPtr->GetOwner();
            // Actor may be pending kill even when its component isn't because of undo
            if (actorPtr != nullptr && !actorPtr->IsPendingKill())
            {
                childPtr = SceneFusion::ActorManager->CreateObject(actorPtr);
            }
        }
        if (childPtr != nullptr)
        {
            objPtr->AddChild(childPtr);
        }
    }
    return objPtr;
}

void sfComponentManager::OnCreate(sfObject::SPtr objPtr, int childIndex)
{
    sfObject::SPtr actorObjPtr = objPtr->Parent();
    while (actorObjPtr != nullptr && actorObjPtr->Type() != sfType::Actor)
    {
        actorObjPtr = actorObjPtr->Parent();
    }
    if (actorObjPtr == nullptr)
    {
        KS::Log::Warning("Component object cannot be created without an actor ancestor.", LOG_CHANNEL);
        return;
    }
    AActor* actorPtr = sfObjectMap::Get<AActor>(actorObjPtr);
    if (actorPtr == nullptr)
    {
        return;
    }
    USceneComponent* componentPtr = Cast<USceneComponent>(InitializeComponent(actorPtr, objPtr));
    if (componentPtr == nullptr)
    {
        sfActorUtil::Reselect(actorPtr);
        return;
    }
    sfObject::SPtr parentObjPtr = objPtr->Parent();
    if (parentObjPtr == actorObjPtr)
    {
        sfProperty::SPtr propPtr;
        if (objPtr->Property()->AsDict()->TryGet(sfProp::IsRoot, propPtr) && (bool)propPtr->AsValue()->GetValue())
        {
            actorPtr->SetRootComponent(componentPtr);
            parentObjPtr = parentObjPtr->Parent();
        }
        else
        {
            parentObjPtr = nullptr;
            if (actorPtr->GetRootComponent() == componentPtr)
            {
                actorPtr->SetRootComponent(nullptr);
            }
        }
    }

    USceneComponent* parentPtr = sfObjectMap::Get<USceneComponent>(parentObjPtr);
    if (parentPtr != nullptr)
    {
        SceneFusion::ActorManager->DisableParentChangeHandler();
        componentPtr->AttachToComponent(parentPtr, FAttachmentTransformRules::KeepRelativeTransform);
        SceneFusion::ActorManager->EnableParentChangeHandler();
    }
    sfActorUtil::Reselect(actorPtr);
    SceneFusion::RedrawActiveViewport();
}

UActorComponent* sfComponentManager::InitializeComponent(AActor* actorPtr, sfObject::SPtr objPtr)
{
    sfDictionaryProperty::SPtr propertiesPtr = objPtr->Property()->AsDict();
    FString className = sfPropertyUtil::ToString(propertiesPtr->Get(sfProp::Class));
    UClass* classPtr = sfUtils::LoadClass(className);
    FString name = sfPropertyUtil::ToString(propertiesPtr->Get(sfProp::Name));
    UActorComponent* componentPtr =
        Cast<UActorComponent>(StaticFindObjectFast(UActorComponent::StaticClass(), actorPtr, FName(*name)));
    bool isRegistered = false;
    if (componentPtr != nullptr)
    {
        if (componentPtr->IsPendingKill())
        {
            // Rename the other component so we can reuse this name
            sfUtils::Rename(componentPtr, componentPtr->GetName() + " (deleted)");
            componentPtr = nullptr;
        }
        else if (sfObjectMap::Contains(componentPtr) || (classPtr != nullptr && classPtr != componentPtr->GetClass()))
        {
            // Rename the other component so we can reuse this name
            sfUtils::Rename(componentPtr, componentPtr->GetName() + "_");
            componentPtr = nullptr;
        }
    }

    EObjectFlags flags = DEFAULT_FLAGS;
    sfProperty::SPtr propPtr;
    if (propertiesPtr->TryGet(sfProp::Flags, propPtr))
    {
        flags = (EObjectFlags)(uint32_t)propPtr->AsValue()->GetValue();
    }

    FVector location, scale;
    FRotator rotation;

    if (componentPtr == nullptr)
    {
        bool isMissingClass = classPtr == nullptr;
        if (isMissingClass)
        {
            if ((objPtr->Parent() != nullptr && objPtr->Parent()->Type() == sfType::Component) ||
                (propertiesPtr->TryGet(sfProp::IsRoot, propPtr) && (bool)propPtr->AsValue()->GetValue()))
            {
                classPtr = UsfMissingSceneComponent::StaticClass();
            }
            else
            {
                classPtr = UsfMissingComponent::StaticClass();
            }
        }
        componentPtr = NewObject<UActorComponent>(actorPtr, classPtr, FName(*name), flags);
        if (isMissingClass)
        {
            IsfMissingObject* missingComponentPtr = Cast<IsfMissingObject>(componentPtr);
            missingComponentPtr->MissingClass() = className;
            SceneFusion::MissingObjectManager->AddStandIn(missingComponentPtr);
        }
    }
    else
    {
        isRegistered = true;
        componentPtr->ClearFlags(RF_AllFlags);
        componentPtr->SetFlags(flags);
    }

    USceneComponent* sceneComponentPtr = Cast<USceneComponent>(componentPtr);
    if (sceneComponentPtr != nullptr)
    {
        location = sceneComponentPtr->RelativeLocation;
        rotation = sceneComponentPtr->RelativeRotation;
        scale = sceneComponentPtr->RelativeScale3D;
    }

    sfObjectMap::Add(objPtr, componentPtr);
    EComponentCreationMethod creationMethod = (EComponentCreationMethod)propertiesPtr->Get(sfProp::CreationMethod)
        ->AsValue()->GetValue().GetByte();
    if (creationMethod != EComponentCreationMethod::SimpleConstructionScript || !actorPtr->IsA<AsfMissingActor>())
    {
        componentPtr->CreationMethod = creationMethod;
    }
    else
    {
        // Setting the creation method to SimpleConstructionScript on a non-blueprint stand-in will delete the
        // component, so we store the components that should have that creation method and instead assign them the
        // default creation method.
        Cast<AsfMissingActor>(actorPtr)->SimpleConstructionComponents.Add(componentPtr);
    }
    sfPropertyUtil::ApplyProperties(componentPtr, propertiesPtr);
    SceneFusion::RedrawActiveViewport();

    // Set references to this component
    std::vector<sfReferenceProperty::SPtr> references = m_sessionPtr->GetReferences(objPtr);
    sfPropertyUtil::SetReferences(componentPtr, references);

    if (sceneComponentPtr != nullptr)
    {
        sceneComponentPtr->bVisualizeComponent = propertiesPtr->TryGet(sfProp::Visualize, propPtr) &&
            (bool)propPtr->AsValue()->GetValue();
        if (sceneComponentPtr->RelativeLocation != location || sceneComponentPtr->RelativeRotation != rotation ||
            sceneComponentPtr->RelativeScale3D != scale)
        {
            actorPtr->InvalidateLightingCache();
        }
    }

    if (!isRegistered)
    {
        componentPtr->RegisterComponent();
        componentPtr->InitializeComponent();
    }

    if (sceneComponentPtr == nullptr)
    {
        if (objPtr->Children().size() > 0)
        {
            KS::Log::Warning(std::string(TCHAR_TO_UTF8(*componentPtr->GetClass()->GetName())) +
                " has children but it is not a scene component. The children will be ignored.", LOG_CHANNEL);
        }
        return componentPtr;
    }

    // Detach from parent to avoid possible loops when we try to attach its children
    SceneFusion::ActorManager->DisableParentChangeHandler();
    sceneComponentPtr->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
    SceneFusion::ActorManager->EnableParentChangeHandler();
    // Initialize children
    for (sfObject::SPtr childPtr : objPtr->Children())
    {
        if (childPtr->Type() == sfType::Component)
        {
            USceneComponent* childComponentPtr = sfObjectMap::Get<USceneComponent>(childPtr);
            if (childComponentPtr != nullptr)
            {
                SyncTransform(childComponentPtr, true);
            }
            else
            {
                childComponentPtr = Cast<USceneComponent>(InitializeComponent(actorPtr, childPtr));
            }
            if (childComponentPtr != nullptr)
            {
                childComponentPtr->AttachToComponent(sceneComponentPtr, FAttachmentTransformRules::KeepRelativeTransform);
            }
        }
        else if (childPtr->Type() == sfType::Actor)
        {
            AActor* childActorPtr = sfObjectMap::Get<AActor>(childPtr);
            if (childActorPtr != nullptr)
            {
                SyncTransform(childActorPtr->GetRootComponent(), true);
            }
            else
            {
                childActorPtr = SceneFusion::ActorManager->InitializeActor(childPtr, actorPtr->GetLevel());
            }
            if (childActorPtr != nullptr)
            {
                SceneFusion::ActorManager->DisableParentChangeHandler();
                childActorPtr->AttachToComponent(sceneComponentPtr, FAttachmentTransformRules::KeepRelativeTransform);
                SceneFusion::ActorManager->EnableParentChangeHandler();
            }
        }
    }

    // If the component is a mesh or the root component and is locked, add a lock component.
    if (objPtr->IsLocked())
    {
        bool isMesh = sceneComponentPtr->IsA<UMeshComponent>();
        if (isMesh || (propertiesPtr->TryGet(sfProp::IsRoot, propPtr) && (bool)propPtr->AsValue()->GetValue() &&
            objPtr->Parent() != nullptr && objPtr->Parent()->Type() == sfType::Actor))
        {
            UMaterialInterface* lockMaterialPtr = SceneFusion::GetLockMaterial(objPtr->LockOwner());
            if (lockMaterialPtr != nullptr)
            {
                UsfLockComponent* lockPtr = NewObject<UsfLockComponent>(actorPtr,
                    *FString("SFLock" + sceneComponentPtr->GetName()));
                lockPtr->SetMobility(sceneComponentPtr->Mobility);
                lockPtr->AttachToComponent(sceneComponentPtr, FAttachmentTransformRules::KeepRelativeTransform);
                lockPtr->RegisterComponent();
                lockPtr->InitializeComponent();
                if (isMesh)
                {
                    lockPtr->DuplicateParentMesh(lockMaterialPtr);
                }
            }
        }
    }

    return componentPtr;
}

void sfComponentManager::OnDelete(sfObject::SPtr objPtr)
{
    UActorComponent* componentPtr = Cast<UActorComponent>(sfObjectMap::Remove(objPtr));
    if (componentPtr != nullptr)
    {
        AActor* actorPtr = componentPtr->GetOwner();
        componentPtr->DestroyComponent();
        SceneFusion::ActorManager->CleanUpChildrenOfDeletedObject(objPtr, nullptr, true);
        SceneFusion::RedrawActiveViewport();
        sfActorUtil::Reselect(actorPtr);
    }
}

void sfComponentManager::RegisterPropertyChangeHandlers()
{
    m_propertyChangeHandlers[sfProp::Name] = [this](UObject* uobjPtr, sfProperty::SPtr propertyPtr)
    {
        UActorComponent* componentPtr = Cast<UActorComponent>(uobjPtr);
        sfUtils::TryRename(componentPtr, sfPropertyUtil::ToString(propertyPtr));
        sfActorUtil::Reselect(componentPtr->GetOwner());
        return true;
    };
    m_propertyChangeHandlers[sfProp::IsRoot] = [this](UObject* uobjPtr, sfProperty::SPtr propertyPtr)
    {
        sfObject::SPtr objPtr = sfObjectMap::GetSFObject(uobjPtr);
        if (objPtr == nullptr)
        {
            return true;
        }
        sfObject::SPtr parentPtr = objPtr->Parent();
        if (parentPtr != nullptr && parentPtr->Type() == sfType::Actor)
        {
            OnParentChange(objPtr, 0);
        }
        return true;
    };
    m_propertyChangeHandlers[sfProp::Location] =
        [this](UObject* uobjPtr, sfProperty::SPtr propertyPtr)
    {
        UActorComponent* componentPtr = Cast<UActorComponent>(uobjPtr);
        AActor* actorPtr = componentPtr->GetOwner();
        actorPtr->InvalidateLightingCache();
        if (actorPtr->IsA<ABrush>())
        {
            SceneFusion::ActorManager->MarkBSPStale(actorPtr);
        }
        return false;
    };
    m_propertyChangeHandlers[sfProp::Rotation] =
        [this](UObject* uobjPtr, sfProperty::SPtr propertyPtr)
    {
        UActorComponent* componentPtr = Cast<UActorComponent>(uobjPtr);
        AActor* actorPtr = componentPtr->GetOwner();
        actorPtr->InvalidateLightingCache();
        if (actorPtr->IsA<ABrush>())
        {
            SceneFusion::ActorManager->MarkBSPStale(actorPtr);
        }
        return false;
    };
    m_propertyChangeHandlers[sfProp::Scale] =
        [this](UObject* uobjPtr, sfProperty::SPtr propertyPtr)
    {
        UActorComponent* componentPtr = Cast<UActorComponent>(uobjPtr);
        AActor* actorPtr = componentPtr->GetOwner();
        actorPtr->InvalidateLightingCache();
        if (actorPtr->IsA<ABrush>())
        {
            SceneFusion::ActorManager->MarkBSPStale(actorPtr);
        }
        return false;
    };
}

bool sfComponentManager::OnUPropertyChange(sfObject::SPtr objPtr, UObject* uobjPtr, UProperty* upropPtr)
{
    UMeshComponent* meshPtr = Cast<UMeshComponent>(uobjPtr);
    if (meshPtr != nullptr && upropPtr->GetName().Contains("mesh"))
    {
        sfPropertyUtil::SyncProperty(objPtr, uobjPtr, "OverrideMaterials");
    }
    else if (upropPtr->GetName() == "bAbsoluteLocation")
    {
        sfPropertyUtil::SyncProperty(objPtr, uobjPtr, FName(sfProp::Location->c_str()));
    }
    return false;
}

void sfComponentManager::OnApplyObjectToActor(UObject* uobjPtr, AActor* actorPtr)
{
    sfObject::SPtr objPtr = sfObjectMap::GetSFObject(actorPtr);
    if (objPtr == nullptr || !objPtr->IsSyncing())
    {
        return;
    }
    
    TSet<UActorComponent*> components = actorPtr->GetComponents();
    for (UActorComponent* componentPtr : components)
    {
        if (!IsSyncable(componentPtr))
        {
            continue;
        }
        sfObject::SPtr objPtr = sfObjectMap::GetSFObject(componentPtr);
        // Check if the component is new
        if (objPtr == nullptr || !objPtr->IsSyncing())
        {
            continue;
        }
        // Check for material change
        sfPropertyUtil::SyncProperty(objPtr, componentPtr, "OverrideMaterials");
    }
}

void sfComponentManager::SyncTransform(USceneComponent* componentPtr, bool applyServerValues)
{
    sfObject::SPtr objPtr = sfObjectMap::GetSFObject(componentPtr);
    if (objPtr == nullptr)
    {
        return;
    }
    sfPropertyUtil::SyncProperty(objPtr, componentPtr, FName(sfProp::Location->c_str()), applyServerValues);
    sfPropertyUtil::SyncProperty(objPtr, componentPtr, FName(sfProp::Rotation->c_str()), applyServerValues);
    sfPropertyUtil::SyncProperty(objPtr, componentPtr, FName(sfProp::Scale->c_str()), applyServerValues);
}

void sfComponentManager::OnPropertyChange(sfProperty::SPtr propertyPtr)
{
    USceneComponent* sceneComponent = sfObjectMap::Get<USceneComponent>(propertyPtr->GetContainerObject());
    if (sceneComponent != nullptr)
    {
        SceneFusion::RedrawActiveViewport();
    }
    sfBaseUObjectManager::OnPropertyChange(propertyPtr);
}

void sfComponentManager::OnRemoveField(sfDictionaryProperty::SPtr dictPtr, const sfName& name)
{
    USceneComponent* sceneComponent = sfObjectMap::Get<USceneComponent>(dictPtr->GetContainerObject());
    if (sceneComponent != nullptr)
    {
        SceneFusion::RedrawActiveViewport();
    }
    sfBaseUObjectManager::OnRemoveField(dictPtr, name);
}

void sfComponentManager::OnParentChange(sfObject::SPtr objPtr, int childIndex)
{
    if (objPtr->Parent() == nullptr)
    {
        KS::Log::Warning("Component became a root object. Components should always have a component or actor parent.",
            LOG_CHANNEL);
        return;
    }
    USceneComponent* componentPtr = sfObjectMap::Get<USceneComponent>(objPtr);
    if (componentPtr == nullptr)
    {
        return;
    }
    USceneComponent* parentPtr;
    if (objPtr->Parent()->Type() == sfType::Actor)
    {
        sfProperty::SPtr propPtr;
        if (objPtr->Property()->AsDict()->TryGet(sfProp::IsRoot, propPtr) && (bool)propPtr->AsValue()->GetValue())
        {
            parentPtr = sfObjectMap::Get<USceneComponent>(objPtr->Parent()->Parent());
            AActor* actorPtr = sfObjectMap::Get<AActor>(objPtr->Parent());
            actorPtr->SetRootComponent(componentPtr);
        }
        else
        {
            parentPtr = nullptr;
        }
    }
    else
    {
        parentPtr = sfObjectMap::Get<USceneComponent>(objPtr->Parent());
        AActor* actorPtr = componentPtr->GetOwner();
        if (actorPtr != nullptr && actorPtr->GetRootComponent() == componentPtr)
        {
            actorPtr->SetRootComponent(nullptr);
        }
    }
    if (parentPtr != nullptr)
    {
        if (parentPtr->IsPendingKill())
        {
            OnCreate(sfObjectMap::GetSFObject(parentPtr), 0);
            return;
        }
        SceneFusion::ActorManager->DisableParentChangeHandler();
        componentPtr->AttachToComponent(parentPtr, FAttachmentTransformRules::KeepRelativeTransform);
        SceneFusion::ActorManager->EnableParentChangeHandler();
        sfActorUtil::Reselect(componentPtr->GetOwner());
    }
    else
    {
        SceneFusion::ActorManager->DisableParentChangeHandler();
        componentPtr->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
        SceneFusion::ActorManager->EnableParentChangeHandler();
    }
}

bool sfComponentManager::OnUndoRedo(sfObject::SPtr objPtr, UObject* uobjPtr)
{
    UActorComponent* componentPtr = Cast<UActorComponent>(uobjPtr);
    if (componentPtr == nullptr)
    {
        return false;
    }
    if (componentPtr->IsPendingKill())
    {
        return true;
    }
    if (objPtr != nullptr)
    {
        sfDictionaryProperty::SPtr propertiesPtr = objPtr->Property()->AsDict();
        if (objPtr->IsLocked())
        {
            sfPropertyUtil::ApplyProperties(componentPtr, propertiesPtr);
        }
        else
        {
            sfPropertyUtil::SendPropertyChanges(componentPtr, propertiesPtr);
        }
        
        AActor* actorPtr = componentPtr->GetOwner();
        if (actorPtr != nullptr && componentPtr == actorPtr->GetRootComponent())
        {
            SceneFusion::ActorManager->SyncParent(actorPtr, sfObjectMap::GetSFObject(actorPtr));
        }

        componentPtr->MarkRenderStateDirty();
    }
    else if (!componentPtr->IsRenderStateCreated())
    {
        // This component was deleted by another user and is in a bad state. Delete it.
        componentPtr->DestroyComponent();
    }
    return true;
}

#undef DEFAULT_FLAGS
#undef LOG_CHANNEL