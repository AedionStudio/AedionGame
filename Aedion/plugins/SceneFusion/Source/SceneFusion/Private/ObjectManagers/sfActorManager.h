#pragma once

#include <CoreMinimal.h>
#include <GameFramework/Actor.h>
#include <sfObject.h>
#include <sfSession.h>
#include <sfValueProperty.h>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <Editor/UnrealEd/Classes/Editor/TransBuffer.h>

#include "sfBaseUObjectManager.h"
#include "sfLevelManager.h"
#include "../sfUPropertyInstance.h"
#include "../Consts.h"

using namespace KS::SceneFusion2;
using namespace KS;

/**
 * Manages actor syncing.
 */
class sfActorManager : public sfBaseUObjectManager
{
public:
    friend class sfLevelManager;
    friend class sfComponentManager;
    friend class sfUndoManager;
    friend class AsfMissingActor;
    friend class SceneFusion;

    /**
     * Types of lock.
     */
    enum LockType
    {
        NotSynced,
        Unlocked,
        PartiallyLocked,
        FullyLocked
    };

    /**
     * Delegate for lock state change.
     *
     * @param   AActor* - pointer to the actor whose lock state changed
     * @param   LockType - lock type
     * @param   sfUser::SPtr - lock owner
     */
    DECLARE_DELEGATE_ThreeParams(OnLockStateChangeDelegate, AActor*, LockType, sfUser::SPtr);

    /**
     * Lock state change event handler.
     */
    OnLockStateChangeDelegate OnLockStateChange;

    /**
     * Constructor
     *
     * @param   TSharedPtr<sfLevelManager> levelManagerPtr
     */
    sfActorManager(TSharedPtr<sfLevelManager> levelManagerPtr);

    /**
     * Destructor
     */
    virtual ~sfActorManager();

    /**
     * Initialization. Called after connecting to a session.
     */
    virtual void Initialize() override;

    /**
     * Deinitialization. Called after disconnecting from a session.
     */
    virtual void CleanUp() override;

    /**
     * Updates the actor manager.
     *
     * @param   float deltaTime in seconds since last tick.
     */
    void Tick(float deltaTime);

    /**
     * Checks if an actor can be synced.
     *
     * @param   AActor* actorPtr
     * @return  bool true if the actor can be synced.
     */
    bool IsSyncable(AActor* actorPtr);

    /**
     * @return   int - number of synced actors.
     */
    int NumSyncedActors();

private:
    FDelegateHandle m_onActorAddedHandle;
    FDelegateHandle m_onActorDeletedHandle;
    FDelegateHandle m_onActorAttachedHandle;
    FDelegateHandle m_onActorDetachedHandle;
    FDelegateHandle m_onFolderChangeHandle;
    FDelegateHandle m_onLabelChangeHandle;
    FDelegateHandle m_onMoveStartHandle;
    FDelegateHandle m_onMoveEndHandle;
    FDelegateHandle m_onActorMovedHandle;

    TArray<AActor*> m_uploadList;
    TQueue<sfObject::SPtr> m_recreateQueue;
    TQueue<AActor*> m_revertFolderQueue;
    TArray<AActor*> m_syncParentList;
    TArray<FString> m_foldersToCheck;

    // Use std map because TSortedMap causes compile errors in Unreal's code
    std::map<AActor*, sfObject::SPtr> m_selectedActors;
    sfSession::SPtr m_sessionPtr;
    int m_numSyncedActors;
    bool m_movingActors;
    TSet<AActor*> m_movedActors;
    bool m_collectGarbage;
    float m_bspRebuildDelay;

    TSharedPtr<sfLevelManager> m_levelManagerPtr;

    /**
     * Checks for selection changes and requests locks on newly selected objects and unlocks unselected objects.
     */
    void UpdateSelection();

    /**
     * Destroys an actor.
     *
     * @param   AActor* actorPtr to destroy.
     */
    void DestroyActor(AActor* actorPtr);

    /**
     * Destroys actors that don't exist on the server in the given level.
     *
     * @param   ULevel* levelPtr - level to check
     */
    void DestroyUnsyncedActorsInLevel(ULevel* levelPtr);

    /**
     * Destroys components of an actor that don't exist on the server.
     *
     * @param   AActor* actorPtr to destroy unsynced components for.
     */
    void DestroyUnsyncedComponents(AActor* actorPtr);

    /**
     * Reverts folders to server values for actors whose folder changed while locked.
     */
    void RevertLockedFolders();

    /**
     * Recreates actors that were deleted while locked.
     */
    void RecreateLockedActors();

    /**
     * Deletes folders that were emptied by other users.
     */
    void DeleteEmptyFolders();

