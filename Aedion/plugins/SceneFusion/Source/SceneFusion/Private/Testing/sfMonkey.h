#pragma once

#include "sfBaseActivity.h"

#include <CoreMinimal.h>

/**
 * Randomly performs configurable activities.
 */
class sfMonkey
{
public:
    /**
     * Constructor.
     */
    sfMonkey();

    /**
     * Destructor
     */
    ~sfMonkey();

    /**
     * Is the monkey running?
     */
    bool IsRunning();

    /**
     * Restores the monkey to its default configuration.
     */
    void UseDefaults();

    /**
     * Sets all activity weights to 0 and clears all activity configuration.
     */
    void Reset();

    /**
     * Gets the activity with the given name.
     *
     * @param   const FString& name of activity to get. Case insensitive.
     * @return  TSharedPtr<sfBaseActivity> activity with the given name, or invalid pointer if none exists.
     */
    TSharedPtr<sfBaseActivity> GetActivity(const FString& name);

    /**
     * Starts randomly adding assets to the level. Does nothing if the monkey does not have a valid path with assets.
     */
    void Start();

    /**
     * Stops randomly adding assets to the level.
     */
    void Stop();

private:
    bool m_isRunning;
    TArray<TSharedPtr<sfBaseActivity>> m_activities;
    TSharedPtr<sfBaseActivity> m_currentActivity;
    FDelegateHandle m_tickHandle;

    /**
     * Adds a random asset to the level.
     *
     * @param   float deltaTime since the last tick.
     * @return  bool true to keep the tick function registered.
     */
    bool Tick(float deltaTime);
};