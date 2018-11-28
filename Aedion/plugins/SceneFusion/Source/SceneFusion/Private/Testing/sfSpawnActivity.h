#pragma once

#include "sfBaseActivity.h"

/**
 * Activity for randomly placing assets into the level. Currently only static meshes, skeletal meshes, and particle
 * systems are supported.
 */
class sfSpawnActivity : public sfBaseActivity
{
public:
    /**
     * Constructor
     *
     * @param   const FString& name of activity.
     * @param   float weight the determines how likely this activity is to occur.
     */
    sfSpawnActivity(const FString& name, float weight);

    /**
     * Handles command arguments. The arguments are either paths relative to /Game/ that determine where we look for
     * assets, or any of the following options:
     *     -r | -reset: clears all paths used to look for assets.
     *
     * @param   const TArray<FString>& args
     * @param   int index of first arg in array to process.
     */
    virtual void HandleArgs(const TArray<FString>& args, int index) override;

    /**
     * Adds a random asset to the level.
     */
    virtual void Start() override;

private:
    TArray<FString> m_assetPaths;
};