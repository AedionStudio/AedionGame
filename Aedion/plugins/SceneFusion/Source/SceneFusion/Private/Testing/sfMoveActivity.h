#pragma once

#include "sfBaseActivity.h"

/**
 * Activity that moves random actors in a random direction over time.
 */
class sfMoveActivity : public sfBaseActivity
{
public:
    /**
     * Constructor
     *
     * @param   const FString& name of activity.
     * @param   float weight the determines how likely this activity is to occur.
     */
    sfMoveActivity(const FString& name, float weight);

    /**
     * Selects a random set of actors.
     */
    virtual void Start() override;

    /**
     * Called every tick to update the location of the actors.
     *
     * @param   float deltaTime in seconds since the last tick.
     */
    virtual void Tick(float deltaTime) override;

    /**
     * Deselects the actors.
     */
    virtual void Finish() override;

private:
    TArray<AActor*> m_actors;
    FVector m_direction;
};