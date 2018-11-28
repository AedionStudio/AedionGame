#pragma once

#include "sfValueProperty.h"
#include "sfSession.h"
#include "sfUPropertyInstance.h"

#include <CoreMinimal.h>

using namespace KS;
using namespace KS::SceneFusion2;

/**
 * Utility for converting between SF properties and common Unreal types.
 */
class sfPropertyUtil
{
public:
    /**
     * Syncs property change.
     *
     * @param   UObject* uobjPtr
     * @param   UProperty* upropPtr
     */
    typedef std::function<void(UObject* uobjPtr, UProperty* upropPtr)> PropertyChangeHandler;

    /**
     * On get asset property event.
     *
     * @param   UObject* assetPtr
     */
    DECLARE_EVENT_OneParam(sfPropertyUtil, OnGetAssetPropertyEvent, UObject*);

    /**
     * Invoked when getting the value of an asset reference property.
     *
     * @return  OnGetAssetPropertyEvent&
     */
    static OnGetAssetPropertyEvent& OnGetAssetProperty()
    {
        return m_onGetAssetProperty;
    }

    /**
     * Constructs a property from a vector.
     *
     * @param   const FVector& value
     * @return  sfValueProperty::SPtr
     */
    static sfValueProperty::SPtr FromVector(const FVector& value)
    {
        return ToProperty(value);
    }

    /**
     * Converts a property to a vector.
     *
     * @param   sfProperty::SPtr propertyPtr
     * @return  FVector
     */
    static FVector ToVector(sfProperty::SPtr propertyPtr)
    {
        return FromProperty<FVector>(propertyPtr);
    }

    /**
     * Constructs a property from a rotator.
     *
     * @param   const FRotator& value
     * @return  sfValueProperty::SPtr
     */
    static sfValueProperty::SPtr FromRotator(const FRotator& value)
    {
        return ToProperty(value);
    }

    /**
     * Converts a property to a rotator.
     *
     * @param   sfProperty::SPtr propertyPtr
     * @return  FRotator
     */
    static FRotator ToRotator(sfProperty::SPtr propertyPtr)
    {
        return FromProperty<FRotator>(propertyPtr);
    }

    /**
     * Constructs a property from a quat.
     *
     * @param   const FQuat& value
     * @return  sfValueProperty::SPtr
     */
    static sfValueProperty::SPtr FromQuat(const FQuat& value)
    {
        return ToProperty(value);
    }

    /**
     * Converts a property to a quat.
     *
     * @param   sfProperty::SPtr propertyPtr
     * @return  FQuat
     */
    static FQuat ToQuat(sfProperty::SPtr propertyPtr)
    {
        return FromProperty<FQuat>(propertyPtr);
    }

    /**
     * Constructs a property from a box.
     *
     * @param   const FBox& value
     * @return  sfValueProperty::SPtr
     */
    static sfValueProperty::SPtr FromBox(const FBox& value)
    {
        return ToProperty(value);
    }

    /**
     * Converts a property to a box.
     *
     * @param   sfProperty::SPtr propertyPtr
     * @return  FBox
     */
    static FBox ToBox(sfProperty::SPtr propertyPtr)
    {
        return FromProperty<FBox>(propertyPtr);
    }

    /**
     * Constructs a property from a string.
     *
     * @param   const FString& value
     * @return  sfValueProperty::SPtr
     */
    static sfValueProperty::SPtr FromString(const FString& value);

    /**
     * Converts a property to a string.
     *
     * @param   sfProperty::SPtr propertyPtr
     * @return  FString
     */
    static FString ToString(sfProperty::SPtr propertyPtr);

    /**
     * Finds a uproperty of a uobject corresponding to an sfproperty.
     *
     * @param   UObject* uobjPtr to find property on.
     * @param   sfProperty::SPtr propPtr to find corresponding uproperty for.
     * @return  sfUPropertyInstance
     */
    static sfUPropertyInstance FindUProperty(UObject* uobjPtr, sfProperty::SPtr propPtr);

    /**
     * Converts a UProperty to an sfProperty using reflection.
     *
     * @param   UObject* uobjPtr to get property from.
     * @param   UProperty* upropPtr
     * @return  sfProperty::SPtr
     */
    static sfProperty::SPtr GetValue(UObject* uobjPtr, UProperty* upropPtr);

