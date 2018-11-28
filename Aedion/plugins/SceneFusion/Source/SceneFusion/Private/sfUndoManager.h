#pragma once

#include "Objects/sfUndoHook.h"
#include <CoreMinimal.h>
#include <Editor/UnrealEd/Classes/Editor/TransBuffer.h>
#include <Runtime/Launch/Resources/Version.h>

/**
 * Manager for handling undo events.
 */
class sfUndoManager
{
public:
    /**
     * Constructor
     */
    sfUndoManager();

    /**
     * Destructor
     */
    ~sfUndoManager();

    /**
     * Initialization. Sets up event handlers.
     */
    void Initialize();

    /**
     * Clean up
     */
    void CleanUp();

private:
    /**
     * This is a hack that allows us to add objects to a transaction, allowing us to run PostEditUndo code from our
     * objects before the PostEditUndo code from other objects in the transaction.
     */
    class TransactionHack : public FTransaction
    {
    public:
        /**
         * Adds an object to the changed object map of a transaction. The actual objects changed by the transaction are
         * not added until the transaction is applied, so any objects we add before applying the transaction will have
         * PostEditUndo called prior to the objects that are actually in the transaction.
         *
         * @param   UObject* uobjPtr
         */
        void AddChangedObject(UObject* uobjPtr)
        {
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 20
            FTransaction::FChangedObjectValue changedObjectValue(0, nullptr);
            ChangedObjects.Add(uobjPtr, changedObjectValue);
#else
            ChangedObjects.Add(uobjPtr);
#endif
        };
    };

    TSet<USceneComponent*> m_childrenToCheck;
    TSet<USceneComponent*> m_parentsToCheck;
    TSet<AActor*> m_destroyedActorsToCheck;
    UTransBuffer* m_undoBufferPtr;
    UsfUndoHook* m_undoHookPtr;
    FDelegateHandle m_onUndoHandle;
    FDelegateHandle m_onRedoHandle;
    FDelegateHandle m_beforeUndoRedoHandle;

    /**
     * Called when a transaction is undone.
     *
     * @param   FUndoSessionContext context
     * @param   bool success
     */
    void OnUndo(FUndoSessionContext context, bool success);

    /**
     * Called when a transaction is redone.
     *
     * @param   FUndoSessionContext context
     * @param   bool success
     */
    void OnRedo(FUndoSessionContext context, bool success);

    /**
     * Called when a transaction is undone or redone.
     *
     * @param   FUndoSessionContext context
     */
    void BeforeUndoRedo(FUndoSessionContext context);

    /**
     * Unreal can get in a bad state if another user changed the children of a component after the transaction was
     * recorded and we undo or redo the transaction, causing a component's child list to be incorrect. We call this
     * before a transaction to store the components of a transaction and their children in private member arrays we
     * can use to correct the bad state after the transaction. Unreal can also partially recreate actors in a
     * transaction that were deleted by another user, so this records the actors in the transaction that are deleted
     * so we can redelete them after the transaction.
     *
     * @param   const FTransaction* transactionPtr
     */
    void RecordPreTransactionState(const FTransaction* transactionPtr);

    /**
     * Checks for and corrects bad state in the child lists of components affected by a transaction.
     */
    void FixTransactedComponentChildren();

    /**
     * Destroys actors that were recreated by a transaction but aren't showing in the world outliner. This happens when
     * an actor in the transaction was destroyed by another user.
     *
     * @param   const TArray<UObject*>& objects in the transaction.
     */
    void DestroyUnwantedActors(const TArray<UObject*>& objects);

    /**
     * Called when a transaction is undone or redone. Sends changes made by the transaction to the server, or reverts
     * changed values to server values for locked objects.
     *
     * @param   FString action that was undone or redone.
     * @param   bool isUndo
     */
    void OnUndoRedo(FString action, bool isUndo);
};