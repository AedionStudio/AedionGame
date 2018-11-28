#pragma once

#include <CoreMinimal.h>
#include "sfAction.h"

/**
 * Executes actions at scheduled times.
 */
class sfTimer
{
public:
    /**
     * Constructor.
     */
    sfTimer();

    /**
     * Destructor
     */
    ~sfTimer();

    /**
     * Executes the given action at the given time.
     *
     * @param   sfAction::Action action to execute
     * @param   const TArray<FString>& args of action
     * @param   FDateTime date time to execute the action
     */
    FDelegateHandle StartTimer(sfAction::Action action, const TArray<FString>& args, FDateTime dateTime);

    /**
     * Stops excuting action of the given handle.
     *
     * @param   FDelegateHandle handle
     */
    void StopTimer(FDelegateHandle handle);

    /**
     * Stops all actions.
     */
    void StopAll();

private:
    TSet<FDelegateHandle> m_handles;
};