#include "sfMissingActor.h"
#include "../sfUtils.h"
#include "../sfObjectMap.h"
#include "../Consts.h"
#include "../SceneFusion.h"
#include <Developer/HotReload/Public/IHotReload.h>

FString& AsfMissingActor::MissingClass()
{
    return ClassName;
}

void AsfMissingActor::Reload()
{
    sfObject::SPtr objPtr = sfObjectMap::Remove(this);
    if (objPtr == nullptr)
    {
        return;
    }
    // Remove child component objects from the sfObjectMap
    objPtr->ForEachDescendant([](sfObject::SPtr childPtr)
    {
        if (childPtr->Type() != sfType::Component)
        {
            return false;
        }
        sfObjectMap::Remove(childPtr);
        return true;
    });
    // Rename this actor so the replacement can use its name
    sfUtils::Rename(this, GetName() + " (deleted)");
    // Create a new actor of the correct class for this sfObject and destroy this actor
    SceneFusion::ActorManager->OnCreate(objPtr, 0);
    if (IsSelected())
    {
        // Unselect this actor and select the replacement actor
        GEditor->SelectActor(this, false, true);
        AActor* actorPtr = sfObjectMap::Get<AActor>(objPtr);
        if (actorPtr != nullptr)
        {
            GEditor->SelectActor(actorPtr, true, true);
        }
    }
    SceneFusion::ActorManager->DestroyActor(this);
}

void AsfMissingActor::BeginDestroy()
{
    if (SceneFusion::MissingObjectManager.IsValid())
    {
        SceneFusion::MissingObjectManager->RemoveStandIn(this);
    }
    Super::BeginDestroy();
}