    /**
     * Sets a UProperty using reflection to a value from an sfValueProperty.
     *
     * @param   UObject* uobjPtr to set property on.
     * @param   const sfUPropertyInstance& upropInstance
     * @param   sfProperty::SPtr propPtr to get value from.
     * @return  bool true if the value changed.
     */
    static bool SetValue(UObject* uobjPtr, const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr);

    /**
     * Checks if an object has the default value for a property using reflection. Returns false if the property cannot
     * be synced by Scene Fusion.
     *
     * @param   UObject* uobjPtr to check property on.
     * @param   UProperty* upropPtr to check.
     * @return  bool false if the property does not have it's default value, or if the property type cannot be synced
     *          by Scene Fusion.
     */
    static bool IsDefaultValue(UObject* uobjPtr, UProperty* upropPtr);

    /**
     * Sets a property on an object to the default value using reflection. Does nothing if the property cannot be
     * synced by Scene Fusion.
     *
     * @param   UObject* objPtr to set property on.
     * @param   UProperty* upropPtr to set.
     */
    static void SetToDefaultValue(UObject* uobjPtr, UProperty* upropPtr);

    /**
     * Adds a property into the force to sync list.
     *
     * @param   FName ownerClassName
     * @param   FName propertyName
     */
    static void AddPropertyToForceSyncList(FName ownerClassName, FName propertyName);

    /**
     * Ignore the CPF_DisableEditOnInstance flag for the given class when deciding if a property is syncable.
     *
     * @param   FName className
     */
    static void IgnoreDisableEditOnInstanceFlagForClass(FName className);

    /**
     * Checks if a property is syncable.
     * A UProperty can be synced if it is in the force sync set or if the CPF_Edit flag is set,
     * the CPF_DisableEditOnInstance flag is not set and the CPF_EditConst flag is not set.
     *
     * @param   UObject* uobjPtr to check.
     * @param   UProperty* upropPtr to check.
     * @return  bool true if the property is syncable.
     */
    static bool IsSyncable(UObject* uobjPtr, UProperty* upropPtr);

    /**
     * Iterates all properties of an object using reflection and creates sfProperties for properties with non-default
     * values as fields in an sfDictionaryProperty.
     *
     * @param   UObject* uobjPtr to create properties for.
     * @param   sfDictionaryProperty::SPtr dictPtr to add properties to.
     * @param   const TArray<FString>* const blacklistPtr - if the property name is in this list, ignore the property.
     */
    static void CreateProperties(
        UObject* uobjPtr,
        sfDictionaryProperty::SPtr dictPtr,
        const TSet<FString>* const blacklistPtr = nullptr);

    /**
     * Applies property values from an sfDictionaryProperty to an object using reflection.
     *
     * @param   UObject* uobjPtr to apply property values to.
     * @param   sfDictionaryProperty::SPtr dictPtr to get property values from. If a value for a property is not in the
     *          dictionary, sets the property to its default value.
     * @param   const TArray<FString>* const blacklistPtr - if the property name is in this list, ignore the property.
     */
    static void ApplyProperties(
        UObject* uobjPtr,
        sfDictionaryProperty::SPtr dictPtr,
        const TSet<FString>* const blacklistPtr = nullptr);

    /**
     * Iterates all properties of an object using reflection and updates an sfDictionaryProperty when its values are
     * different from those on the object. Removes fields from the dictionary for properties that have their default
     * value.
     *
     * @param   UObject* uobjPtr to iterate properties on.
     * @param   sfDictionaryProperty::SPtr dictPtr to update.
     * @param   const TArray<FString>* const blacklistPtr - if the property name is in this list, ignore the property.
     */
    static void SendPropertyChanges(
        UObject* uobjPtr,
        sfDictionaryProperty::SPtr dictPtr,
        const TSet<FString>* const blacklistPtr = nullptr);

    /**
     * Sets a list of references to the given uobject using reflection.
     *
     * @param   UObject* uobjPtr to set references to.
     * @param   const std::vector<sfReferenceProperty::SPtr>& references to set to the uobject.
     */
    static void SetReferences(UObject* uobjPtr, const std::vector<sfReferenceProperty::SPtr>& references);

