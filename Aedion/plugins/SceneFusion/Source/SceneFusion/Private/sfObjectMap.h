#pragma once

#include <sfObject.h>
#include <CoreMinimal.h>
#include <unordered_map>

using namespace KS::SceneFusion2;

/**
 * Maps sfObjects to uobjects and vice versa.
 */
class sfObjectMap
{
public:
    /**
     * Checks if a uobject is in the map.
     *
     * @param   const UObject* uobjPtr
     * @return  bool true if the uobject is in the map.
     */
    static bool Contains(const UObject* uobjPtr);

    /**
     * Checks if an sfObject is in the map.
     *
     * @param   sfObject::SPtr objPtr
     * @return  bool true if the object is in the map.
     */
    static bool Contains(sfObject::SPtr objPtr);
    
    /**
     * Gets the sfObject for a uobject, or nullptr if the uobject has no sfObject.
     *
     * @param   const UObject* uobjPtr
     * @return  sfObject::SPtr sfObject for the uobject, or nullptr if none was found.
     */
    static sfObject::SPtr GetSFObject(const UObject* uobjPtr);

    /**
     * Gets the sfObject for a uobject, or creates one with an empty dictionary property and adds it to the map if none
     * was found.
     *
     * @param   UObject* uobjPtr
     * @param   const sfName& type of object to create.
     * @return  sfObject::SPtr sfObject for the uobject.
     */
    static sfObject::SPtr GetOrCreateSFObject(UObject* uobjPtr, const sfName& type);

    /**
     * Gets the uobject for an sfObject, or nullptr if the sfObject has no uobject.
     *
     * @param   sfObject::SPtr objPtr to get uobject for.
     * @return  UObject* uobject for the sfObject.
     */
    static UObject* GetUObject(sfObject::SPtr objPtr);

    /**
     * Adds a mapping between a uobject and an sfObject.
     *
     * @param   sfObject::SPtr objPtr
     * @param   UObject* uobjPtr
     */
    static void Add(sfObject::SPtr objPtr, UObject* uobjPtr);

    /**
     * Removes a uobject and its sfObject from the map.
     *
     * @param   const UObject* uobjPtr to remove.
     * @return  sfObject::SPtr that was removed, or nullptr if the uobject was not in the map.
     */
    static sfObject::SPtr Remove(const UObject* uobjPtr);

    /**
     * Removes an sfObject and its uobject from the map.
     *
     * @param   sfObject::SPtr objPtr to remove.
     * @return  UObject* that was removed, or nullptr if the sfObject was not in the map.
     */
    static UObject* Remove(sfObject::SPtr objPtr);

    /**
     * Clears the map.
     */
    static void Clear();

    /**
     * @return  std::unordered_map<sfObject::SPtr, UObject*>::iterator pointing to the first pair in the map.
     */
    static std::unordered_map<sfObject::SPtr, UObject*>::iterator Begin();

    /**
     * @return  std::unordered_map<sfObject::SPtr, UObject*>::iterator pointing past the last pair in the map.
     */
    static std::unordered_map<sfObject::SPtr, UObject*>::iterator End();

    /**
     * Gets the uobject for an sfObject cast to T*.
     *
     * @param   const sfObject::SPtr objPtr
     * @return  T* uobject for the sfObject, or nullptr not found or not of type T.
     */
    template<typename T>
    static T* Get(const sfObject::SPtr objPtr)
    {
        return Cast<T>(GetUObject(objPtr));
    }

private:
    static TMap<const UObject*, sfObject::SPtr> m_uToSFObjectMap;
    static std::unordered_map<sfObject::SPtr, UObject*> m_sfToUObjectMap;
};