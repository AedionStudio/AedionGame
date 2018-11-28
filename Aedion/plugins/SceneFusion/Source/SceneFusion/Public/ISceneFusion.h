#pragma once

#include <ModuleManager.h>

/**
 * Scene Fusion plugin module interface
 */
class ISceneFusion : public IModuleInterface
{
public:
    /**
     * Find the loaded Scene Fusion module
     *
     * @return  ISceneFusion&
     */
    static inline ISceneFusion& Get()
    {
        return FModuleManager::LoadModuleChecked<ISceneFusion>("SceneFusion");
    }

    /**
     * Check if the Scene Fusion module is loaded.
     *
     * @return  bool - true if the module is available
     */
    static inline bool IsAvailable()
    {
        return FModuleManager::Get().IsModuleLoaded("SceneFusion");
    }
};