    /**
     * Copies the data from one property into another if they are the same property type.
     *
     * @param   sfProperty::SPtr destPtr to copy into.
     * @param   sfProperty::SPtr srcPtr to copy from.
     * @return  bool false if the properties were not the same type.
     */
    static bool Copy(sfProperty::SPtr destPtr, sfProperty::SPtr srcPtr);

    /**
     * Adds a uproperty instance's containing hash to the set of hashes that need rehashing. Does nothing if the
     * uproperty instance is not a key in a hash.
     *
     * @param   const sfUPropertyInstance& upropInstance
     */
    static void MarkHashStale(const sfUPropertyInstance& upropInstance);

    /**
     * Marks a property as being changed so we will call the appropriate change events for it when BroadcastChangeEvents
     * is called.
     *
     * @param   UObject* uobjPtr the property belongs to.
     * @param   UProperty* upropPtr - uproperty that changed.
     * @param   sfProperty::SPtr propPtr - property that changed. If provided, will look for a uproperty with the name
     *          of the sfProperty at depth 1.
     */
    static void MarkPropertyChanged(UObject* uobjPtr, UProperty* upropPtr, sfProperty::SPtr propPtr = nullptr);

    /**
     * Rehashes property containers whose keys were changed by other users.
     */
    static void RehashProperties();

    /**
     * Invokes property change events for properties that were changed by the functions in this class.
     */
    static void BroadcastChangeEvents();

    /**
     * Enables the property change event handler that syncs property changes.
     */
    static void EnablePropertyChangeHandler();

    /**
     * Disables the property change event handler that syncs property changes.
     */
    static void DisablePropertyChangeHandler();

    /**
     * Returns true if it is listening for property changes.
     *
     * @return  bool
     */
    static bool ListeningForPropertyChanges();

    /**
     * Sends new property values to the server for properties that were changed locally, or reverts them to the server
     * value if they belong to a locked object.
     */
    static void SyncProperties();

    /**
     * Sends a new property value to the server, or reverts it to the server value if its object is locked.
     *
     * @param   sfObject::SPtr objPtr the property belongs to.
     * @param   UObject* uobjPtr the property belongs to.
     * @param   UProperty* upropPtr to sync.
     * @param   bool applyServerValue - if true, will apply the server value even if the object is unlocked.
     */
    static void SyncProperty(
        sfObject::SPtr objPtr,
        UObject* uobjPtr,
        UProperty* upropPtr,
        bool applyServerValue = false);

    /**
     * Sends a new property value to the server, or reverts it to the server value if its object is locked.
     *
     * @param   sfObject::SPtr objPtr the property belongs to.
     * @param   UObject* uobjPtr the property belongs to.
     * @param   const FName& name of property to sync.
     * @param   bool applyServerValue - if true, will apply the server value even if the object is unlocked.
     */
    static void SyncProperty(
        sfObject::SPtr objPtr,
        UObject* uobjPtr,
        const FName& name,
        bool applyServerValue = false);

    /**
     * Rehashes stale properties and broadcasts change events. Clears state.
     */
    static void CleanUp();

    /**
     * Registers UProperty change handler for the given class.
     *
     * @param   FName classname
     * @param   PropertyChangeHandler handler
     */
    static void RegisterPropertyChangeHandlerForClass(FName className, PropertyChangeHandler handler);

    /**
     * Unregisters UProperty change handler for the given class.
     *
     * @param   FName classname
     */
    static void UnregisterPropertyChangeHandlerForClass(FName className);

private:
    /**
     * Holds getter and setter delegates for converting between a UProperty type and sfValueProperty.
     */
    struct TypeHandler
    {
    public:
        /**
         * Gets a UProperty value using reflection and converts it to an sfProperty.
         *
         * @param   const sfUPropertyInstance& to get value for.
         * @return  sfProperty::SPtr
         */
        typedef std::function<sfProperty::SPtr(const sfUPropertyInstance&)> Getter;

        /**
         * Sets a UProperty value using reflection to a value from an sfProperty.
         *
         * @param   const sfUPropertyInstance& to set value for.
         * @param   sfProperty::SPtr to get value from.
         * @return  bool true if the value changed.
         */
        typedef std::function<bool(const sfUPropertyInstance&, sfProperty::SPtr)> Setter;

