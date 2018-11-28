#pragma once

#include "CoreMinimal.h"
#include "Components/SpotLightComponent.h"
#include "sfFlashlightComponent.generated.h"

/**
 * Flashlight component that has no icon shown in the viewport.
 */
UCLASS()
class UsfFlashlightComponent : public USpotLightComponent
{
	GENERATED_BODY()
	
public:
    /**
     * Constructor
     */
    UsfFlashlightComponent()
    {
        //Hide icon
#if WITH_EDITORONLY_DATA
        DynamicEditorTextureScale = 0.0f;
#endif
    }
};
