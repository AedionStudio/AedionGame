#pragma once

#include "sfBaseActivity.h"

/**
 * Activity that deletes random actors from the level.
 */
class sfDeleteActivity : public sfBaseActivity
{
public:
    /**
     * Constructor
     *
     * @param   const FString& name of activity.
     * @param   float weight the determines how likely this activity is to occur.
     */
    sfDeleteActivity(const FString& name, float weight);

    /**
     * Deletes random actors from the level.
     */
    virtual void Start() override;
};