        /**
         * Getter
         */
        Getter Get;

        /**
         * Setter
         */
        Setter Set;

        /**
         * Constructor
         *
         * @param   Getter getter
         * @param   Setter setter
         */
        TypeHandler(Getter getter, Setter setter) :
            Get{ getter },
            Set{ setter }
        {

        }
    };

    // TMaps seem buggy and I don't trust them. Dereferencing the pointer returned by TMap.find causes an access
    // violation, so we use std::unordered_map which works fine.
    // Keys are UProperty class name ids.
    static std::unordered_map<int, TypeHandler> m_typeHandlers;
    static TMap<FScriptMap*, TSharedPtr<FScriptMapHelper>> m_staleMaps;// maps that need rehashing
    static TMap<FScriptSet*, TSharedPtr<FScriptSetHelper>> m_staleSets;// sets that need rehashing
    // properties changed by the server we need to fire events for
    static TSet<TPair<UObject*, UProperty*>> m_serverChangedProperties;
    // properties changed locally we need to process
    static TSet<TPair<UObject*, UProperty*>> m_localChangedProperties;
    // we don't call property change handlers on non-editable properties unless they're in the white list
    static TSet<TPair<FName, FName>> m_forceSyncList;// key: owner class name, value: property name
    static FDelegateHandle m_onPropertyChangeHandle;
    static TMap<FName, PropertyChangeHandler> m_classNameToPropertyChangeHandler;
    static TSet<FName> m_syncDefaultOnlyList;// Sync default only properties for types in this list
    static OnGetAssetPropertyEvent m_onGetAssetProperty;

    /**
     * Registers UProperty type handlers.
     */
    static void Initialize();

    /**
     * Creates a property type handler.
     *
     * @param   UClass* typePtr to create handler for.
     * @param   TypeHandler::Getter getter for getting properties of the given type.
     * @param   TypeHandler::Setter setter for setting properties of the given type.
     */
    static void CreateTypeHandler(UClass* typePtr, TypeHandler::Getter getter, TypeHandler::Setter setter);

    /**
     * Returns true if the given UProperty is in the force to sync list.
     *
     * @param   UObject* uobjPtr to check
     * @param   UProperty* upropPtr to check
     * @return  bool
     */
    static bool IsPropertyInForceSyncList(UProperty* upropPtr);

    /**
     * Called when a property is changed through the details panel.
     *
     * @param   UObject* uobjPtr whose property changed.
     * @param   FPropertyChangedEvent& ev with information on what property changed.
     */
    static void OnUPropertyChange(UObject* uobjPtr, FPropertyChangedEvent& ev);

    /**
     * Gets a double property value using reflection converted to an sfProperty.
     *
     * @param   const sfUPropertyInstance& upropInstance to get.
     * @return  sfProperty::SPtr
     */
    static sfProperty::SPtr GetDouble(const sfUPropertyInstance& upropInstance);

    /**
     * Sets a double property value using reflection.
     *
     * @param   const sfUPropertyInstance& upropInstance to set.
     * @param   sfProperty::SPtr propPtr to get value from.
     * @return  bool true if the value changed.
     */
    static bool SetDouble(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr);

    /**
     * Gets a string property value using reflection converted to an sfProperty.
     *
     * @param   const sfUPropertyInstance& upropInstance to get.
     * @return  sfProperty::SPtr
     */
    static sfProperty::SPtr GetFString(const sfUPropertyInstance& upropInstance);

    /**
     * Sets a string property value using reflection.
     *
     * @param   const sfUPropertyInstance& upropInstance to set.
     * @param   sfProperty::SPtr propPtr to get value from.
     * @return  bool true if the value changed.
     */
    static bool SetFString(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr);

    /**
     * Gets a text property value using reflection converted to an sfProperty.
     *
     * @param   const sfUPropertyInstance& upropInstance to get.
     * @return  sfProperty::SPtr
     */
    static sfProperty::SPtr GetFText(const sfUPropertyInstance& upropInstance);

    /**
     * Sets a text property value using reflection.
     *
     * @param   const sfUPropertyInstance& upropInstance to set.
     * @param   sfProperty::SPtr propPtr to get value from.
     * @return  bool true if the value changed.
     */
    static bool SetFText(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr);

