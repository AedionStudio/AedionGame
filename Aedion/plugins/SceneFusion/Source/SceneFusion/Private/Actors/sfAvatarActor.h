#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include <Materials/MaterialInstanceDynamic.h>
#include "sfAvatarActor.generated.h"

/**
 * Unselectable static meshPtr actor used for avatars.
 */
UCLASS()
class AsfAvatarActor : public AStaticMeshActor
{
    GENERATED_BODY()

public:
    /**
     * Creates an avatar actor.
     *
     * @param   const FVector& location
     * @param   const FRotator& rotation
     * @param   UStaticMesh* meshPtr
     * @param   UMaterialInstanceDynamic* materialPtr
     * @return  AsfAvatarActor*
     */
    static AsfAvatarActor* Create(
        const FVector& location,
        const FRotator& rotation,
        UStaticMesh* meshPtr,
        UMaterialInstanceDynamic* materialPtr);

    /**
     * Overrides IsSelectable to always return false.
     *
     * @return  bool
     */
    virtual bool IsSelectable() const override;

    /**
     * Moves the actor instantly to the specified location.
     *
     * @param   const FVector& newLocation - The new location to teleport the actor to.
     */
    virtual void SetLocation(const FVector& newLocation);

    /**
     * Sets the actor's rotation instantly to the specified rotation.
     *
     * @param   const FQuat& newRotation - the new rotation for the actor.
     */
    virtual void SetRotation(const FQuat& newRotation);

    /**
     * Sets the actor's scale instantly to the specified scale.
     *
     * @param   const FVector& newScale - the new scale for the actor.
     */
    virtual void SetScale(const FVector& newScale);
};