#include "sfMissingSceneComponent.h"
#include "../sfUtils.h"
#include "../sfActorUtil.h"
#include "../sfObjectMap.h"
#include "../Consts.h"
#include "../SceneFusion.h"
#include <Developer/HotReload/Public/IHotReload.h>

FString& UsfMissingSceneComponent::MissingClass()
{
    return ClassName;
}

void UsfMissingSceneComponent::Reload()
{
    sfObject::SPtr objPtr = sfObjectMap::Remove(this);
    if (objPtr == nullptr)
    {
        return;
    }
    // Rename this component so the replacement can use its name
    sfUtils::Rename(this, GetName() + " (deleted)");
    // Create a new component of the correct class for this sfObject and destroy this component
    SceneFusion::ComponentManager->OnCreate(objPtr, 0);
    if (IsSelected())
    {
        // Unselect this component and select the replacement component
        GEditor->SelectComponent(this, false, true);
        UActorComponent* componentPtr = sfObjectMap::Get<UActorComponent>(objPtr);
        if (componentPtr != nullptr)
        {
            GEditor->SelectComponent(componentPtr, true, true);
        }
    }
    AActor* ownerPtr = GetOwner();
    DestroyComponent();
    sfActorUtil::Reselect(ownerPtr);
}

void UsfMissingSceneComponent::BeginDestroy()
{
    if (SceneFusion::MissingObjectManager.IsValid())
    {
        SceneFusion::MissingObjectManager->RemoveStandIn(this);
    }
    Super::BeginDestroy();
}