    /**
     * Gets a name property value using reflection converted to an sfProperty.
     *
     * @param   const sfUPropertyInstance& upropInstance to get.
     * @return  sfProperty::SPtr
     */
    static sfProperty::SPtr GetFName(const sfUPropertyInstance& upropInstance);

    /**
     * Sets a name property value using reflection.
     *
     * @param   const sfUPropertyInstance& upropInstance to set.
     * @param   sfProperty::SPtr propPtr to get value from.
     * @return  bool true if the value changed.
     */
    static bool SetFName(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr);

    /**
     * Gets an enum property value from an object using reflection converted to an sfProperty.
     *
     * @param   const sfUPropertyInstance& upropInstance to get.
     * @return  sfProperty::SPtr
     */
    static sfProperty::SPtr GetEnum(const sfUPropertyInstance& upropInstance);

    /**
     * Sets an enum property value using reflection.
     *
     * @param   const sfUPropertyInstance& upropInstance to set.
     * @param   sfProperty::SPtr propPtr to get value from.
     * @return  bool true if the value changed.
     */
    static bool SetEnum(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr);

    /**
     * Gets an array property value from an object using reflection converted to an sfProperty.
     *
     * @param   const sfUPropertyInstance& upropInstance to get.
     * @return  sfProperty::SPtrr
     */
    static sfProperty::SPtr GetArray(const sfUPropertyInstance& upropInstance);

    /**
     * Sets an array property value using reflection.
     *
     * @param   const sfUPropertyInstance& upropInstance to set.
     * @param   sfProperty::SPtr propPtr to get value from.
     * @return  bool true if the value changed.
     */
    static bool SetArray(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr);

    /**
     * Gets a map property value from an object using reflection converted to an sfProperty.
     *
     * @param   const sfUPropertyInstance& upropInstance to get.
     * @return  sfProperty::SPtr
     */
    static sfProperty::SPtr GetMap(const sfUPropertyInstance& upropInstance);

    /**
     * Sets a map property value using reflection.
     *
     * @param   const sfUPropertyInstance& upropInstance to set.
     * @param   sfProperty::SPtr propPtr to get value from.
     * @return  bool true if the value changed.
     */
    static bool SetMap(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr);

    /**
     * Gets a set property value from an object using reflection converted to an sfProperty.
     *
     * @param   const sfUPropertyInstance& upropInstance to get.
     * @return  sfProperty::SPtr
     */
    static sfProperty::SPtr GetSet(const sfUPropertyInstance& upropInstance);

    /**
     * Sets a set property value using reflection.
     *
     * @param   const sfUPropertyInstance& upropInstance to set.
     * @param   sfProperty::SPtr propPtr to get value from.
     * @return  bool true if the value changed.
     */
    static bool SetSet(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr);

    /**
     * Gets a struct property value from an object using reflection converted to an sfProperty.
     *
     * @param   const sfUPropertyInstance& upropInstance to get.
     * @return  sfProperty::SPtr
     */
    static sfProperty::SPtr GetStruct(const sfUPropertyInstance& upropInstance);

    /**
     * Sets a struct property value using reflection.
     *
     * @param   const sfUPropertyInstance& upropInstance to set.
     * @param   sfProperty::SPtr propPtr to get value from.
     * @return  bool true if the value changed.
     */
    static bool SetStruct(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr);

    /**
     * Gets an object property value from an object using reflection converted to an sfProperty.
     *
     * @param   const sfUPropertyInstance& upropInstance to get.
     * @return  sfProperty::SPtr
     */
    static sfProperty::SPtr GetObject(const sfUPropertyInstance& upropInstance);

    /**
     * Sets an object property value using reflection.
     *
     * @param   const sfUPropertyInstance& upropInstance to set.
     * @param   sfProperty::SPtr propPtr to get value from.
     * @return  bool true if the value changed.
     */
    static bool SetObject(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr);

    /**
     * Gets a soft object property value from an object using reflection converted to an sfProperty.
     *
     * @param   const sfUPropertyInstance& upropInstance to get.
     * @return  sfProperty::SPtr
     */
    static sfProperty::SPtr GetSoftObject(const sfUPropertyInstance& upropInstance);

