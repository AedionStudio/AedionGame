#pragma once

#include <CoreMinimal.h>
#include <sfObject.h>
#include <sfDictionaryProperty.h>
#include <Materials/MaterialInterface.h>
#include "sfBaseUObjectManager.h"
#include "../sfIStandInGenerator.h"

using namespace KS::SceneFusion2;

/**
 * Generates bounding-box mesh stand-ins for missing meshes. Uploads mesh bounding-box data so stand-ins can be created
 * to fit the dimensions of the missing mesh.
 */
class sfMeshStandInManager : public sfBaseUObjectManager, public sfIStandInGenerator
{
public:
    /**
     * Constructor
     */
    sfMeshStandInManager();

    /**
     * Destructor
     */
    virtual ~sfMeshStandInManager();

    /**
     * Deinitialization. Called after disconnecting from a session.
     */
    virtual void CleanUp() override;

private:
    // Keys are mesh paths, values are their bounding-boxes.
    sfDictionaryProperty::SPtr m_meshBoundsPtr;
    FDelegateHandle m_onGetAssetPropertyHandle;
    UMaterialInterface* m_materialPtr;

    /**
     * Called when the mesh bounds object is created by another user.
     *
     * @param   sfObject::SPtr objPtr that was created.
     * @param   int childIndex of new object. -1 if object is a root
     */
    virtual void OnCreate(sfObject::SPtr objPtr, int childIndex) override;

    /**
     * Generates a mesh stand-in using the dimensions of the missing mesh.
     *
     * @param   const FString& path of missing mesh.
     * @param   UObject* uobjPtr - mesh to generate data for.
     */
    virtual void Generate(const FString& path, UObject* uobjPtr) override;

    /**
     * Sends mesh bounding-box data to the server if it hasn't already been sent.
     *
     * @param   UObject* uobjPtr - mesh to send bounding-box for.
     */
    void SendBounds(UObject* uobjPtr);

    /**
     * Gets the bounding-box from server data for the mesh at the given path.
     *
     * @param   const sfName& path to mesh.
     * @return  FBox bounds of the mesh.
     */
    FBox GetBounds(const sfName& path);

    /**
     * Replaces characters in a mesh path that aren't permitted in property names with "//".
     *
     * @param   FString path to sanitize.
     * @return  sfName sanitized path.
     */
    sfName SanitizePath(FString path);
};