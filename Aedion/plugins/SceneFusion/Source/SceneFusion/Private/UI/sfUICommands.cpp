#include "sfUICommands.h"
#include "sfUIStyles.h"
#include "../SceneFusion.h"

#define LOCTEXT_NAMESPACE "SceneFusion"

sfUICommands::sfUICommands() :
    TCommands<sfUICommands>(
        TEXT("SceneFusion"), 
        NSLOCTEXT("Contexts", "Scene Fusion", "Scene Fusion Plugin"), 
        NAME_None, 
        sfUIStyles::GetStyleSetName()
    )
{

}

void sfUICommands::RegisterCommands()
{
    UI_COMMAND(ToolBarClickPtr, "Scene Fusion", "Open the Scene Fusion tab", 
        EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE