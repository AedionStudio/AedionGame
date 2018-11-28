#pragma once

#include <CoreMinimal.h>
#include <Engine/Level.h>
#include <Engine/LevelStreaming.h>
#include <sfSession.h>
#include <unordered_map>
#include <unordered_set>

#include "sfBaseUObjectManager.h"

using namespace KS::SceneFusion2;
using namespace KS;

/**
 * Manages level syncing. Level relationship is not maintained.
 */
class sfLevelManager : public sfBaseUObjectManager
{
public:
    friend class sfUndoManager;

    /**
     * Constructor
     */
    sfLevelManager();

    /**
     * Destructor
     */
    virtual ~sfLevelManager();

    /**
     * Initialization. Called after connecting to a session.
     */
    virtual void Initialize();

    /**
     * Deinitialization. Called after disconnecting from a session.
     */
    virtual void CleanUp();

    /**
     * Updates the level manager.
     */
    void Tick();

    /**
     * Called when an sfObject of type level, level lock or world settings is created.
     *
     * @param   sfObject::SPtr objPtr that was created.
     * @param   int childIndex of new object. -1 if object is a root
     */
    virtual void OnCreate(sfObject::SPtr objPtr, int childIndex) override;

    /**
     * Called when a level is deleted by another user. Unloads the level.
     *
     * @param   sfObject::SPtr objPtr that was deleted.
     */
    virtual void OnDelete(sfObject::SPtr objPtr) override;

    /**
     * Called when one or more elements are added to a list property.
     *
     * @param   sfListProperty::SPtr listPtr that elements were added to.
     * @param   int index elements were inserted at.
     * @param   int count - number of elements added.
     */
    virtual void OnListAdd(sfListProperty::SPtr listPtr, int index, int count) override;

    /**
     * Gets the uobject for an sfObject, or nullptr if the sfObject has no uobject.
     *
     * @param   sfObject::SPtr objPtr to get uobject for.
     * @return  UObject* uobject for the sfObject.
     */
    virtual UObject* GetUObject(sfObject::SPtr objPtr) override;

    /**
     * Gets sfObject by the given ULevel.
     *
     * @param   ULevel* levelPtr
     * @return  sfObject::SPtr
     */
    sfObject::SPtr GetLevelObject(ULevel* levelPtr);

    /**
     * Gets ULevel by the given sfObject. If could not find the ULevel, returns nullptr.
     *
     * @param   sfObject::SPtr levelObjPtr
     * @return  ULevel*
     */
    ULevel* FindLevelByObject(sfObject::SPtr levelObjPtr);

    /**
     * Returns true if we can find sfObject for the given level and the level object has received its children from
     * the server.
     *
     * @param   ULevel* levelPtr - pointer of level to check
     * @return  bool
     */
    bool IsLevelObjectInitialized(ULevel* levelPtr);

private:
    typedef std::function<void()> Callback;

    bool m_initialized;
    sfSession::SPtr m_sessionPtr;
    bool m_uploadUnsyncedLevels;
    UWorld* m_worldPtr;
    sfObject::SPtr m_worldSettingsObjPtr; // sfObject for the world setting properties
    sfObject::SPtr m_lockObject; // Reqeust lock on this object before upload levels to the server
    bool m_worldSettingsDirty;
    bool m_hierarchicalLODSetupDirty;

    // ULevel <--> sfObject maps
    TMap<ULevel*, sfObject::SPtr> m_levelToObjectMap;
    std::unordered_map<sfObject::SPtr, ULevel*> m_objectToLevelMap;

    // Level object <--> level property object maps
    std::unordered_map<sfObject::SPtr, sfObject::SPtr> m_objectToProperty;
    std::unordered_map<sfObject::SPtr, sfObject::SPtr> m_propertyToObject;

    std::unordered_set<ULevel*> m_movedLevels; // Levels whose offset have changed
    TSet<ULevelStreaming*> m_dirtyStreamingLevels; // Dirty streaming levels to check folder change
    std::unordered_set<sfObject::SPtr> m_levelsNeedToBeLoaded; // Levels that was deleted but locked
    std::unordered_set<ULevel*> m_levelsToUpload; // Levels to upload to server
    TMap<FString, sfObject::SPtr> m_unloadedLevelObjects; // sfObjects of unloaded levels
    // sfObjects of levels that requested server for children
    std::unordered_set<sfObject::SPtr> m_levelsWaitingForChildren;
    // Levels whose package was dirty. We need to check parent change on them
    TSet<ULevel*> m_dirtyParentLevels;
    // Levels that are just added to the world without applying server properties.
    TSet<ULevel*> m_uninitializedLevels;

