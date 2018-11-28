#include "sfMoveActivity.h"
#include "../SceneFusion.h"

#include <Runtime/Engine/Classes/Engine/Brush.h>

sfMoveActivity::sfMoveActivity(const FString& name, float weight)
    : sfBaseActivity{ name, weight }
{}

void sfMoveActivity::Start()
{
    RandomActors(m_actors);
    for (AActor* actorPtr : m_actors)
    {
        GEditor->SelectActor(actorPtr, true, true);
    }
    m_direction = FMath::VRand();
}

void sfMoveActivity::Tick(float deltaTime)
{
    FVector delta = m_direction * 200 * deltaTime;
    for (AActor* actorPtr : m_actors)
    {
        actorPtr->SetActorLocation(actorPtr->GetActorLocation() + delta);
        SceneFusion::ComponentManager->SyncTransform(actorPtr->GetRootComponent());
    }
}

void sfMoveActivity::Finish()
{
    bool rebuildBsp = false;
    for (AActor* actorPtr : m_actors)
    {
        GEditor->SelectActor(actorPtr, false, true);
        if (actorPtr->IsA<ABrush>())
        {
            rebuildBsp = true;
            ABrush::SetNeedRebuild(actorPtr->GetLevel());
        }
    }
    if (rebuildBsp)
    {
        GEditor->RebuildAlteredBSP();
    }
}