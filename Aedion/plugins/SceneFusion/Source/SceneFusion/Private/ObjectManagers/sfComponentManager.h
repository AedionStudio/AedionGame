#pragma once

#include <CoreMinimal.h>
#include <GameFramework/Actor.h>
#include <sfSession.h>
#include "sfBaseUObjectManager.h"

using namespace KS::SceneFusion2;

class sfComponentManager : public sfBaseUObjectManager
{
public:
    friend class sfActorManager;
    friend class sfUndoManager;
    friend class UsfMissingComponent;
    friend class UsfMissingSceneComponent;

    /**
     * Constructor
     */
    sfComponentManager();

    /**
     * Destructor
     */
    virtual ~sfComponentManager();

    /**
     * Initialization. Called after connecting to a session.
     */
    virtual void Initialize() override;

    /**
     * Deinitialization. Called after disconnecting from a session.
     */
    virtual void CleanUp() override;

    /**
     * Checks if a component is syncable.
     *
     * @return  bool true if the component is syncable.
     */
    bool IsSyncable(UActorComponent* componentPtr);

    /**
     * Sends a new transform value to the server, or applies the server value to the component if the object is locked.
     *
     * @param   USceneComponent* componentPtr to sync transform for.
     * @param   bool applyServerValues - if true, applies server values to the component even if the object is
     *          unlocked.
     */
    void SyncTransform(USceneComponent* componentPtr, bool applyServerValues = false);

    /**
     * Checks for new, deleted, renamed, and reparented components and sends changes to the server, or reverts to the
     * server state if the actor is locked.
     *
     * @param   AActor* actorPtr to sync components for.
     * @param   sfObject::SPtr actorObjPtr
     */
    void SyncComponents(AActor* actorPtr, sfObject::SPtr actorObjPtr);

private:
    sfSession::SPtr m_sessionPtr;
    FDelegateHandle m_onApplyObjectToActorHandle;

    /**
     * Creates an sfObject for a component and uploads it to the server.
     *
     * @param UActorComponent* componentPtr
     */
    void Upload(UActorComponent* componentPtr);

    /**
     * Recursively creates sfObjects for a component and its children.
     *
     * @param   UActorComponent* componentPtr to create object for.
     * @return  sfObject::SPtr object for the actor.
     */
    sfObject::SPtr CreateObject(UActorComponent* componentPtr);

    /**
     * Creates or finds a component for an object and initializes it with server values. Recursively initializes
     * children for child objects.
     *
     * @param   AActor* actorPtr to add component to.
     * @param   sfObject::SPtr objPtr to initialize component for.
     * @return  UActorComponent* component for the object.
     */
    UActorComponent* InitializeComponent(AActor* actorPtr, sfObject::SPtr objPtr);

    /**
     * Iterates an object and its descendents, looking for children whose objects are not attached and
     * attaches those objects.
     *
     * @param   const std::list<sfObject::SPtr>& objects
     */
    void FindAndAttachChildren(sfObject::SPtr objPtr);

    /**
     * Registers property change handlers for server events.
     */
    void RegisterPropertyChangeHandlers();

    /**
     * Recursively iterates the component children of an object, and recreates destroyed components.
     *
     * @param sfObject::SPtr objPtr to restore deleted component children for.
     */
    void RestoreDeletedComponents(sfObject::SPtr objPtr);

    /**
     * Recursively iterates the component children of an object, looking for destroyed components and deletes their
     * corresponding objects.
     *
     * @param   sfObject::Sptr objPtr to check for deleted child components.
     */
    void FindDeletedComponents(sfObject::SPtr objPtr);

    /**
     * Sets the parent of a component on the server, or moves the component back to it's server parent if the component
     * is locked.
     *
     * @param   AActor* actorPtr the component belongs to.
     * @param   UActorComponent* componentPtr to sync parent for.
     * @param   sfObject::SPtr objPtr for the component.
     */
    void SyncParent(AActor* actorPtr, UActorComponent* componentPtr, sfObject::SPtr objPtr);

    /**
     * Called when a component is created by another user.
     *
     * @param   sfObject::SPtr objPtr that was created.
     * @param   int childIndex of new object. -1 if object is a root.
     */
    virtual void OnCreate(sfObject::SPtr objPtr, int childIndex) override;

    /**
     * Called when a component is deleted by another user.
     *
     * @param   sfObject::SPtr objPtr that was deleted.
     */
    virtual void OnDelete(sfObject::SPtr objPtr) override;

    /**
     * Called when a component's parent is changed by another user.
     *
     * @param   sfObject::SPtr objPtr whose parent changed.
     * @param   int childIndex of the object. -1 if the object is a root.
     */
    virtual void OnParentChange(sfObject::SPtr objPtr, int childIndex) override;

    /**
     * Called when a component property changes.
     *
     * @param   sfProperty::SPtr propertyPtr that changed.
     */
    virtual void OnPropertyChange(sfProperty::SPtr propertyPtr) override;

    /**
     * Called when a field is removed from a dictionary property.
     *
     * @param   sfDictionaryProperty::SPtr dictPtr the field was removed from.
     * @param   const sfName& name of removed field.
     */
    virtual void OnRemoveField(sfDictionaryProperty::SPtr dictPtr, const sfName& name) override;

    /**
     * Called when a property on an component changes.
     *
     * @param   sfObject::SPtr objPtr for the component whose property changed.
     * @param   UObject* uobjPtr whose property changed.
     * @param   UProperty* upropPtr that changed.
     * @return  bool false if the property change event should be handled by the default handler.
     */
    virtual bool OnUPropertyChange(sfObject::SPtr objPtr, UObject* uobjPtr, UProperty* upropPtr) override;

    /**
     * Called when an object is modified by an undo or redo transaction.
     *
     * @param   sfObject::SPtr objPtr for the uobject that was modified. nullptr if the uobjPtr is not synced.
     * @param   UObject* uobjPtr that was modified.
     * @param   bool levelChanged - true if a level object was modified by the transaction.
     * @return  bool true if event was handled and need not be processed by other handlers.
     */
    virtual bool OnUndoRedo(sfObject::SPtr objPtr, UObject* uobjPtr) override;

    /**
     * Called when attempting to apply an object to an actor via drag drop. Sends components material changes
     * to the server, or reverts it to the server value if the actor is locked.
     *
     * @param   UObject* uobjPtr
     * @param   AActor* actorPtr
     */
    void OnApplyObjectToActor(UObject* uobjPtr, AActor* actorPtr);
};