    // List of properties we want sfPropertyUtil class to ignore because they are handled in sfLevelManager
    const TSet<FString> PROPERTY_BLACKLIST{ "LevelTransform" };
    const TSet<FString> WORLD_SETTINGS_BLACKLIST{ "bEnableWorldComposition" };

    UClass* m_worldTileDetailsClassPtr;
    UProperty* m_packageNamePropertyPtr;


    FDelegateHandle m_onAddLevelToWorldHandle;
    FDelegateHandle m_onPrepareToCleanseEditorObjectHandle;
    FDelegateHandle m_onObjectModifiedHandle;
    FDelegateHandle m_onWorldCompositionChangeHandle;
    FDelegateHandle m_onPackageMarkedDirtyHandle;
    FDelegateHandle m_onPropertyChangeHandle;
    sfSession::AcknowledgeSubscriptionEventHandle m_onAcknowledgeSubscriptionHandle;
    TMap<ULevel*, FDelegateHandle> m_onLevelTransformChangeHandles;

    /**
     * Tries to find level in all loaded levels. If found, returns level pointer. Otherwise, returns nullptr.
     *
     * @param   FString levelPath
     * @param   bool isPersistentLevel
     * @return  ULevel*
     */
    ULevel* FindLevelInLoadedLevels(FString levelPath, bool isPersistentLevel);

    /**
     * Tries to load level from file and return level pointer.
     *
     * @param   FString levelPath
     * @param   bool isPersistentLevel
     * @return  ULevel*
     */
    ULevel* TryLoadLevelFromFile(FString levelPath, bool isPersistentLevel);

    /**
     * Creates map file for level and returns level pointer.
     *
     * @param   FString levelPath
     * @param   bool isPersistentLevel
     * @return  ULevel*
     */
    ULevel* CreateMap(FString levelPath, bool isPersistentLevel);

    /**
     * Called when a level is added to the world. Uploads the new level.
     *
     * @param   ULevel* newLevelPtr
     * @param   UWorld* worldPtr
     */
    void OnAddLevelToWorld(ULevel* newLevelPtr, UWorld* worldPtr);

    /**
     * Called when the editor is about to cleanse an object that must be purged,
     * such as when changing the active map or level. If the object is a world object, disconnect.
     * If it is a level object, delete the sfObject on the server. Clears raw pointers of actors in
     * the level from our containers.
     *
     * @param   UObject* uobjPtr - object to be purged
     */
    void OnPrepareToCleanseEditorObject(UObject* uobjPtr);

    /**
     * Called when an object is modified. Sends streaming level changes to server.
     *
     * @param   UObject* uobjPtr - modified object
     */
    void OnObjectModified(UObject* uobjPtr);

    /**
     * Uploads levels that don't exist on the server.
     */
    void UploadUnsyncedLevels();

    /**
     * Registers property change handlers for server events.
     */
    void RegisterPropertyChangeHandlers();

    /**
     * Checks for and sends transform changes for a level to the server.
     *
     * @param   ULevel* levelPtr to send transform update for.
     */
    void SendTransformUpdate(ULevel* levelPtr);

    /**
     * Sends a new folder value to the server.
     *
     * @param   ULevelStreaming* streamingLevelPtr to send folder change for.
     */
    void SendFolderChange(ULevelStreaming* streamingLevelPtr);

    /**
     * Modifies a ULevel. Removes event handlers before and adds event handlers back after.
     * Prevents any changes to the undo stack during the call.
     *
     * @param   ULevel* levelPtr to modify
     * @param   Callback callback to modify the given level
     */
    void ModifyLevelWithoutTriggerEvent(ULevel* levelPtr, Callback callback);

    /**
     * Uploads the given level.
     *
     * @param   ULevel* levelPtr
     */
    void UploadLevel(ULevel* levelPtr);
    
    /**
     * Reuqests lock for uploading levels.
     */
    void RequestLock();

    /**
     * Regsiters handlers on level events.
     */
    void RegisterLevelEvents();

    /**
     * Unregsiters handlers on level events.
     */
    void UnregisterLevelEvents();

    /**
     * Called when the server acknowledges the subscription for the given object's children.
     *
     * @param   bool isSubscription - if true, this acknowledgement is for a subscription.
     *          Otherwise, it is for an unsubscription.
     * @param   sfObject::SPtr objPtr
     */
    void OnAcknowledgeSubscription(bool isSubscription, sfObject::SPtr objPtr);

    /**
     * Called when a level object is created. Subscribes to children of the object if the level is loaded.
     *
     * @param   sfObject::SPtr objPtr
     */
    void OnCreateLevelObject(sfObject::SPtr objPtr);

    /**
     * Called when a level lock object is created.
     *
     * @param   sfObject::SPtr objPtr
     */
    void OnCreateLevelLockObject(sfObject::SPtr objPtr);

