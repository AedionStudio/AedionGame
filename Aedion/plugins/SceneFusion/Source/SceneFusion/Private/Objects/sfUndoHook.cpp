#include "sfUndoHook.h"
#include <Log.h>
#include <Editor.h>
#include <Components/ModelComponent.h>

void UsfUndoHook::PostEditUndo()
{
    // Remove unregistered model components from the levels we are rebuilding BSP for to avoid log spam when Unreal
    // tries to unregister them.
    TArray<TWeakObjectPtr<ULevel>> levelsToRebuild;
    ABrush::NeedsRebuild(&levelsToRebuild);
    for (TWeakObjectPtr<ULevel> levelPtr : levelsToRebuild)
    {
        for (auto iter = levelPtr->ModelComponents.CreateIterator(); iter; ++iter)
        {
            if (!(*iter)->IsRegistered())
            {
                iter.RemoveCurrent();
            }
        }
    }
    GIsTransacting = false;
    GEditor->RebuildAlteredBSP();
    GIsTransacting = true;
}