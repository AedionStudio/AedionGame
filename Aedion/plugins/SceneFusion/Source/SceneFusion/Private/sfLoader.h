#pragma once

#include "sfIStandInGenerator.h"
#include <sfProperty.h>
#include <sfObject.h>
#include <unordered_map>
#include <vector>
#include <functional>
#include <CoreMinimal.h>
#include <IInputProcessor.h>
#include <SlateApplication.h>
#include <Engine/AssetManager.h>

using namespace KS;
using namespace KS::SceneFusion2;

/**
 * Utility for loading assets from memory, or from disc when the user is idle. Loading from disc may trigger assets to
 * be baked which can block the main thread (and cannot be done from another thread) for several seconds and interrupt
 * the user, so we wait till the user is idle.
 */
class sfLoader : public IInputProcessor
{
public:
    /**
     * @return  sfLoader& singleton instance.
     */
    static sfLoader& Get();

    /**
     * Constructor
     */
    sfLoader();

    /**
     * Starts monitoring user activity and loads assets when the user becomes idle.
     */
    void Start();

    /**
     * Stops monitoring user activity and stops loading.
     */
    void Stop();

    /**
     * Registers a generator to generate data for stand-ins of a given class.
     *
     * @param   UClass* classPtr to register generator for.
     * @param   TSharedPtr<sfIStandInGenerator> generatorPtr to register.
     */
    void RegisterStandInGenerator(UClass* classPtr, TSharedPtr<sfIStandInGenerator> generatorPtr);

    /**
     * Checks if the user is idle.
     *
     * @return  bool true if the user is idle.
     */
    bool IsUserIdle();

    /**
     * Loads the asset for a property when the user becomes idle.
     *
     * @param   sfProperty::SPtr propPtr to load asset for.
     */
    void LoadWhenIdle(sfProperty::SPtr propPtr);

    /**
     * Loads delayed assets referenced by an object or its component children.
     *
     * @param   sfObject::SPtr objPtr to load assets for.
     */
    void LoadAssetsFor(sfObject::SPtr objPtr);

    /**
     * Loads an asset. If the asset could not be found, creates a stand-in to represent the asset.
     *
     * @param   const FString& path of asset to load.
     * @param   const FString& className of asset to load.
     * @return  UObject* asset or stand-in for the asset. nullptr if the asset was not found and a stand-in could not
     *          be created.
     */
    UObject* Load(const FString& path, const FString& className);

    /**
     * Gets the class name and path of the asset a stand-in is representing.
     *
     * @param   UObject* standInPtr
     * @return  FString class name and path seperated by a ';'.
     */
    FString GetPathFromStandIn(UObject* standInPtr);

    /**
     * Loads an asset from memory. Returns null if the asset was not found in memory.
     *
     * @param   FString path to the asset.
     * @return  UObject* loaded asset or nullptr.
     */
    UObject* LoadFromCache(FString path);

    /**
     * Called every tick while the sfLoader is running. Loads assets if the user is idle and sets references to them,
     * and replaces stand-ins with their proper assets if they are available.
     *
     * @param   const float deltaTime
     * @param   FSlateApplication& slateApp
     * @param   TSharedRef<ICursor> cursor
     */
    virtual void Tick(const float deltaTime, FSlateApplication& slateApp, TSharedRef<ICursor> cursor) override;

    /**
     * Called when a mouse button is pressed.
     *
     * @param   FSlateApplication& slateApp
     * @param   const FPointerEvent& mouseEvent
     * @return  bool false to indicate the event was not handled.
     */
    virtual bool HandleMouseButtonDownEvent(FSlateApplication& slateApp, const FPointerEvent& mouseEvent) override;

    /**
     * Called when a mouse button is released.
     *
     * @param   FSlateApplication& slateApp
     * @param   const FPointerEvent& mouseEvent
     * @return  bool false to indicate the event was not handled.
     */
    virtual bool HandleMouseButtonUpEvent(FSlateApplication& slateApp, const FPointerEvent& mouseEvent) override;

private:
    static TSharedPtr<sfLoader> m_instancePtr;

    // Maps objects to a list of their properties that referenced assets to be loaded when the user is idle.
    std::unordered_map<sfObject::SPtr, std::vector<sfProperty::SPtr>> m_delayedAssets;
    // Maps classes to the path to their stand-in asset. If a class is not in the map, a stand-in is created using
    // NewObject.
    TMap<UClass*, FString> m_standInPaths;
    TMap<UClass*, TSharedPtr<sfIStandInGenerator>> m_standInGenerators;
    // Maps missing asset paths to their stand-ins.
    TMap<FString, UObject*> m_standIns;
    TArray<UObject*> m_standInsToReplace;
    float m_replaceTimer;
    bool m_isMouseDown;
    bool m_overrideIdle;// if true, IsUserIdle() returns true even if the user isn't idle
    FDelegateHandle m_onNewAssetHandle;

    /**
     * Replaces references to stand-ins with the proper assets.
     */
    void ReplaceStandIns();

    /**
     * Loads delayed assets and sets references to them.
     */
    void LoadDelayedAssets();

    /**
     * Loads the asset for a property and sets the reference to it.
     *
     * @param   sfProperty::SPtr propPtr
     */
    void LoadProperty(sfProperty::SPtr propPtr);

    /**
     * Called when a new asset is created. If the asset has a stand-in, adds the stand-in to a list to replaced with
     * the new asset.
     *
     * @param   const FAssetData& assetData
     */
    void OnNewAsset(const FAssetData& assetData);
};