    /**
     * Sets a soft object property value using reflection.
     *
     * @param   const sfUPropertyInstance& upropInstance to set.
     * @param   sfProperty::SPtr propPtr to get value from.
     * @return  bool true if the value changed.
     */
    static bool SetSoftObject(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr);

    /**
     * Creates property for the given object reference.
     *
     * @param   const sfUPropertyInstance& upropInstance
     * @param   UObject* referencePtr
     * @return  sfProperty::SPtr
     */
    static sfProperty::SPtr CreatePropertyForObjectReference(
        const sfUPropertyInstance& upropInstance,
        UObject* referencePtr);

    /**
     * Gets a class property value from an object using reflection converted to an sfProperty.
     *
     * @param   const sfUPropertyInstance& upropInstance to get.
     * @return  sfProperty::SPtr
     */
    static sfProperty::SPtr GetClass(const sfUPropertyInstance& upropInstance);

    /**
     * Sets a class property value using reflection.
     *
     * @param   const sfUPropertyInstance& upropInstance to set.
     * @param   sfProperty::SPtr propPtr to get value from.
     * @return  bool true if the value changed.
     */
    static bool SetClass(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr);

    /**
     * Gets a soft class property value from an object using reflection converted to an sfProperty.
     *
     * @param   const sfUPropertyInstance& upropInstance to get.
     * @return  sfProperty::SPtr
     */
    static sfProperty::SPtr GetSoftClass(const sfUPropertyInstance& upropInstance);

    /**
     * Sets a soft class property value using reflection.
     *
     * @param   const sfUPropertyInstance& upropInstance to set.
     * @param   sfProperty::SPtr propPtr to get value from.
     * @return  bool true if the value changed.
     */
    static bool SetSoftClass(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr);

    /**
     * Takes a pointer to a struct and sets it to point at a field of the struct using reflection. Returns false if the
     * uproperty is not a struct property.
     *
     * @param   const sfName& name of field to get.
     * @param   UProperty*& upropPtr - if not a struct property, returns false. Otherwise this is updated to point to
     *          the field property. If the field is not found, this is set to nullptr.
     * @param   void*& ptr to the struct data. Will be updated to point to the field data, if found.
     * @return  bool true if upropPtr is a struct property.
     */
    static bool GetStructField(const sfName& name, UProperty*& upropPtr, void*& ptr);

    /**
     * Takes a pointer to an array and sets it to point at an element of the array using reflection. Returns false if
     * the uproperty is not an array property.
     *
     * @param   index of element to get.
     * @param   UProperty*& upropPtr - if not an array property, returns false. Otherwise this is updated to point to
     *          the element property. If the element is not found, this is set to nullptr.
     * @param   void*& ptr to the array data. Will be updated to point to the element data, if found.
     * @return  bool true if upropPtr is an array property.
     */
    static bool GetArrayElement(int index, UProperty*& upropPtr, void*& ptr);

    /**
     * Takes a pointer to a map and sets it to point at a key or value of the map using reflection. Returns false if
     * the uproperty is not a map property.
     *
     * @param   index of element to get.
     * @param   UProperty*& upropPtr - if not a map property, returns false. Otherwise this is updated to point to the
     *          element property. If the element is not found, this is set to nullptr.
     * @param   void*& ptr to the map data. Will be updated to point to the element data, if found.
     * @param   TSharedPtr<FScriptMapHelper> outMapPtr - will point to the map if the element we are getting is a key.
     * @param   std::stack<sfProperty::SPtr>& propertyStack containing the sub properties to look for next. If the
     *          uproperty is a map and the index is within its bounds, the top property will be popped off the stack.
     *          If it's index is 0, we'll get the key, If 1, we'll get the value.
     * @return  bool true if upropPtr is a map property.
     */
    static bool GetMapElement(
        int index,
        UProperty*& upropPtr,
        void*& ptr,
        TSharedPtr<FScriptMapHelper>& outMapPtr,
        std::stack<sfProperty::SPtr>& propertyStack);