    /**
     * Decreases the rebuild bsp timer and rebuilds bsp if it reaches 0.
     *
     * @param   deltaTime in seconds since the last cick.
     */
    void RebuildBSPIfNeeded(float deltaTime);

    /**
     * Marks an actor as needing it's BSP rebuilt, and resets a timer to rebuild BSP.
     *
     * @param   AActor* actorPtr whose BSP needs to be rebuilt.
     */
    void MarkBSPStale(AActor* actorPtr);

    /**
     * Recursively removes child components of a deleted actor from the sfObjectMap. Optionally removes child actors or
     * adds them as children of a level object.
     *
     * @param   sfObject::SPtr objPtr to recursively cleanup children for.
     * @param   sfObject::SPtr levelObjPtr to add actor child objects to. Not used if recurseChildActors is true.
     * @param   bool recurseChildActors - if true, child actors and their descendants will also be removed from the
     *          sfObjectMap.
     */
    void CleanUpChildrenOfDeletedObject(
        sfObject::SPtr objPtr,
        sfObject::SPtr levelObjPtr = nullptr,
        bool recurseChildActors = false);

    /**
     * Called when an actor is added to the level.
     *
     * @param   AActor* actorPtr
     */
    void OnActorAdded(AActor* actorPtr);

    /**
     * Called when an actor is deleted from the level.
     *
     * @param   AActor* actorPtr
     */
    void OnActorDeleted(AActor* actorPtr);

    /**
     * Called when an actor is attached to or detached from another actor.
     *
     * @param   AActor* actorPtr
     * @param   const AActor* parentPtr
     */
    void OnAttachDetach(AActor* actorPtr, const AActor* parentPtr);

    /**
     * Called when an actor's folder changes.
     *
     * @param   const AActor* actorPtr
     * @param   FName oldFolder
     */
    void OnFolderChange(const AActor* actorPtr, FName oldFolder);

    /**
     * Called when an actor's label changes.
     *
     * @param   const AActor* actorPtr
     */
    void OnLabelChanged(AActor* actorPtr);

    /**
     * Called when an object starts being dragged in the viewport.
     *
     * @param   UObject& obj
     */
    void OnMoveStart(UObject& obj);

    /**
     * Called when an object stops being dragged in the viewport.
     *
     * @param   UObject& object
     */
    void OnMoveEnd(UObject& obj);

    /**
     * Called when an actor moves.
     *
     * @param   AActor* actorPtr
     */
    void OnActorMoved(AActor* actorPtr);

    /**
     * Called for each actor in an undo delete transaction, or redo create transaction. Recreates the actor on the
     * server, or deletes the actor if another actor with the same name already exists.
     *
     * @param   AActor* actorPtr
     */
    void OnUndoDelete(AActor* actorPtr);

    /**
     * Sends new label and name values to the server, or reverts to the server values if the actor is locked.
     *
     * @param   AActor* actorPtr to sync label and name for.
     * @param   sfObject::SPtr objPtr for the actor.
     * @param   sfDictionaryProperty::SPtr propertiesPtr for the actor.
     */
    void SyncLabelAndName(AActor* actorPtr, sfObject::SPtr objPtr, sfDictionaryProperty::SPtr propertiesPtr);

    /**
     * Sends a new folder value to the server, or reverts to the server value if the actor is locked.
     *
     * @param   AActor* actorPtr to sync folder for.
     * @param   sfObject::SPtr objPtr for the actor.
     * @param   sfDictionaryProperty::SPtr propertiesPtr for the actor.
     */
    void SyncFolder(AActor* actorPtr, sfObject::SPtr objPtr, sfDictionaryProperty::SPtr propertiesPtr);

    /**
     * Sends a new parent value to the server, or reverts to the server value if the actor or new parent are locked.
     *
     * @param   AActor* actorPtr to sync parent for.
     * @param   sfObject::SPtr objPtr for the actor.
     */
    void SyncParent(AActor* actorPtr, sfObject::SPtr objPtr);

    /**
     * Creates actor objects on the server.
     *
     * @param   const TArray<AActor*>& actors to upload.
     */
    void UploadActors(const TArray<AActor*>& actors);

    /**
     * Recursively creates actor objects for an actor and its children.
     *
     * @param   AActor* actorPtr to create object for.
     * @return  sfObject::SPtr object for the actor.
     */
    sfObject::SPtr CreateObject(AActor* actorPtr);

    /**
     * Creates or finds an actor for an object and initializes it with server values. Recursively initializes child
     * actors for child objects.
     *
     * @param   sfObject::SPtr objPtr to initialize actor for.
     * @param   ULevel* levelPtr the actor belongs to.
     * @return  AActor* actor for the object.
     */
    AActor* InitializeActor(sfObject::SPtr objPtr, ULevel* levelPtr);

