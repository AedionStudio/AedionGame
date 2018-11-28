#include "sfLockComponent.h"
#include "../SceneFusion.h"
#include <Editor.h>

UsfLockComponent::UsfLockComponent()
{
    m_copied = false;
    m_initialized = false;
    m_materialPtr = nullptr;
    ClearFlags(RF_Transactional);// Prevent component from being recorded in transactions
    SetFlags(RF_Transient);// Prevent component from being saved
}

UsfLockComponent::~UsfLockComponent()
{
    FTicker::GetCoreTicker().RemoveTicker(m_tickerHandle);
    
}

void UsfLockComponent::InitializeComponent()
{
    Super::InitializeComponent();
    // Prevents the component from saving and showing in the details panel
    bIsEditorOnly = true;
    AActor* actorPtr = GetOwner();
    if (actorPtr != nullptr)
    {
        actorPtr->bLockLocation = true;
        if (actorPtr->GetClass()->IsInBlueprint())
        {
            CreationMethod = EComponentCreationMethod::UserConstructionScript;
        }
    }
    FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(this, &UsfLockComponent::OnUPropertyChange);
    FEditorDelegates::PreSaveWorld.AddUObject(this, &UsfLockComponent::PreSave);
    FEditorDelegates::PostSaveWorld.AddUObject(this, &UsfLockComponent::PostSave);
}

void UsfLockComponent::DuplicateParentMesh(UMaterialInterface* materialPtr)
{
    m_initialized = true;
    if (materialPtr != nullptr)
    {
        m_materialPtr = materialPtr;
    }
    UMeshComponent* parentPtr = Cast<UMeshComponent>(GetAttachParent());
    if (parentPtr == nullptr)
    {
        return;
    }
    FString name = GetName();
    name.Append("Mesh");
    UMeshComponent* copyPtr = DuplicateObject(parentPtr, this, *name);
    if (copyPtr->IsPendingKill())
    {
        return;
    }
    copyPtr->CreationMethod = CreationMethod;
    copyPtr->bIsEditorOnly = true;
    copyPtr->SetRelativeLocation(FVector::ZeroVector);
    copyPtr->SetRelativeRotation(FQuat::Identity);
    copyPtr->SetRelativeScale3D(FVector::OneVector);
    for (int i = 0; i < copyPtr->GetNumMaterials(); i++)
    {
        copyPtr->SetMaterial(i, m_materialPtr);
    }
    copyPtr->SetMobility(Mobility);
    copyPtr->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
    copyPtr->RegisterComponent();
    copyPtr->InitializeComponent();
    copyPtr->ClearFlags(RF_Transactional);// Prevent component from being recorded in transactions
    copyPtr->SetFlags(RF_Transient);// Prevent component from being saved
}

void UsfLockComponent::SetMaterial(UMaterialInterface* materialPtr)
{
    m_materialPtr = materialPtr;
    for (USceneComponent* childPtr : GetAttachChildren())
    {
        UMeshComponent* meshPtr = Cast<UMeshComponent>(childPtr);
        if (meshPtr != nullptr)
        {
            for (int i = 0; i < meshPtr->GetNumMaterials(); i++)
            {
                meshPtr->SetMaterial(i, materialPtr);
            }
        }
    }
}

void UsfLockComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    AActor* actorPtr = GetOwner();
    if (actorPtr != nullptr)
    {
        actorPtr->bLockLocation = false;
    }
    if (GetNumChildrenComponents() > 0 && !bDestroyingHierarchy)
    {
        for (int i = GetNumChildrenComponents() - 1; i >= 0; i--)
        {
            USceneComponent* childPtr = GetChildComponent(i);
            if (childPtr != nullptr)
            {
                childPtr->DestroyComponent();
            }
        }
    }
    Super::OnComponentDestroyed(bDestroyingHierarchy);
}

// When a component is detroyed, the children are attached to the root. When a lock component's parent changes, we
// assume the parent was destroyed and we destroy the lock component as well, unless this is the only lock component
// on the actor, in which case we destroy the children of this lock component.
void UsfLockComponent::OnAttachmentChanged()
{
    if (m_initialized && bRegistered)
    {
        AActor* actorPtr = GetOwner();
        if (actorPtr != nullptr && actorPtr->GetRootComponent() != nullptr)
        {
            TArray<UsfLockComponent*> locks;
            actorPtr->GetComponents(locks);
            if (locks.Num() == 1)
            {
                // Destroy children
                for (int i = GetNumChildrenComponents() - 1; i >= 0; i--)
                {
                    USceneComponent* childPtr = GetChildComponent(i);
                    if (childPtr != nullptr)
                    {
                        childPtr->DestroyComponent();
                    }
                }
                return;
            }
        }
        // We want to destroy this component and its child, but if we do it now we'll get a null reference in Unreal's
        // code that runs after this function, so we wait a tick.
        FTicker::GetCoreTicker().RemoveTicker(m_tickerHandle);
        m_tickerHandle = FTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateLambda([this, actorPtr](float deltaTime)
        {
            DestroyComponent();
            SceneFusion::RedrawActiveViewport();
            if (actorPtr != nullptr && actorPtr->GetRootComponent() != nullptr)
            {
                actorPtr->bLockLocation = true;
            }
            return false;
        }), 1.0f / 60.0f); 
    }
}

void UsfLockComponent::PostEditImport()
{
    // This is called twice when the object is duplicated, so we check if it was already called
    if (m_copied)
    {
        return;
    }
    m_copied = true;
    // We want to destroy this component and its child, but we have to wait a tick for the child to be created
    FTicker::GetCoreTicker().RemoveTicker(m_tickerHandle);
    m_tickerHandle = FTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateLambda([this](float deltaTime)
    {
        DestroyComponent();
        return false;
    }), 1.0f / 60.0f);
}

void UsfLockComponent::PreSave(uint32_t saveFlags, UWorld* worldPtr)
{
    AActor* actorPtr = GetOwner();
    if (actorPtr != nullptr)
    {
        actorPtr->bLockLocation = false;
    }
}

void UsfLockComponent::PostSave(uint32_t saveFlags, UWorld* worldPtr, bool success)
{
    AActor* actorPtr = GetOwner();
    if (actorPtr != nullptr)
    {
        actorPtr->bLockLocation = true;
    }
}

void UsfLockComponent::OnUPropertyChange(UObject* uobjPtr, FPropertyChangedEvent& ev)
{
    if (uobjPtr != GetAttachParent() || ev.MemberProperty == nullptr)
    {
        return;
    }
    if (ev.MemberProperty->GetFName() == "OverrideMaterials")
    {
        SetMaterial(m_materialPtr);
    }
    else if (ev.MemberProperty->GetName().Contains("mesh"))
    {
        // Destroy child mesh and create a new copy of the parent mesh
        for (int i = GetNumChildrenComponents() - 1; i >= 0; i--)
        {
            USceneComponent* childPtr = GetChildComponent(i);
            if (childPtr != nullptr && childPtr->GetClass() == uobjPtr->GetClass())
            {
                if (ev.MemberProperty->Identical_InContainer(childPtr, uobjPtr))
                {
                    // The mesh is the same as the lock mesh. Do nothing.
                    return;
                }
                childPtr->DestroyComponent();
            }
        }
        DuplicateParentMesh(m_materialPtr);
    }
    SceneFusion::RedrawActiveViewport();
}
