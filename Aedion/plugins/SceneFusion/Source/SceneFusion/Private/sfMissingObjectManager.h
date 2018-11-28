#pragma once

#include "sfMissingObject.h"
#include <Runtime/Engine/Classes/Engine/AssetManager.h>

/**
 * Maps missing class/blueprint names to stand-in objects and replaces the stand-ins with the correct class/blueprint
 * when they become available.
 */
class sfMissingObjectManager
{
public:
    /**
     * Initialization. Adds event handlers that listen for new assets.
     */
    void Initialize();

    /**
     * Clean up. Removes event handlers and clears the stand-in map.
     */
    void CleanUp();

    /**
     * Adds a stand-in to the stand-in map.
     *
     * @param   IsfMissingObject* standInPtr to add.
     */
    void AddStandIn(IsfMissingObject* standInPtr);

    /**
     * Removes a stand-in from the stand-in map.
     *
     * @param   IsfMissingObject* standInPtr to remove.
     */
    void RemoveStandIn(IsfMissingObject* standInPtr);

private:
    // Maps missing class/blueprint names/paths to stand-in objects.
    TMap<FString, TArray<IsfMissingObject*>> m_standInMap;
    FDelegateHandle m_onHotReloadHandle;
    FDelegateHandle m_onNewAssetHandle;

    /**
     * Called after game code is compiled. Checks if missing classes are now available and replaces stand-ins with
     * instances of the newly available classes.
     *
     * @param   bool automatic -  true if the recompile was triggered automatically.
     */
    void OnHotReload(bool automatic);

    /**
     * Called when a new asset is created. If the asset is a blueprint, replaces any stand-ins for that blueprint with
     * instances of the blueprint.
     *
     * @param   const FAssetData& assetData for the new asset.
     */
    void OnNewAsset(const FAssetData& assetData);
};