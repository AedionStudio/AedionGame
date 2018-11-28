#pragma once
#include "sfBaseObjectManager.h"
#include "../sfUPropertyInstance.h"
#include <CoreUObject.h>
#include <unordered_map>

/**
 * Base class for IObjectManagers that sync UObject properties using reflection.
 */
class sfBaseUObjectManager : public sfBaseObjectManager
{
protected:
    /**
     * Handler for an sfProperty change event.
     *
     * @param   UObject* the changed property belongs to.
     * @param   sfProperty::SPtr new property value. nullptr if the property was set to the default value.
     * @return  bool if false, the default property handler will be called.
     */
    typedef std::function<bool(UObject*, sfProperty::SPtr)> PropertyChangeHandler;

    // Map of property names to custom sfProperty change event handlers
    std::unordered_map<sfName, PropertyChangeHandler> m_propertyChangeHandlers;

    /**
     * Tries to insert elements from an sfListProperty into an array using reflection. Returns false if the UProperty
     * is not an array property.
     *
     * @param   UObject* uobjPtr the property belongs to.
     * @param   const sfUPropertyInstance& upropInstance to try inserting array elements for. Returns false if this is
     *          not an array property.
     * @param   sfListProperty::SPtr listPtr with elements to insert.
     * @param   int index of first element to insert, and the index to insert at.
     * @param   int count - number of elements to insert.
     * @return  bool true if the UProperty was an array property.
     */
    bool ArrayInsert(
        UObject* uobjPtr,
        const sfUPropertyInstance& upropInstance,
        sfListProperty::SPtr listPtr,
        int index,
        int count);

    /**
     * Tries to remove elements from an array using reflection. Returns false if the UProperty is not an array
     * property.
     *
     * @param   const sfUPropertyInstance& upropInstance to try removing array elements from. Returns false if this is
     *          not an array property.
     * @param   int index of first element to remove.
     * @param   int count - number of elements to remove.
     * @return  bool true if the UProperty was an array property.
     */
    bool ArrayRemove(const sfUPropertyInstance& upropInstance, int index, int count);

    /**
     * Tries to insert elements from an sfListProperty into a set using reflection. Returns false if the UProperty is
     * not a set property.
     *
     * @param   UObject* uobjPtr the property belongs to.
     * @param   const sfUPropertyInstance& upropInstance to try inserting set elements for. Returns false if this is
     *          not a set property.
     * @param   sfListProperty::SPtr listPtr with elements to insert.
     * @param   int index of first element to insert, and the index to insert at.
     * @param   int count - number of elements to insert.
     * @return  bool true if the UProperty was a set property.
     */
    bool SetInsert(
        UObject* uobjPtr,
        const sfUPropertyInstance& upropInstance,
        sfListProperty::SPtr listPtr,
        int index,
        int count);

    /**
     * Tries to remove elements from a set using reflection. Returns false if the UProperty is not a set property.
     *
     * @param   const sfUPropertyInstance& upropInstance to try removing set elements from. Returns false if this is
     *          not a set property.
     * @param   int index of first element to remove.
     * @param   int count - number of elements to remove.
     * @return  bool true if the UProperty was a set property.
     */
    bool SetRemove(const sfUPropertyInstance& upropInstance, int index, int count);

    /**
     * Tries to insert elements from an sfListProperty into a map using reflection. Returns false if the UProperty is
     * not a map property.
     *
     * @param   UObject* uobjPtr the property belongs to.
     * @param   const sfUPropertyInstance& upropInstance to try inserting map elements for. Returns false if this is
     *          not a map property.
     * @param   sfListProperty::SPtr listPtr with elements to insert. Each element is another sfListProperty with two
                elements: a key followed by a value.
     * @param   int index of first element to insert, and the index to insert at.
     * @param   int count - number of elements to insert.
     * @return  bool true if the UProperty was a map property.
     */
    bool MapInsert(
        UObject* uobjPtr,
        const sfUPropertyInstance& upropInstance,
        sfListProperty::SPtr listPtr,
        int index,
        int count);

    /**
     * Tries to remove elements from a map using reflection. Returns false if the UProperty is not a map property.
     *
     * @param   const sfUPropertyInstance& upropInstance to try removing map elements from. Returns false if this is
     *          not a map property.
     * @param   int index of first element to remove.
     * @param   int count - number of elements to remove.
     * @return  bool true if the UProperty was a map property.
     */
    bool MapRemove(const sfUPropertyInstance& upropInstance, int index, int count);

    /**
     * Called when an object property changes.
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
     * Called when one or more elements are added to a list property.
     *
     * @param   sfListProperty::SPtr listPtr that elements were added to.
     * @param   int index elements were inserted at.
     * @param   int count - number of elements added.
     */
    virtual void OnListAdd(sfListProperty::SPtr listPtr, int index, int count) override;

    /**
     * Called when one or more elements are removed from a list property.
     *
     * @param   sfListProperty::SPtr listPtr that elements were removed from.
     * @param   int index elements were removed from.
     * @param   int count - number of elements removed.
     */
    virtual void OnListRemove(sfListProperty::SPtr listPtr, int index, int count) override;

    /**
     * Gets the uobject for an sfObject, or nullptr if the sfObject has no uobject.
     *
     * @param   sfObject::SPtr objPtr to get uobject for.
     * @return  UObject* uobject for the sfObject.
     */
    virtual UObject* GetUObject(sfObject::SPtr objPtr);
};