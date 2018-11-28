#pragma once

#include <CoreMinimal.h>
#include <Runtime/CoreUObject/Public/UObject/UnrealType.h>

/**
 * Stores pointers to an Unreal property and to the data for an instance of that property.
 */
class sfUPropertyInstance
{
public:
    /**
     * Constructor for an invalid property instance.
     */
    sfUPropertyInstance() :
        m_propertyPtr{ nullptr },
        m_dataPtr{ nullptr },
        m_mapPtr{ nullptr },
        m_setPtr{ nullptr }
    {

    }

    /**
     * Constructor
     *
     * @param   UProperty* propertyPtr
     * @param   void* dataPtr
     */
    sfUPropertyInstance(UProperty* propertyPtr, void* dataPtr) :
        m_propertyPtr{ propertyPtr },
        m_dataPtr{ dataPtr },
        m_mapPtr{ nullptr },
        m_setPtr{ nullptr }
    {

    }

    /**
     * Constructor
     *
     * @param   UProperty* propertyPtr
     * @param   void* dataPtr
     * @param   TSharedPtr<FScriptMapHelper> mapPtr - container for this property, needed only if this property is a
     *          key in a hash.
     * @param   TSharedPtr<FScriptSetHelper> setPtr - container for this property, needed only if this property is a
     *          key in a hash.
     */
    sfUPropertyInstance(
        UProperty* propertyPtr,
        void* dataPtr,
        TSharedPtr<FScriptMapHelper> mapPtr,
        TSharedPtr<FScriptSetHelper> setPtr)
        :
        m_propertyPtr{ propertyPtr },
        m_dataPtr{ dataPtr },
        m_mapPtr{ mapPtr },
        m_setPtr{ setPtr }
    {

    }        

    /**
     * Checks if the property is valid.
     *
     * @return  bool true if the property pointer and data pointer are not null.
     */
    bool IsValid() const { return m_propertyPtr != nullptr && m_dataPtr != nullptr; }

    /**
     * Unreal property pointer.
     *
     * @return  UProperty*
     */
    UProperty* Property() const { return m_propertyPtr; }

    /**
     * Pointer to property instance data.
     *
     * @return  void*
     */
    void* Data() const { return m_dataPtr; }

    /**
     * If this property is a key in a map, this points to the map.
     *
     * @return  const TSharedPtr<FScriptMapHelper>&
     */
    const TSharedPtr<FScriptMapHelper>& ContainerMap() const { return m_mapPtr; }

    /**
     * If this property is a key in a set, this points ot the set.
     *
     * @return  const TSharedPtr<FScriptSetHelper>&
     */
    const TSharedPtr<FScriptSetHelper>& ContainerSet() const { return m_setPtr; }

private:
    UProperty* m_propertyPtr;
    void* m_dataPtr;
    // If this property is a key in a hash, we need a pointer to the hash container so it can be rehashed if we set the
    // property.
    TSharedPtr<FScriptMapHelper> m_mapPtr;
    TSharedPtr<FScriptSetHelper> m_setPtr;
};