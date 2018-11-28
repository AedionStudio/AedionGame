#pragma once

#include "sfBaseActivity.h"

/**
 * Activity that randomly renames actors.
 */
class sfRenameActivity : public sfBaseActivity
{
public:
    /**
     * Constructor
     *
     * @param   const FString& name of activity.
     * @param   float weight the determines how likely this activity is to occur.
     */
    sfRenameActivity(const FString& name, float weight);

    /**
     * Selects and random set of actors and gives them random labels.
     */
    virtual void Start() override;

    /**
     * Deselects the actors.
     */
    virtual void Finish() override;

private:
    TArray<AActor*> m_actors;
    TArray<FString> m_adjectives;
    TArray<FString> m_nouns;
};