    /**
     * Iterates a list of objects and their descendants, looking for child actors whose objects are not attached and
     * attaches those objects.
     *
     * @param   const std::list<sfObject::SPtr>& objects
     */
    void FindAndAttachChildren(const std::list<sfObject::SPtr>& objects);

    /**
     * Checks for and sends transform changes for selected components in an actor to the server.
     *
     * @param   AActor* actorPtr to send transform update for.
     */
    void SyncComponentTransforms(AActor* actorPtr);

    /**
     * Registers property change handlers for server events.
     */
    void RegisterPropertyChangeHandlers();

    /**
     * Locks an actor.
     * 
     * @param   AActor* actorPtr
     * @param   sfObject::SPtr objPtr for the actor.
     */
    void Lock(AActor* actorPtr, sfObject::SPtr objPtr);

    /**
     * Unlocks an actor.
     *
     * @param   AActor* actorPtr
     */
    void Unlock(AActor* actorPtr);

    /**
     * Called when an actor is created by another user.
     *
     * @param   sfObject::SPtr objPtr that was created.
     * @param   int childIndex of new object. -1 if object is a root
     */
    virtual void OnCreate(sfObject::SPtr objPtr, int childIndex) override;

    /**
     * Called when an actor is deleted by another user.
     *
     * @param   sfObject::SPtr objPtr that was deleted.
     */
    virtual void OnDelete(sfObject::SPtr objPtr) override;

    /**
     * Called when an actor is locked by another user.
     *
     * @param   sfObject::SPtr objPtr that was locked.
     */
    virtual void OnLock(sfObject::SPtr objPtr) override;

    /**
     * Called when an actor is unlocked by another user.
     *
     * @param   sfObject::SPtr objPtr that was unlocked.
     */
    virtual void OnUnlock(sfObject::SPtr objPtr) override;

    /**
     * Called when an actor's lock owner changes.
     *
     * @param   sfObject::SPtr objPtr whose lock owner changed.
     */
    virtual void OnLockOwnerChange(sfObject::SPtr objPtr) override;

    /**
     * Called when an actor's parent is changed by another user.
     *
     * @param   sfObject::SPtr objPtr whose parent changed.
     * @param   int childIndex of the object. -1 if the object is a root.
     */
    virtual void OnParentChange(sfObject::SPtr objPtr, int childIndex) override;

    /**
     * Called when an object is modified by an undo or redo transaction.
     *
     * @param   sfObject::SPtr objPtr for the uobject that was modified. nullptr if the uobjPtr is not synced.
     * @param   UObject* uobjPtr that was modified.
     * @return  bool true if event was handled and need not be processed by other handlers.
     */
    virtual bool OnUndoRedo(sfObject::SPtr objPtr, UObject* uobjPtr) override;

    /**
     * Enables the actor attached and dettached event handler.
     */
    void EnableParentChangeHandler();

    /**
     * Disables the actor attached and dettached event handler.
     */
    void DisableParentChangeHandler();

    /**
     * Calls OnLockStateChange event handlers.
     *
     * @param   sfObject::SPtr objPtr whose lock state changed
     * @param   AActor* actorPtr
     */
    void InvokeOnLockStateChange(sfObject::SPtr objPtr, AActor* actorPtr);

    /**
     * Clears collections of actors.
     */
    void ClearActorCollections();

    /**
     * Deletes all actors in the given level.
     *
     * @param   sfObject::SPtr levelObjPtr
     * @param   ULevel* levelPtr
     */
    void OnRemoveLevel(sfObject::SPtr levelObjPtr, ULevel* levelPtr);

    /**
     * Calls OnCreate on every child of the given level sfObject. Destroys all unsynced actors after.
     *
     * @param   sfObject::SPtr sfLevelObjPtr
     * @param   ULevel* levelPtr
     */
    void OnSFLevelObjectCreate(sfObject::SPtr sfLevelObjPtr, ULevel* levelPtr);

    /**
     * Detaches the given actor from its parent if the given sfObject's parent is a level object and returns true.
     * Otherwise, returns false.
     *
     * @param   sfObject::SPtr objPtr
     * @param   AActor* actorPtr
     * @return  bool
     */
    bool DetachIfParentIsLevel(sfObject::SPtr objPtr, AActor* actorPtr);

    /**
     * Logs out an error that the given sfObject has no parent and then leaves the session.
     *
     * @param   sfObject::SPtr objPtr
     */
    void LogNoParentErrorAndDisconnect(sfObject::SPtr objPtr);
};
