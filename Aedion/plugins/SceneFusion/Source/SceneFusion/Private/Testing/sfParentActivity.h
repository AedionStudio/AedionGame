#pragma once

#include "sfBaseActivity.h"

/**
 * Activity that randomly parents actors to other actors.
 */
class sfParentActivity : public sfBaseActivity
{
public:
    /**
     * Constructor
     *
     * @param   const FString& name of activity.
     * @param   float weight the determines how likely this activity is to occur.
     */
    sfParentActivity(const FString& name, float weight);

    /**
     * Changes a random actors parent to another random actor. If actor already has a parent, it might detach the actor
     * without attaching it to another. Selects the actor whose parent changed.
     */
    virtual void Start() override;

    /**
     * Deselects the actor whose parent changed.
     */
    virtual void Finish() override;

private:
    AActor* m_actorPtr;
};