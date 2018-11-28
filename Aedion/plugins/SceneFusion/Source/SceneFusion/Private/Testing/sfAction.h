#pragma once

#include <CoreMinimal.h>
#include <functional>

/**
 * Contains mapping from action name to action.
 */
class sfAction
{
public:
    typedef std::function<void(const TArray<FString>&)> Action;

    /**
     * Constructor.
     */
    sfAction();

    /**
     * Destructor
     */
    ~sfAction();

    /**
     * Registers an action.
     *
     * @param   FString action name
     * @param   Action action
     * @return  bool - true if the action is successfully added
     */
    bool Register(FString actionName, Action action);

    /**
     * Unregisters an action.
     *
     * @param   FString action name
     * @return  bool - true if the action is successfully removed
     */
    bool Unregister(FString actionName);

    /**
     * Returns action with the given name. Returns nullptr if the action is not found.
     *
     * @param   FString action name
     * @return  Action
     */
    Action Get(FString actionName);

private:
    TMap<FString, Action> m_actions;
};