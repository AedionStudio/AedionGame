#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Classes/Components/MeshComponent.h"
#include "sfLockComponent.generated.h"


/**
 * Lock component for indicating an actor cannot be edited. This is added to each mesh component of the actor, and
 * adds a copy of the mesh as a child with a lock shader. It also deletes itself and unlocks the actor when copied.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UsfLockComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    /**
     * Constructor
     */
    UsfLockComponent();

    /**
     * Destructor
     */
    virtual ~UsfLockComponent();

    /**
     * Initialization
     */
    virtual void InitializeComponent() override;

    /**
     * Called after being duplicated. Destroys this component, its children if any, and unlocks the actor.
     */
    virtual void PostEditImport() override;

    /**
     * Called when the attach parent changes. If the DuplicateParentMesh was never called, does nothing. Otherwise if
     * this is the only lock component on the actor, destroys the children of this component. Otherwise destroys this
     * component.
     */
    virtual void OnAttachmentChanged() override;

    /**
     * Called when the component is destroyed. Destroys children components.
     *
     * @param   bool bDestroyingHierarchy
     */
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

    /**
     * Duplicates the parent mesh component and adds the duplicate as a child.
     *
     * @param   UMaterialInterface* materialPtr to use on the duplicate mesh. If nullptr, will use the current
     *          material.
     */
    void DuplicateParentMesh(UMaterialInterface* materialPtr = nullptr);

    /**
     * Sets the material of all child meshes.
     *
     * @param   UMaterialInterface* materialPtr
     */
    void SetMaterial(UMaterialInterface* materialPtr);

private:
    bool m_copied;
    bool m_initialized;
    FDelegateHandle m_tickerHandle;
    UMaterialInterface* m_materialPtr;
    
    /**
     * Called before saving the world. Unlocks the actor's transform.
     *
     * @param   uint32_t saveFlags
     * @param   UWorld* worldPtr that will be saved.
     */
    void PreSave(uint32_t saveFlags, UWorld* worldPtr);

    /**
     * Called after saving the world. Relocks the actor's transform.
     *
     * @param   uint32_t saveFlags
     * @param   UWorld* worldPtr that was saved.
     * @param   bool success - true if the save was successful.
     */
    void PostSave(uint32_t saveFlags, UWorld* worldPtr, bool success);

    /**
     * Called when a UProperty changes. If the property belongs to the parent object and contains the string "mesh",
     * replaces the child mesh component with a new duplicate of the parent.
     *
     * @param   UObject* uobjPtr whose property changed.
     * @param   FPropertyChangedEvent& ev
     */
    void OnUPropertyChange(UObject* uobjPtr, FPropertyChangedEvent& ev);
};