    /**
     * Called when a world settings object is created.
     *
     * @param   sfObject::SPtr objPtr
     */
    void OnCreateWorldSettingsObject(sfObject::SPtr objPtr);

    /**
     * Called when a game mode object is created.
     *
     * @param   sfObject::SPtr objPtr
     */
    void OnCreateGameModeObject(sfObject::SPtr objPtr);

    /**
     * Tries to toggle world composition. If it failed, leaves the session.
     *
     * @param   bool enableWorldComposition - if ture enable world composition. Otherwise, disable world composition.
     */
    void TryToggleWorldComposition(bool enableWorldComposition);

    /**
     * Toggles world composition. Unloads all sublevels before and loads them back after.
     *
     * @param   bool enableWorldComposition - if ture enable world composition. Otherwise, disable world composition.
     */
    void ToggleWorldComposition(bool enableWorldComposition);

    /**
     * Sets the world composition property on the server.
     *
     * @param   UWorld* worldPtr
     */
    void SetWorldCompositionOnServer(UWorld* worldPtr);

    /**
     * Gets the world composition property on the server.
     *
     * @return  bool
     */
    bool GetWorldCompositionOnServer();

    /**
     * Called when UPackage is marked dirty. If the package contains a level, put the level into the dirty parent level
     * set to check the level parent change in the Tick function.
     *
     * @param   UPackage* packagePtr - pointer to the dirty package
     * @param   bool wasDirty - If true, the package was marked as dirty before.
     */
    void OnPackageMarkedDirty(UPackage* packagePtr, bool wasDirty);

    /**
     * Loads level from file if the map file exists. Otherwise, creates a level and save it to the given path.
     * Returns the loaded or created level.
     *
     * @param   const FString& levelPath
     * @param   bool isPersistentLevel - true if the given level is the persitent level
     * @return  ULevel* - loaded level
     */
    ULevel* LoadOrCreateMap(const FString& levelPath, bool isPersistentLevel);

    /**
     * Called when a property on a ULevelStreaming object changes.
     *
     * @param   sfObject::SPtr objPtr for the uobject whose property changed.
     * @param   UObject* uobjPtr - pointer to the ULevelStreaming object whose property changed.
     * @param   UProperty* upropPtr that changed.
     * @return  bool false if the property change event should be handled by the default handler.
     */
    virtual bool OnUPropertyChange(sfObject::SPtr objPtr, UObject* uobjPtr, UProperty* upropPtr) override;

    /**
     * Called when an object is modified by an undo or redo transaction.
     *
     * @param   sfObject::SPtr objPtr for the uobject that was modified. nullptr if the uobjPtr is not synced.
     * @param   UObject* uobjPtr that was modified.
     * @return  bool true if event was handled and need not be processed by other handlers.
     */
    virtual bool OnUndoRedo(sfObject::SPtr objPtr, UObject* uobjPtr) override;

    /**
     * Called when a property on a world tile details object changes.
     *
     * @param   UObject* uobjPtr - pointer to the ULevelStreaming object whose property changed.
     * @param   UProperty* upropPtr that changed.
     */
    void OnTileDetailsChange(UObject* uobjPtr, UProperty* upropPtr);

    /**
     * Returns UWorldTileDetails object for the given level.
     *
     * @param   FString levelPath - path of the level to find UWorldTileDetails object for.
     * @return  UObject* - the found object. Returns nullptr if it could not be found.
     */
    UObject* FindWorldTileDetailsObject(FString levelPath);

    /**
     * Called when a property is changed through the details panel. If the changed property is the level tile position
     * and the level object is locked, reverts the position back to server value. This needs to be done before Unreal
     * moves all the level actors so we cannot do it in the sfBaseObjectManager::OnUPropertyChange.
     *
     * @param   UObject* uobjPtr whose property changed.
     * @param   FPropertyChangedEvent& ev with information on what property changed.
     */
    void OnUPropertyChange(UObject* uobjPtr, FPropertyChangedEvent& ev);

    /**
     * Tries to find level object and properties for the given world tile details object.
     * Returns true if found. Otherwise, returns false.
     *
     * @param   UObject* worldTileDetailPtr
     * @param   sfObject::SPtr& levelObjPtr
     * @param   sfDictionaryProperty::SPtr& levelPropertiesPtr
     * @return  bool
     */
    bool TryGetLevelObjectAndPropertyForTileDetailObject(
        UObject* worldTileDetailPtr,
        sfObject::SPtr& levelObjPtr,
        sfDictionaryProperty::SPtr& levelPropertiesPtr);

    /**
     * Refreshes world settings tab.
     */
    void RefreshWorldSettingsTab();
};