    /**
     * Takes a pointer to a set and sets it to point at an element of the set using reflection. Returns false if the
     * uproperty is not a set property.
     *
     * @param   index of element to get.
     * @param   UProperty*& upropPtr - if not a set property, returns false. Otherwise this is updated to point to the
     *          element property. If the element is not found, this is set to nullptr.
     * @param   void*& ptr to the set data. Will be updated to point to the element data, if found.
     * @param   TSharedPtr<FScriptMapHelper> outSetPtr - will point to the set.
     * @return  bool true if upropPtr is a map property.
     */
    static bool GetSetElement(
        int index,
        UProperty*& upropPtr,
        void*& ptr,
        TSharedPtr<FScriptSetHelper>& outSetPtr);

    /**
     * Gets the default object for an object.
     *
     * @param   UObject* uobjPtr to get default object for.
     * @return  UObject* default object
     */
    static UObject* GetDefaultObject(UObject* uobjPtr);

    /**
     * Adds, removes, and/or sets elements in a destination list to make it the same as a source list.
     *
     * @param   sfListProperty::SPtr destPtr to modify.
     * @param   sfListProperty::SPtr srcPtr to make destPtr a copy of.
     */
    static void CopyList(sfListProperty::SPtr destPtr, sfListProperty::SPtr srcPtr);

    /**
     * Adds, removes, and/or sets fields in a destination dictionary so to make it the same as a source dictionary.
     *
     * @param   sfDictionaryProperty::SPtr destPtr to modify.
     * @param   sfDictionaryProperty::SPtr srcPtr to make destPtr a copy of.
     */
    static void CopyDict(sfDictionaryProperty::SPtr destPtr, sfDictionaryProperty::SPtr srcPtr);

    /**
     * Constructs a property from a T.
     *
     * @param   const T& value
     * @return  sfValueProperty::SPtr
     */
    template<typename T>
    static sfValueProperty::SPtr ToProperty(const T& value)
    {
        const uint8_t* temp = reinterpret_cast<const uint8_t*>(&value);
        ksMultiType multiType(ksMultiType::BYTE_ARRAY, temp, sizeof(T), sizeof(T));
        return sfValueProperty::Create(std::move(multiType));
    }

    /**
     * Converts a property to T.
     *
     * @param   sfProperty::SPtr propertyPtr
     * @return  T
     */
    template<typename T>
    static T FromProperty(sfProperty::SPtr propertyPtr)
    {
        if (propertyPtr == nullptr || propertyPtr->Type() != sfProperty::VALUE)
        {
            return T();
        }
        sfValueProperty::SPtr valuePtr = propertyPtr->AsValue();
        return *(reinterpret_cast<const T*>(valuePtr->GetValue().GetData().data()));
    }

    /**
     * Creates a property handler for type T.
     */
    template<typename T>
    static void CreateTypeHandler()
    {
        CreateTypeHandler(T::StaticClass(),
            [](const sfUPropertyInstance& upropInstance)
            {
                T* tPtr = Cast<T>(upropInstance.Property());
                return sfValueProperty::Create(tPtr->GetPropertyValue(upropInstance.Data()));
            },
            [](const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr)
            {
                T* tPtr = Cast<T>(upropInstance.Property());
                if (!propPtr->Equals(sfValueProperty::Create(tPtr->GetPropertyValue(upropInstance.Data()))))
                {
                    tPtr->SetPropertyValue(upropInstance.Data(), propPtr->AsValue()->GetValue());
                    return true;
                }
                return false;
            }
        );
    }

    /**
     * Creates a property handler for type T that casts the value to U, where U is a type supported by ksMultiType.
     */
    template<typename T, typename U>
    static void CreateTypeHandler()
    {
        CreateTypeHandler(T::StaticClass(),
            [](const sfUPropertyInstance& upropInstance)
            {
                T* tPtr = Cast<T>(upropInstance.Property());
                return sfValueProperty::Create((U)tPtr->GetPropertyValue(upropInstance.Data()));
            },
            [](const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr)
            {
                T* tPtr = Cast<T>(upropInstance.Property());
                U value = propPtr->AsValue()->GetValue();
                if ((U)tPtr->GetPropertyValue(upropInstance.Data()) != value)
                {
                    tPtr->SetPropertyValue(upropInstance.Data(), value);
                    return true;
                }
                return false;
            }
        );
    }
};