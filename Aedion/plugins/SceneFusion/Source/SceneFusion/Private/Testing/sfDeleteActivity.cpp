#include "sfDeleteActivity.h"
#include "../SceneFusion.h"

#include <Runtime/Engine/Classes/Engine/Brush.h>

sfDeleteActivity::sfDeleteActivity(const FString& name, float weight)
    : sfBaseActivity{ name, weight }
{}

void sfDeleteActivity::Start()
{
    AActor* actorPtr = RandomActor();
    if (actorPtr != nullptr)
    {
        GEditor->GetEditorWorldContext().World()->EditorDestroyActor(actorPtr, true);
        if (actorPtr->IsA<ABrush>())
        {
            SceneFusion::RedrawActiveViewport();
            GEditor->RebuildAlteredBSP();
        }
    }
}