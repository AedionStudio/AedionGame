#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "sfUndoHook.generated.h"

/**
 * This is part of a hack to run code after an undo transaction but before the objects in the transaction have
 * PostEditUndo called.
 */
UCLASS()
class UsfUndoHook : public UObject
{
	GENERATED_BODY()
	
public:
    /**
     * Called after undoing a transaction that this object was hacked into. Rebuilds BSP.
     */
    virtual void PostEditUndo() override;
};
