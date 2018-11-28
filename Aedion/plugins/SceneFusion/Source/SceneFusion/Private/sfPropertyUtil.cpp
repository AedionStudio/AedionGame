#include "sfPropertyUtil.h"
#include "SceneFusion.h"
#include "sfNullProperty.h"
#include "sfObjectMap.h"
#include "Consts.h"
#include "sfLoader.h"
#include "sfUtils.h"

#include <UnrealType.h>
#include <EnumProperty.h>
#include <TextProperty.h>
#include <CoreRedirects.h>

#define LOG_CHANNEL "sfPropertyUtil"

std::unordered_map<int, sfPropertyUtil::TypeHandler> sfPropertyUtil::m_typeHandlers;
TMap<FScriptMap*, TSharedPtr<FScriptMapHelper>> sfPropertyUtil::m_staleMaps;
TMap<FScriptSet*, TSharedPtr<FScriptSetHelper>> sfPropertyUtil::m_staleSets;
TSet<TPair<UObject*, UProperty*>> sfPropertyUtil::m_serverChangedProperties;
TSet<TPair<UObject*, UProperty*>> sfPropertyUtil::m_localChangedProperties;
FDelegateHandle sfPropertyUtil::m_onPropertyChangeHandle;
TSet<TPair<FName, FName>> sfPropertyUtil::m_forceSyncList;
TMap<FName, sfPropertyUtil::PropertyChangeHandler> sfPropertyUtil::m_classNameToPropertyChangeHandler;
TSet<FName> sfPropertyUtil::m_syncDefaultOnlyList;
sfPropertyUtil::OnGetAssetPropertyEvent sfPropertyUtil::m_onGetAssetProperty;

using namespace KS;

sfValueProperty::SPtr sfPropertyUtil::FromString(const FString& value)
{
    if (SceneFusion::Service->Session() == nullptr)
    {
        KS::Log::Error("Cannot convert string to property; session is nullptr", LOG_CHANNEL);
        return sfValueProperty::Create(0);
    }
    std::string str = TCHAR_TO_UTF8(*value);
    uint32_t id = SceneFusion::Service->Session()->GetStringTableId(str);
    return sfValueProperty::Create(id);
}

FString sfPropertyUtil::ToString(sfProperty::SPtr propertyPtr)
{
    if (SceneFusion::Service->Session() == nullptr)
    {
        KS::Log::Error("Cannot convert property to string; session is nullptr", LOG_CHANNEL);
        return "";
    }
    if (propertyPtr == nullptr || propertyPtr->Type() != sfProperty::VALUE)
    {
        return "";
    }
    sfValueProperty::SPtr valuePtr = propertyPtr->AsValue();
    if (valuePtr->GetValue().GetType() == ksMultiType::STRING)
    {
        std::string str = valuePtr->GetValue();
        return FString(UTF8_TO_TCHAR(str.c_str()));
    }
    uint32_t id = valuePtr->GetValue();
    sfName name = SceneFusion::Service->Session()->GetStringFromTable(id);
    return FString(UTF8_TO_TCHAR(name->c_str()));
}

sfUPropertyInstance sfPropertyUtil::FindUProperty(UObject* uobjPtr, sfProperty::SPtr propPtr)
{
    if (uobjPtr == nullptr || propPtr == nullptr)
    {
        return sfUPropertyInstance();
    }
    // Push property and its ancestors into a stack, so we can then iterate them from the top down
    // We don't need to push the root dictionary into the stack
    std::stack<sfProperty::SPtr> stack;
    while (propPtr->GetDepth() > 0)
    {
        stack.push(propPtr);
        propPtr = propPtr->GetParentProperty();
    }
    UProperty* upropPtr = nullptr;
    void* ptr = nullptr;// pointer to UProperty instance data
    TSharedPtr<FScriptMapHelper> mapPtr = nullptr;
    TSharedPtr<FScriptSetHelper> setPtr = nullptr;
    // Traverse properties from the top down, finding the UProperty at each level until we reach the one we want or
    // don't find what we expect.
    while (stack.size() > 0)
    {
        propPtr = stack.top();
        stack.pop();
        if (upropPtr == nullptr)
        {
            // Get the first property from the object
            upropPtr = uobjPtr->GetClass()->FindPropertyByName(FName(UTF8_TO_TCHAR(propPtr->Key()->c_str())));
            if (upropPtr == nullptr)
            {
                break;
            }
            ptr = upropPtr->ContainerPtrToValuePtr<void>(uobjPtr);
            continue;
        }
        if (!GetStructField(propPtr->Key(), upropPtr, ptr) &&
            !GetArrayElement(propPtr->Index(), upropPtr, ptr) &&
            !GetMapElement(propPtr->Index(), upropPtr, ptr, mapPtr, stack) &&
            !GetSetElement(propPtr->Index(), upropPtr, ptr, setPtr))
        {
            // We were expecting the UProperty to be one of the above container types but it was not. Abort.
            upropPtr = nullptr;
            break;
        }
        if (upropPtr == nullptr)
        {
            // We did not find the field or element we were looking for. Abort.
            break;
        }
    }
    if (upropPtr == nullptr)
    {
        KS::Log::Warning("Could not find property " + propPtr->GetPath() + " on " +
            std::string(TCHAR_TO_UTF8(*uobjPtr->GetClass()->GetName())), LOG_CHANNEL);
        return sfUPropertyInstance();
    }
    return sfUPropertyInstance(upropPtr, ptr, mapPtr, setPtr);
}

bool sfPropertyUtil::GetStructField(const sfName& name, UProperty*& upropPtr, void*& ptr)
{
    UStructProperty* structPropPtr = Cast<UStructProperty>(upropPtr);
    if (structPropPtr == nullptr)
    {
        return false;
    }
    if (!name.IsValid())
    {
        upropPtr = nullptr;
        return true;
    }
    upropPtr = structPropPtr->Struct->FindPropertyByName(FName(UTF8_TO_TCHAR(name->c_str())));
    if (upropPtr != nullptr)
    {
        ptr = upropPtr->ContainerPtrToValuePtr<void>(ptr);
    }
    return true;
}

bool sfPropertyUtil::GetArrayElement(int index, UProperty*& upropPtr, void*& ptr)
{
    UArrayProperty* arrayPropPtr = Cast<UArrayProperty>(upropPtr);
    if (arrayPropPtr == nullptr)
    {
        return false;
    }
    FScriptArrayHelper array(arrayPropPtr, ptr);
    if (index < 0 || index >= array.Num())
    {
        upropPtr = nullptr;
    }
    else
    {
        upropPtr = arrayPropPtr->Inner;
        ptr = array.GetRawPtr(index);
    }
    return true;
}

bool sfPropertyUtil::GetMapElement(
    int index,
    UProperty*& upropPtr,
    void*& ptr,
    TSharedPtr<FScriptMapHelper>& outMapPtr,
    std::stack<sfProperty::SPtr>& propertyStack)
{
    UMapProperty* mapPropPtr = Cast<UMapProperty>(upropPtr);
    if (mapPropPtr == nullptr)
    {
        return false;
    }
    // Because maps are serialized as lists of key values, we expect at least one more property in the stack
    if (propertyStack.size() <= 0)
    {
        upropPtr = nullptr;
        return true;
    }
    outMapPtr = MakeShareable(new FScriptMapHelper(mapPropPtr, ptr));
    if (index < 0 || index >= outMapPtr->Num())
    {
        upropPtr = nullptr;
        return true;
    }
    int sparseIndex = -1;
    while (index >= 0)
    {
        sparseIndex++;
        if (sparseIndex >= outMapPtr->GetMaxIndex())
        {
            upropPtr = nullptr;
            return true;
        }
        if (outMapPtr->IsValidIndex(sparseIndex))
        {
            index--;
        }
    }
    // Get the next property in the stack, and check its index to determine if we want the map key or value.
    sfProperty::SPtr propPtr = propertyStack.top();
    propertyStack.pop();
    if (propPtr->Index() == 0)
    {
        upropPtr = mapPropPtr->KeyProp;
        ptr = outMapPtr->GetKeyPtr(sparseIndex);
    }
    else if (propPtr->Index() == 1)
    {
        upropPtr = mapPropPtr->ValueProp;
        ptr = outMapPtr->GetValuePtr(sparseIndex);
        outMapPtr = nullptr;
    }
    else
    {
        upropPtr = nullptr;
    }
    return true;
}

bool sfPropertyUtil::GetSetElement(
    int index,
    UProperty*& upropPtr,
    void*& ptr,
    TSharedPtr<FScriptSetHelper>& outSetPtr)
{
    USetProperty* setPropPtr = Cast<USetProperty>(upropPtr);
    if (setPropPtr == nullptr)
    {
        return false;
    }
    outSetPtr = MakeShareable(new FScriptSetHelper(setPropPtr, ptr));
    if (index < 0 || index >= outSetPtr->Num())
    {
        upropPtr = nullptr;
        return true;
    }
    int sparseIndex = -1;
    while (index >= 0)
    {
        sparseIndex++;
        if (sparseIndex >= outSetPtr->GetMaxIndex())
        {
            upropPtr = nullptr;
            return true;
        }
        if (outSetPtr->IsValidIndex(sparseIndex))
        {
            index--;
        }
    }
    upropPtr = setPropPtr->ElementProp;
    ptr = outSetPtr->GetElementPtr(sparseIndex);
    return true;
}

sfProperty::SPtr sfPropertyUtil::GetValue(UObject* uobjPtr, UProperty* upropPtr)
{
    if (uobjPtr == nullptr || upropPtr == nullptr)
    {
        return nullptr;
    }
    if (m_typeHandlers.size() == 0)
    {
        Initialize();
    }
    auto iter = m_typeHandlers.find(upropPtr->GetClass()->GetFName().GetComparisonIndex());
    return iter == m_typeHandlers.end() ? nullptr : iter->second.Get(sfUPropertyInstance(upropPtr,
        upropPtr->ContainerPtrToValuePtr<void>(uobjPtr)));
}

bool sfPropertyUtil::SetValue(UObject* uobjPtr, const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr)
{
    if (!upropInstance.IsValid() || propPtr == nullptr)
    {
        return false;
    }
    if (m_typeHandlers.size() == 0)
    {
        Initialize();
    }
    auto iter = m_typeHandlers.find(upropInstance.Property()->GetClass()->GetFName().GetComparisonIndex());
    if (iter != m_typeHandlers.end() && iter->second.Set(upropInstance, propPtr))
    {
        MarkHashStale(upropInstance);
        MarkPropertyChanged(uobjPtr, upropInstance.Property(), propPtr);
        return true;
    }
    return false;
}

bool sfPropertyUtil::IsDefaultValue(UObject* uobjPtr, UProperty* upropPtr)
{
    if (uobjPtr == nullptr || uobjPtr == uobjPtr->GetClass()->GetDefaultObject() || upropPtr == nullptr)
    {
        return false;
    }
    if (m_typeHandlers.size() == 0)
    {
        Initialize();
    }
    if (m_typeHandlers.find(upropPtr->GetClass()->GetFName().GetComparisonIndex()) != m_typeHandlers.end())
    {
        return upropPtr->Identical_InContainer(uobjPtr, GetDefaultObject(uobjPtr));
    }
    return false;
}

void sfPropertyUtil::SetToDefaultValue(UObject* uobjPtr, UProperty* upropPtr)
{
    if (uobjPtr == nullptr || upropPtr == nullptr)
    {
        return;
    }
    if (m_typeHandlers.size() == 0)
    {
        Initialize();
    }
    if (m_typeHandlers.find(upropPtr->GetClass()->GetFName().GetComparisonIndex()) == m_typeHandlers.end())
    {
        return;
    }
    UObject* defaultObjPtr = GetDefaultObject(uobjPtr);
    if (!upropPtr->Identical_InContainer(uobjPtr, defaultObjPtr))
    {
        upropPtr->CopyCompleteValue_InContainer(uobjPtr, defaultObjPtr);
        MarkPropertyChanged(uobjPtr, upropPtr);
    }
}

UObject* sfPropertyUtil::GetDefaultObject(UObject* uobjPtr)
{
    // First try get the default sub object from the object's outer
    UObject* defaultObjPtr = nullptr;
    if (uobjPtr->GetOuter() != nullptr)
    {
        defaultObjPtr = uobjPtr->GetOuter()->GetClass()->GetDefaultObject()->GetDefaultSubobjectByName(
            uobjPtr->GetFName());
    }
    // If that fails, get the class default object
    if (defaultObjPtr == nullptr)
    {
        defaultObjPtr = uobjPtr->GetClass()->GetDefaultObject();
    }
    return defaultObjPtr;
}

void sfPropertyUtil::CreateProperties(
    UObject* uobjPtr,
    sfDictionaryProperty::SPtr dictPtr,
    const TSet<FString>* const blacklistPtr)
{
    if (uobjPtr == nullptr || dictPtr == nullptr)
    {
        return;
    }

    for (TFieldIterator<UProperty> iter(uobjPtr->GetClass()); iter; ++iter)
    {
        if (IsSyncable(uobjPtr, *iter) && !IsDefaultValue(uobjPtr, *iter))
        {
            FString propertyName = iter->GetName();
            if (blacklistPtr != nullptr && blacklistPtr->Contains(propertyName))
            {
                continue;
            }

            sfProperty::SPtr propPtr = GetValue(uobjPtr, *iter);
            if (propPtr != nullptr)
            {
                std::string name = std::string(TCHAR_TO_UTF8(*propertyName));
                dictPtr->Set(name, propPtr);
            }
        }
    }
}

void sfPropertyUtil::ApplyProperties(
    UObject* uobjPtr,
    sfDictionaryProperty::SPtr dictPtr,
    const TSet<FString>* const blacklistPtr)
{
    if (uobjPtr == nullptr || dictPtr == nullptr)
    {
        return;
    }
    for (TFieldIterator<UProperty> iter(uobjPtr->GetClass()); iter; ++iter)
    {
        if (IsSyncable(uobjPtr, *iter))
        {
            FString propertyName = iter->GetName();
            if (blacklistPtr != nullptr && blacklistPtr->Contains(propertyName))
            {
                continue;
            }

            std::string name = std::string(TCHAR_TO_UTF8(*propertyName));
            sfProperty::SPtr propPtr;
            if (!dictPtr->TryGet(name, propPtr))
            {
                SetToDefaultValue(uobjPtr, *iter);
            }
            else
            {
                SetValue(uobjPtr, sfUPropertyInstance(*iter, iter->ContainerPtrToValuePtr<void>(uobjPtr)), propPtr);
            }
        }
    }
}

void sfPropertyUtil::SendPropertyChanges(
    UObject* uobjPtr,
    sfDictionaryProperty::SPtr dictPtr,
    const TSet<FString>* const blacklistPtr)
{
    if (uobjPtr == nullptr || dictPtr == nullptr)
    {
        return;
    }
    for (TFieldIterator<UProperty> iter(uobjPtr->GetClass()); iter; ++iter)
    {
        if (IsSyncable(uobjPtr, *iter))
        {
            FString propertyName = iter->GetName();
            if (blacklistPtr != nullptr && blacklistPtr->Contains(propertyName))
            {
                continue;
            }

            if (IsDefaultValue(uobjPtr, *iter))
            {
                std::string name = std::string(TCHAR_TO_UTF8(*propertyName));
                dictPtr->Remove(name);
            }
            else
            {
                sfProperty::SPtr propPtr = GetValue(uobjPtr, *iter);
                if (propPtr == nullptr)
                {
                    continue;
                }

                std::string name = std::string(TCHAR_TO_UTF8(*propertyName));
                sfProperty::SPtr oldPropPtr = nullptr;
                if (!dictPtr->TryGet(name, oldPropPtr) || !Copy(oldPropPtr, propPtr))
                {
                    dictPtr->Set(name, propPtr);
                }
            }
        }
    }
}

void sfPropertyUtil::SetReferences(UObject* uobjPtr, const std::vector<sfReferenceProperty::SPtr>& references)
{
    for (const sfReferenceProperty::SPtr& referencePtr : references)
    {
        UObject* referencingObjectPtr = sfObjectMap::GetUObject(referencePtr->GetContainerObject());
        if (referencingObjectPtr == nullptr)
        {
            continue;
        }
        sfUPropertyInstance upropInstance = sfPropertyUtil::FindUProperty(referencingObjectPtr, referencePtr);
        if (upropInstance.IsValid())
        {
            UObjectProperty* objPropPtr = Cast<UObjectProperty>(upropInstance.Property());
            if (objPropPtr != nullptr)
            {
                objPropPtr->SetObjectPropertyValue(upropInstance.Data(), uobjPtr);
                MarkHashStale(upropInstance);
                MarkPropertyChanged(referencingObjectPtr, upropInstance.Property(), referencePtr);
            }
            else
            {
                KS::Log::Warning("Expected " + referencePtr->GetPath() + " on " +
                    std::string(TCHAR_TO_UTF8(*referencingObjectPtr->GetName())) +
                    " to be UObjectProperty but found " +
                    std::string(TCHAR_TO_UTF8(*upropInstance.Property()->GetClass()->GetName())), LOG_CHANNEL);
            }
        }
    }
}

bool sfPropertyUtil::Copy(sfProperty::SPtr destPtr, sfProperty::SPtr srcPtr)
{
    if (destPtr == nullptr || srcPtr == nullptr || destPtr->Type() != srcPtr->Type())
    {
        return false;
    }
    switch (destPtr->Type())
    {
        case sfProperty::VALUE:
        {
            if (!destPtr->Equals(srcPtr))
            {
                destPtr->AsValue()->SetValue(srcPtr->AsValue()->GetValue());
            }
            break;
        }
        case sfProperty::REFERENCE:
        {
            if (!destPtr->Equals(srcPtr))
            {
                destPtr->AsReference()->SetObjectId(srcPtr->AsReference()->GetObjectId());
            }
            break;
        }
        case sfProperty::LIST:
        {
            CopyList(destPtr->AsList(), srcPtr->AsList());
            break;
        }
        case sfProperty::DICTIONARY:
        {
            CopyDict(destPtr->AsDict(), srcPtr->AsDict());
            break;
        }
    }
    return true;
}

void sfPropertyUtil::MarkHashStale(const sfUPropertyInstance& upropInstance)
{
    if (upropInstance.ContainerMap().IsValid())
    {
        m_staleMaps.Add(upropInstance.ContainerMap()->Map, upropInstance.ContainerMap());
    }
    if (upropInstance.ContainerSet().IsValid())
    {
        m_staleSets.Add(upropInstance.ContainerSet()->Set, upropInstance.ContainerSet());
    }
}

void sfPropertyUtil::RehashProperties()
{
    for (TPair<FScriptMap*, TSharedPtr<FScriptMapHelper>>& pair : m_staleMaps)
    {
        pair.Value->Rehash();
    }
    m_staleMaps.Empty();
    for (TPair<FScriptSet*, TSharedPtr<FScriptSetHelper>>& pair : m_staleSets)
    {
        pair.Value->Rehash();
    }
    m_staleSets.Empty();
}

void sfPropertyUtil::MarkPropertyChanged(UObject* uobjPtr, UProperty* upropPtr, sfProperty::SPtr propPtr)
{
    if (uobjPtr == nullptr)
    {
        return;
    }
    if (propPtr != nullptr)
    {
        // Get the root uproperty by the name of the sfproperty at depth 1
        int depth = propPtr->GetDepth();
        if (depth > 1)
        {
            do
            {
                propPtr = propPtr->GetParentProperty();
                depth--;
            } while (depth > 1);
            upropPtr = uobjPtr->GetClass()->FindPropertyByName(FName(UTF8_TO_TCHAR(propPtr->Key()->c_str())));
        }
    }
    if (upropPtr == nullptr)
    {
        return;
    }
    // Workaround for a bug where moving a level will move all the level actors twice.
    // This bug was happening because actor transform changes got applied first, and then when we move the level,
    // Unreal will move the level actors again with an offset. This bug will be fixed if make actor transform relative
    // to the level.
    if (uobjPtr->GetClass()->GetFName() == "WorldTileDetails")
    {
        DisablePropertyChangeHandler();
        FPropertyChangedEvent propertyEvent(upropPtr);
        uobjPtr->PostEditChangeProperty(propertyEvent);
        EnablePropertyChangeHandler();
        return;
    }
    bool alreadyInSet = false;
    m_serverChangedProperties.Emplace(TPair<UObject*, UProperty*>(uobjPtr, upropPtr), &alreadyInSet);
    if (alreadyInSet)
    {
        return;
    }
    AActor* actorPtr = Cast<AActor>(uobjPtr->GetOuter());
    if (actorPtr != nullptr)
    {
        m_serverChangedProperties.Emplace(TPair<UObject*, UProperty*>(actorPtr, nullptr));
    }
}

void sfPropertyUtil::BroadcastChangeEvents()
{
    if (m_serverChangedProperties.Num() <= 0)
    {
        return;
    }
    DisablePropertyChangeHandler();
    for (TPair<UObject*, UProperty*>& pair : m_serverChangedProperties)
    {
        // Check if the uobject is in the sfobjectmap to ensure it is a valid pointer
        if (sfObjectMap::Contains(pair.Key))
        {
            FPropertyChangedEvent propertyEvent(pair.Value);
            AActor* actorPtr = Cast<AActor>(pair.Key);
            bool oldActorSeamlessTraveled = false;
            if (actorPtr != nullptr)
            {
                oldActorSeamlessTraveled = actorPtr->bActorSeamlessTraveled;

                // Calling PostEditChangeProperty triggers blueprint actors to be reconstructed and will trigger an
                // assertion because the cached transform is different from the real one.
                // Setting bActorSeamlessTraveled to true will prevent this from happening.
                actorPtr->bActorSeamlessTraveled = true;
            }
            pair.Key->PostEditChangeProperty(propertyEvent);
            if (actorPtr != nullptr)
            {
                actorPtr->bActorSeamlessTraveled = oldActorSeamlessTraveled;
            }
        }
    }
    m_serverChangedProperties.Empty();
    EnablePropertyChangeHandler();
}

void sfPropertyUtil::AddPropertyToForceSyncList(FName ownerClassName, FName propertyName)
{
    // Register extra properties to sync
    m_forceSyncList.Add(TPair<FName, FName>(ownerClassName, propertyName));
}

void sfPropertyUtil::IgnoreDisableEditOnInstanceFlagForClass(FName className)
{
    m_syncDefaultOnlyList.Add(className);
}

void sfPropertyUtil::EnablePropertyChangeHandler()
{
    m_onPropertyChangeHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddStatic(
        &sfPropertyUtil::OnUPropertyChange);
}

void sfPropertyUtil::DisablePropertyChangeHandler()
{
    FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(m_onPropertyChangeHandle);
    m_onPropertyChangeHandle.Reset();
}

bool sfPropertyUtil::ListeningForPropertyChanges()
{
    return m_onPropertyChangeHandle.IsValid();
}

void sfPropertyUtil::SyncProperties()
{
    for (auto& iter : m_localChangedProperties)
    {
        // uobjPtr may be an invalid pointer, in which case it will not have an sfObject. Do not try dereferencing
        // until we see if it has an sfObject.
        UObject* uobjPtr = iter.Key;
        sfObject::SPtr objPtr = sfObjectMap::GetSFObject(uobjPtr);
        if (uobjPtr->IsPendingKill())
        {
            continue;
        }

        PropertyChangeHandler handler = m_classNameToPropertyChangeHandler.FindRef(uobjPtr->GetClass()->GetFName());
        if (handler != nullptr)
        {
            handler(uobjPtr, iter.Value);
        }
        else if (objPtr != nullptr)
        {
            SyncProperty(objPtr, uobjPtr, iter.Value);
        }
    }
    m_localChangedProperties.Empty();
}

void sfPropertyUtil::SyncProperty(sfObject::SPtr objPtr, UObject* uobjPtr, const FName& name, bool applyServerValue)
{
    if (uobjPtr == nullptr)
    {
        return;
    }
    UProperty* upropPtr = uobjPtr->GetClass()->FindPropertyByName(name);
    if (upropPtr == nullptr)
    {
        KS::Log::Warning("Could not find property " + std::string(TCHAR_TO_UTF8(*name.ToString())) + " on " +
            std::string(TCHAR_TO_UTF8(*uobjPtr->GetClass()->GetName())), LOG_CHANNEL);
    }
    else
    {
        SyncProperty(objPtr, uobjPtr, upropPtr, applyServerValue);
    }
}

void sfPropertyUtil::SyncProperty(sfObject::SPtr objPtr, UObject* uobjPtr, UProperty* upropPtr, bool applyServerValue)
{
    if (upropPtr == nullptr || objPtr == nullptr ||
        objPtr->Property()->Type() != sfProperty::DICTIONARY ||
        SceneFusion::ObjectEventDispatcher->OnUPropertyChange(objPtr, uobjPtr, upropPtr))
    {
        return;
    }

    sfDictionaryProperty::SPtr propertiesPtr = objPtr->Property()->AsDict();
    FString path = upropPtr->GetName();
    std::string name = std::string(TCHAR_TO_UTF8(*path));

    if (objPtr->IsLocked() || applyServerValue)
    {
        sfProperty::SPtr propPtr;
        if (propertiesPtr->TryGet(name, propPtr))
        {
            SetValue(uobjPtr, FindUProperty(uobjPtr, propPtr), propPtr);
        }
        else
        {
            SetToDefaultValue(uobjPtr, upropPtr);
        }
    }
    else if (IsDefaultValue(uobjPtr, upropPtr))
    {
        propertiesPtr->Remove(name);
    }
    else
    {
        sfProperty::SPtr propPtr = GetValue(uobjPtr, upropPtr);
        sfProperty::SPtr oldPropPtr;
        if (propPtr == nullptr)
        {
            FString str = upropPtr->GetClass()->GetName() + " is not supported by Scene Fusion. Changes to " +
                upropPtr->GetName() + " will not sync.";
            KS::Log::Warning(TCHAR_TO_UTF8(*str));
        }
        else if (!propertiesPtr->TryGet(name, oldPropPtr) || !Copy(oldPropPtr, propPtr))
        {
            propertiesPtr->Set(name, propPtr);
        }
    }
}

void sfPropertyUtil::CleanUp()
{
    RehashProperties();
    BroadcastChangeEvents();
    m_localChangedProperties.Empty();
}

// private functions

void sfPropertyUtil::Initialize()
{
    CreateTypeHandler<UBoolProperty>();
    CreateTypeHandler<UFloatProperty>();
    CreateTypeHandler<UIntProperty>();
    CreateTypeHandler<UUInt32Property>();
    CreateTypeHandler<UByteProperty>();
    CreateTypeHandler<UInt64Property>();

    CreateTypeHandler<UInt8Property, uint8_t>();
    CreateTypeHandler<UInt16Property, int>();
    CreateTypeHandler<UUInt16Property, int>();
    CreateTypeHandler<UUInt64Property, int64_t>();

    CreateTypeHandler(UDoubleProperty::StaticClass(), &GetDouble, &SetDouble);
    CreateTypeHandler(UStrProperty::StaticClass(), &GetFString, &SetFString);
    CreateTypeHandler(UTextProperty::StaticClass(), &GetFText, &SetFText);
    CreateTypeHandler(UNameProperty::StaticClass(), &GetFName, &SetFName);
    CreateTypeHandler(UEnumProperty::StaticClass(), &GetEnum, &SetEnum);
    CreateTypeHandler(UArrayProperty::StaticClass(), &GetArray, &SetArray);
    CreateTypeHandler(UMapProperty::StaticClass(), &GetMap, &SetMap);
    CreateTypeHandler(USetProperty::StaticClass(), &GetSet, &SetSet);
    CreateTypeHandler(UStructProperty::StaticClass(), &GetStruct, &SetStruct);
    CreateTypeHandler(UObjectProperty::StaticClass(), &GetObject, &SetObject);
    CreateTypeHandler(USoftObjectProperty::StaticClass(), &GetSoftObject, &SetSoftObject);
    CreateTypeHandler(UClassProperty::StaticClass(), &GetClass, &SetClass);
    CreateTypeHandler(USoftClassProperty::StaticClass(), &GetSoftClass, &SetSoftClass);
}

void sfPropertyUtil::CreateTypeHandler(UClass* typePtr, TypeHandler::Getter getter, TypeHandler::Setter setter)
{
    int key = typePtr->GetFName().GetComparisonIndex();
    if (m_typeHandlers.find(key) != m_typeHandlers.end())
    {
        KS::Log::Warning("Duplicate handler for type " + std::string(TCHAR_TO_UTF8(*typePtr->GetName())), LOG_CHANNEL);
    }
    m_typeHandlers.emplace(key, TypeHandler(getter, setter));
}

bool sfPropertyUtil::IsSyncable(UObject* uobjPtr, UProperty* upropPtr)
{
    if (IsPropertyInForceSyncList(upropPtr))
    {
        return true;
    }

    uint64_t flags = upropPtr->PropertyFlags;
    return flags & CPF_Edit &&
        (!(flags & CPF_DisableEditOnInstance) || m_syncDefaultOnlyList.Contains(uobjPtr->GetClass()->GetFName()))
        && !(flags & CPF_EditConst);
}

bool sfPropertyUtil::IsPropertyInForceSyncList(UProperty* upropPtr)
{
    UClass* ownerClassPtr = upropPtr->GetOwnerClass();
    if (ownerClassPtr == nullptr)
    {
        return false;
    }
    return m_forceSyncList.Contains(TPair<FName, FName>(ownerClassPtr->GetFName(), upropPtr->GetFName()));
}

void sfPropertyUtil::OnUPropertyChange(UObject* uobjPtr, FPropertyChangedEvent& ev)
{
    // Objects in the transient package are non-saved objects we don't sync unless we registered a special handler
    // for a given type in the m_classNameToPropertyChangeHandler map. If we don't do this check here we will
    // get invalid pointers when transient objects created while merging levels are garbage collected.
    if (ev.MemberProperty == nullptr || 
        (uobjPtr->GetOutermost() == GetTransientPackage() &&
            !m_classNameToPropertyChangeHandler.Contains(uobjPtr->GetClass()->GetFName())))
    {
        return;
    }
    if (IsSyncable(uobjPtr, ev.MemberProperty))
    {
        // Sliding values in the details panel can generate nearly 1000 change events per second, so to throttle the
        // update rate we queue the property to be processed at most once per tick.
        m_localChangedProperties.Add(TPair<UObject*, UProperty*>(uobjPtr, ev.MemberProperty));
    }
}

sfProperty::SPtr sfPropertyUtil::GetDouble(const sfUPropertyInstance& upropInstance)
{
    return sfValueProperty::Create(ksMultiType(ksMultiType::BYTE_ARRAY, (uint8_t*)upropInstance.Data(), sizeof(double),
        sizeof(double)));
}

bool sfPropertyUtil::SetDouble(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr)
{
    const ksMultiType& value = propPtr->AsValue()->GetValue();
    if (value.GetData().size() != sizeof(double))
    {
        KS::Log::Error("Error setting double property " + 
            std::string(TCHAR_TO_UTF8(*upropInstance.Property()->GetName())) +
            ". Expected " + std::to_string(sizeof(double)) + " bytes, but got " +
            std::to_string(value.GetData().size()) + ".", LOG_CHANNEL);
        return false;
    }
    if (std::memcmp((uint8_t*)upropInstance.Data(), value.GetData().data(), sizeof(double)) != 0)
    {
        std::memcpy((uint8_t*)upropInstance.Data(), value.GetData().data(), sizeof(double));
        return true;
    }
    return false;
}

sfProperty::SPtr sfPropertyUtil::GetFString(const sfUPropertyInstance& upropInstance)
{
    return FromString(*(FString*)upropInstance.Data());
}

bool sfPropertyUtil::SetFString(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr)
{
    FString* strPtr = (FString*)upropInstance.Data();
    FString newValue = ToString(propPtr);
    if (!strPtr->Equals(newValue))
    {
        *strPtr = newValue;
        return true;
    }
    return false;
}

sfProperty::SPtr sfPropertyUtil::GetFText(const sfUPropertyInstance& upropInstance)
{
    return FromString(((FText*)upropInstance.Data())->ToString());
}

bool sfPropertyUtil::SetFText(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr)
{
    FText* textPtr = (FText*)upropInstance.Data();
    FString newValue = ToString(propPtr);
    if (!textPtr->ToString().Equals(newValue))
    {
        *textPtr = FText::FromString(newValue);
        return true;
    }
    return false;
}

sfProperty::SPtr sfPropertyUtil::GetFName(const sfUPropertyInstance& upropInstance)
{
    return FromString(((FName*)upropInstance.Data())->ToString());
}

bool sfPropertyUtil::SetFName(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr)
{
    FName* namePtr = (FName*)upropInstance.Data();
    FName newValue = *ToString(propPtr);
    if (*namePtr != newValue)
    {
        *namePtr = newValue;
        return true;
    }
    return false;
}

sfProperty::SPtr sfPropertyUtil::GetEnum(const sfUPropertyInstance& upropInstance)
{
    UEnumProperty* tPtr = Cast<UEnumProperty>(upropInstance.Property());
    int64_t value = tPtr->GetUnderlyingProperty()->GetSignedIntPropertyValue(upropInstance.Data());
    if (value >= 0 && value < 256)
    {
        return sfValueProperty::Create((uint8_t)value);
    }
    return sfValueProperty::Create(value);
}

bool sfPropertyUtil::SetEnum(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr)
{
    UEnumProperty* tPtr = Cast<UEnumProperty>(upropInstance.Property());
    int64_t value = propPtr->AsValue()->GetValue();
    if (tPtr->GetUnderlyingProperty()->GetSignedIntPropertyValue(upropInstance.Data()) != value)
    {
        tPtr->GetUnderlyingProperty()->SetIntPropertyValue(upropInstance.Data(), value);
        return true;
    }
    return false;
}

sfProperty::SPtr sfPropertyUtil::GetArray(const sfUPropertyInstance& upropInstance)
{
    UArrayProperty* tPtr = Cast<UArrayProperty>(upropInstance.Property());
    auto iter = m_typeHandlers.find(tPtr->Inner->GetClass()->GetFName().GetComparisonIndex());
    if (iter == m_typeHandlers.end())
    {
        return nullptr;
    }
    sfListProperty::SPtr listPtr = sfListProperty::Create();
    FScriptArrayHelper array(tPtr, upropInstance.Data());
    for (int i = 0; i < array.Num(); i++)
    {

        sfProperty::SPtr elementPtr = iter->second.Get(sfUPropertyInstance(tPtr->Inner, (void*)array.GetRawPtr(i)));
        if (elementPtr == nullptr)
        {
            return nullptr;
        }
        listPtr->Add(elementPtr);
    }
    return listPtr;
}

bool sfPropertyUtil::SetArray(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr)
{
    UArrayProperty* tPtr = Cast<UArrayProperty>(upropInstance.Property());
    auto iter = m_typeHandlers.find(tPtr->Inner->GetClass()->GetFName().GetComparisonIndex());
    if (iter == m_typeHandlers.end())
    {
        return false;
    }
    bool changed = false;
    sfListProperty::SPtr listPtr = propPtr->AsList();
    FScriptArrayHelper array(tPtr, upropInstance.Data());
    if (array.Num() != listPtr->Size())
    {
        array.Resize(listPtr->Size());
        changed = true;
    }
    for (int i = 0; i < listPtr->Size(); i++)
    {
        if (iter->second.Set(sfUPropertyInstance(tPtr->Inner, (void*)array.GetRawPtr(i)), listPtr->Get(i)))
        {
            changed = true;
        }
    }
    return changed;
}

sfProperty::SPtr sfPropertyUtil::GetMap(const sfUPropertyInstance& upropInstance)
{
    UMapProperty* tPtr = Cast<UMapProperty>(upropInstance.Property());
    auto keyIter = m_typeHandlers.find(tPtr->KeyProp->GetClass()->GetFName().GetComparisonIndex());
    if (keyIter == m_typeHandlers.end())
    {
        return nullptr;
    }
    auto valueIter = m_typeHandlers.find(tPtr->ValueProp->GetClass()->GetFName().GetComparisonIndex());
    if (valueIter == m_typeHandlers.end())
    {
        return nullptr;
    }
    sfListProperty::SPtr listPtr = sfListProperty::Create();
    FScriptMapHelper map(tPtr, upropInstance.Data());
    for (int i = 0; i < map.GetMaxIndex(); i++)
    {
        if (!map.IsValidIndex(i))
        {
            continue;
        }
        sfListProperty::SPtr pairPtr = sfListProperty::Create();
        sfProperty::SPtr keyPtr = keyIter->second.Get(sfUPropertyInstance(tPtr->KeyProp, (void*)map.GetKeyPtr(i)));
        if (keyPtr == nullptr)
        {
            return nullptr;
        }
        sfProperty::SPtr valuePtr = valueIter->second.Get(
            sfUPropertyInstance(tPtr->ValueProp, (void*)map.GetValuePtr(i)));
        if (valuePtr == nullptr)
        {
            return nullptr;
        }
        pairPtr->Add(keyPtr);
        pairPtr->Add(valuePtr);
        listPtr->Add(pairPtr);
    }
    return listPtr;
}

bool sfPropertyUtil::SetMap(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr)
{
    UMapProperty* tPtr = Cast<UMapProperty>(upropInstance.Property());
    auto keyIter = m_typeHandlers.find(tPtr->KeyProp->GetClass()->GetFName().GetComparisonIndex());
    if (keyIter == m_typeHandlers.end())
    {
        return false;
    }
    auto valueIter = m_typeHandlers.find(tPtr->ValueProp->GetClass()->GetFName().GetComparisonIndex());
    if (valueIter == m_typeHandlers.end())
    {
        return false;
    }
    bool changed = false;
    bool changedKey = false;
    sfListProperty::SPtr listPtr = propPtr->AsList();
    FScriptMapHelper map(tPtr, upropInstance.Data());
    if (map.Num() != listPtr->Size())
    {
        changed = true;
        changedKey = true;
        map.EmptyValues(listPtr->Size());
    }
    for (int i = 0; i < listPtr->Size(); i++)
    {
        if (map.Num() < listPtr->Size())
        {
            map.AddDefaultValue_Invalid_NeedsRehash();
        }
        sfListProperty::SPtr pairPtr = listPtr->Get(i)->AsList();
        if (keyIter->second.Set(sfUPropertyInstance(tPtr->KeyProp, (void*)map.GetKeyPtr(i)), pairPtr->Get(0)))
        {
            changed = true;
            changedKey = true;
        }
        if (valueIter->second.Set(sfUPropertyInstance(tPtr->ValueProp, (void*)map.GetValuePtr(i)), pairPtr->Get(1)))
        {
            changed = true;
        }
    }
    if (changedKey)
    {
        map.Rehash();
    }
    return changed;
}

sfProperty::SPtr sfPropertyUtil::GetSet(const sfUPropertyInstance& upropInstance)
{
    USetProperty* tPtr = Cast<USetProperty>(upropInstance.Property());
    auto iter = m_typeHandlers.find(tPtr->ElementProp->GetClass()->GetFName().GetComparisonIndex());
    if (iter == m_typeHandlers.end())
    {
        return nullptr;
    }
    sfListProperty::SPtr listPtr = sfListProperty::Create();
    FScriptSetHelper set(tPtr, upropInstance.Data());
    for (int i = 0; i < set.GetMaxIndex(); i++)
    {
        if (!set.IsValidIndex(i))
        {
            continue;
        }
        sfProperty::SPtr elementPtr = iter->second.Get(
            sfUPropertyInstance(tPtr->ElementProp, (void*)set.GetElementPtr(i)));
        if (elementPtr == nullptr)
        {
            return nullptr;
        }
        listPtr->Add(elementPtr);
    }
    return listPtr;
}

bool sfPropertyUtil::SetSet(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr)
{
    USetProperty* tPtr = Cast<USetProperty>(upropInstance.Property());
    auto iter = m_typeHandlers.find(tPtr->ElementProp->GetClass()->GetFName().GetComparisonIndex());
    if (iter == m_typeHandlers.end())
    {
        return false;
    }
    bool changed = false;
    sfListProperty::SPtr listPtr = propPtr->AsList();
    FScriptSetHelper set(tPtr, upropInstance.Data());
    if (set.Num() != listPtr->Size())
    {
        changed = true;
        set.EmptyElements(listPtr->Size());
    }
    for (int i = 0; i < listPtr->Size(); i++)
    {
        if (set.Num() < listPtr->Size())
        {
            set.AddDefaultValue_Invalid_NeedsRehash();
        }
        if (iter->second.Set(sfUPropertyInstance(tPtr->ElementProp, (void*)set.GetElementPtr(i)), listPtr->Get(i)))
        {
            changed = true;
        }
    }
    if (changed)
    {
        set.Rehash();
    }
    return changed;
}

sfProperty::SPtr sfPropertyUtil::GetStruct(const sfUPropertyInstance& upropInstance)
{
    UStructProperty* tPtr = Cast<UStructProperty>(upropInstance.Property());
    sfDictionaryProperty::SPtr dictPtr = sfDictionaryProperty::Create();
    UField* fieldPtr = tPtr->Struct->Children;
    while (fieldPtr)
    {
        UProperty* subPropPtr = Cast<UProperty>(fieldPtr);
        if (subPropPtr != nullptr)
        {
            auto iter = m_typeHandlers.find(subPropPtr->GetClass()->GetFName().GetComparisonIndex());
            if (iter != m_typeHandlers.end())
            {
                sfProperty::SPtr valuePtr = iter->second.Get(
                    sfUPropertyInstance(subPropPtr, subPropPtr->ContainerPtrToValuePtr<void>(upropInstance.Data())));
                if (valuePtr != nullptr)
                {
                    std::string name = TCHAR_TO_UTF8(*subPropPtr->GetName());
                    dictPtr->Set(name, valuePtr);
                }
            }
        }
        fieldPtr = fieldPtr->Next;
    }
    return dictPtr;
}

bool sfPropertyUtil::SetStruct(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr)
{
    UStructProperty* tPtr = Cast<UStructProperty>(upropInstance.Property());
    sfDictionaryProperty::SPtr dictPtr = propPtr->AsDict();
    UField* fieldPtr = tPtr->Struct->Children;
    bool changed = false;
    while (fieldPtr)
    {
        UProperty* subPropPtr = Cast<UProperty>(fieldPtr);
        if (subPropPtr != nullptr)
        {
            auto iter = m_typeHandlers.find(subPropPtr->GetClass()->GetFName().GetComparisonIndex());
            if (iter != m_typeHandlers.end())
            {
                std::string name = TCHAR_TO_UTF8(*subPropPtr->GetName());
                sfProperty::SPtr valuePtr;
                if (dictPtr->TryGet(name, valuePtr) && iter->second.Set(sfUPropertyInstance(subPropPtr,
                    subPropPtr->ContainerPtrToValuePtr<void>(upropInstance.Data())), valuePtr))
                {
                    changed = true;
                }
            }
        }
        fieldPtr = fieldPtr->Next;
    }
    return changed;
}

sfProperty::SPtr sfPropertyUtil::GetObject(const sfUPropertyInstance& upropInstance)
{
    UObjectProperty* tPtr = Cast<UObjectProperty>(upropInstance.Property());
    UObject* referencePtr = tPtr->GetObjectPropertyValue(upropInstance.Data());
    if (referencePtr == nullptr)
    {
        return sfNullProperty::Create();
    }
    if (referencePtr->IsPendingKill())
    {
        // The object is deleted. Clear the reference.
        tPtr->SetObjectPropertyValue(upropInstance.Data(), nullptr);
        return sfNullProperty::Create();
    }
    return CreatePropertyForObjectReference(upropInstance, referencePtr);
}

bool sfPropertyUtil::SetObject(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr)
{
    UObjectProperty* tPtr = Cast<UObjectProperty>(upropInstance.Property());
    UObject* oldPtr = tPtr->GetObjectPropertyValue(upropInstance.Data());
    if (propPtr->Type() == sfProperty::NUL)
    {
        if (oldPtr == nullptr)
        {
            return false;
        }
        tPtr->SetObjectPropertyValue(upropInstance.Data(), nullptr);
        return true;
    }
    if (propPtr->Type() == sfProperty::REFERENCE)
    {
        // The object is in the level.
        uint32_t objId = propPtr->AsReference()->GetObjectId();
        sfObject::SPtr objPtr = SceneFusion::Service->Session()->GetObject(objId);
        UObject* referencePtr = sfObjectMap::GetUObject(objPtr);
        if (referencePtr != oldPtr)
        {
            tPtr->SetObjectPropertyValue(upropInstance.Data(), referencePtr);
            return true;
        }
        return false;
    }
    // The object is an asset
    FString str = ToString(propPtr);
    // If str is empty we keep our current value
    if (str.IsEmpty())
    {
        return false;
    }

    FString path, className;
    if (!str.Split(";", &className, &path))
    {
        KS::Log::Warning("Invalid asset string: " + std::string(TCHAR_TO_UTF8(*str)), LOG_CHANNEL);
        return false;
    }

    UObject* assetPtr = sfLoader::Get().LoadFromCache(path);
    if (assetPtr == nullptr || !assetPtr->IsA(tPtr->PropertyClass))
    {
        if (sfLoader::Get().IsUserIdle())
        {
            assetPtr = sfLoader::Get().Load(path, className);
        }
        else
        {
            sfLoader::Get().LoadWhenIdle(propPtr);
        }
    }
    if (assetPtr != nullptr && assetPtr != oldPtr)
    {
        tPtr->SetObjectPropertyValue(upropInstance.Data(), assetPtr);
        return true;
    }
    return false;
}

sfProperty::SPtr sfPropertyUtil::GetSoftObject(const sfUPropertyInstance& upropInstance)
{
    FSoftObjectPtr& softObjectPtr = *(FSoftObjectPtr*)upropInstance.Data();
    if (softObjectPtr.IsNull())
    {
        return sfNullProperty::Create();
    }
    UObject* referencePtr = softObjectPtr.Get();
    if (referencePtr != nullptr && !referencePtr->IsPendingKill())
    {
        return CreatePropertyForObjectReference(upropInstance, referencePtr);
    }
    // The object isn't loaded. Get the class name from the asset registry.
    FAssetData asset = UAssetManager::Get().GetAssetRegistry().GetAssetByObjectPath(*softObjectPtr.ToString());
    if (!asset.IsValid())
    {
        KS::Log::Warning("Invalid soft asset path: " + std::string(TCHAR_TO_UTF8(*softObjectPtr.ToString())),
            LOG_CHANNEL);
        return sfNullProperty::Create();
    }
    UClass* classPtr = asset.GetClass();
    if (classPtr == nullptr)
    {
        // The class isn't loaded. We have to load the object to get the class.
        GIsSlowTask = true;
        UObject* assetPtr = LoadObject<UObject>(nullptr, *asset.ObjectPath.ToString());
        GIsSlowTask = false;
        if (assetPtr == nullptr)
        {
            KS::Log::Warning("Unable to load soft asset " + std::string(TCHAR_TO_UTF8(*softObjectPtr.ToString())),
                LOG_CHANNEL);
            return FromString(";" + asset.ObjectPath.ToString());
        }
        classPtr = assetPtr->GetClass();
    }
    return FromString(sfUtils::ClassToFString(classPtr) + ";" + asset.ObjectPath.ToString());
}

bool sfPropertyUtil::SetSoftObject(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr)
{
    FSoftObjectPtr& softObjectPtr = *(FSoftObjectPtr*)upropInstance.Data();
    if (propPtr->Type() == sfProperty::NUL)
    {
        softObjectPtr = nullptr;
        return true;
    }
    if (propPtr->Type() == sfProperty::REFERENCE)
    {
        // The object is in the level.
        uint32_t objId = propPtr->AsReference()->GetObjectId();
        sfObject::SPtr objPtr = SceneFusion::Service->Session()->GetObject(objId);
        UObject* referencePtr = sfObjectMap::GetUObject(objPtr);
        if (referencePtr != softObjectPtr.Get())
        {
            softObjectPtr = FSoftObjectPath(referencePtr);
            return true;
        }
        return false;
    }
    // The object is an asset
    FString str = ToString(propPtr);
    // If str is empty we keep our current value
    if (str.IsEmpty())
    {
        return false;
    }
    FString path, className;
    if (!str.Split(";", &className, &path))
    {
        KS::Log::Warning("Invalid asset string: " + std::string(TCHAR_TO_UTF8(*str)), LOG_CHANNEL);
        return false;
    }
    if (softObjectPtr.ToString() == path)
    {
        return false;
    }
    if (!UAssetManager::Get().GetAssetRegistry().GetAssetByObjectPath(*path).IsValid())
    {
        // The asset is missing. Loading it will create a stand-in.
        UObject* standInPtr = sfLoader::Get().Load(path, className);
        if (standInPtr != softObjectPtr.Get())
        {
            softObjectPtr = FSoftObjectPath(standInPtr);
            return true;
        }
        return false;
    }
    softObjectPtr = FSoftObjectPath(path);
    return true;
}

sfProperty::SPtr sfPropertyUtil::CreatePropertyForObjectReference(
    const sfUPropertyInstance& upropInstance,
    UObject* referencePtr)
{
    if (referencePtr->GetTypedOuter<ULevel>() != nullptr)
    {
        // The object is in the level.
        sfObject::SPtr objPtr = sfObjectMap::GetSFObject(referencePtr);
        if (objPtr == nullptr)
        {
            sfName type;
            if (referencePtr->IsA<AActor>())
            {
                type = sfType::Actor;
            }
            else if (referencePtr->IsA<UActorComponent>())
            {
                type = sfType::Component;
            }
            else
            {
                // Empty string means keep your current value
                return sfValueProperty::Create("");
            }
            objPtr = sfObject::Create(type, sfDictionaryProperty::Create());
            sfObjectMap::Add(objPtr, referencePtr);
        }
        return sfReferenceProperty::Create(objPtr->Id());
    }

    // The object is an asset
    FString str;
    if (referencePtr->HasAllFlags(RF_Transient))
    {
        // This is a stand-in for a missing asset.
        str = sfLoader::Get().GetPathFromStandIn(referencePtr);
        // Try to load the asset from memory
        FString path, className;
        if (str.Split(";", &className, &path))
        {
            UObject* assetPtr = sfLoader::Get().LoadFromCache(path);
            if (assetPtr != nullptr)
            {
                // Replace the stand-in with the correct asset
                if (UObjectProperty* tPtr = Cast<UObjectProperty>(upropInstance.Property()))
                {
                    tPtr->SetObjectPropertyValue(upropInstance.Data(), assetPtr);
                }
                else if (USoftObjectProperty* tPtr = Cast<USoftObjectProperty>(upropInstance.Property()))
                {
                    tPtr->SetObjectPropertyValue(upropInstance.Data(), assetPtr);
                }
            }
        }
        else
        {
            KS::Log::Warning("Reference to transient object " + std::string(TCHAR_TO_UTF8(*referencePtr->GetName())) +
                " will not sync.", LOG_CHANNEL);
        }
    }
    else
    {
        str = sfUtils::ClassToFString(referencePtr->GetClass()) + ";" + referencePtr->GetPathName();
        m_onGetAssetProperty.Broadcast(referencePtr);
    }
    return FromString(str);
}

sfProperty::SPtr sfPropertyUtil::GetClass(const sfUPropertyInstance& upropInstance)
{
    UClassProperty* tPtr = Cast<UClassProperty>(upropInstance.Property());
    UObject* classPtr = tPtr->GetObjectPropertyValue(upropInstance.Data());
    if (classPtr == nullptr)
    {
        return sfNullProperty::Create();
    }
    return FromString(sfUtils::ClassToFString(Cast<UClass>(classPtr)));
}

bool sfPropertyUtil::SetClass(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr)
{
    UObjectProperty* tPtr = Cast<UObjectProperty>(upropInstance.Property());
    UObject* oldPtr = tPtr->GetObjectPropertyValue(upropInstance.Data());
    if (propPtr->Type() == sfProperty::NUL)
    {
        if (oldPtr == nullptr)
        {
            return false;
        }
        tPtr->SetObjectPropertyValue(upropInstance.Data(), nullptr);
        return true;
    }
    UClass* classPtr = sfUtils::LoadClass(ToString(propPtr));
    if (oldPtr == classPtr)
    {
        return false;
    }
    tPtr->SetObjectPropertyValue(upropInstance.Data(), classPtr);
    return true;
}

sfProperty::SPtr sfPropertyUtil::GetSoftClass(const sfUPropertyInstance& upropInstance)
{
    TSoftClassPtr<UObject>& softClassPtr = *(TSoftClassPtr<UObject>*)upropInstance.Data();
    if (softClassPtr == nullptr)
    {
        return sfNullProperty::Create();
    }
    return FromString(softClassPtr.ToString());
}

bool sfPropertyUtil::SetSoftClass(const sfUPropertyInstance& upropInstance, sfProperty::SPtr propPtr)
{
    TSoftClassPtr<UObject>& softClassPtr = *(TSoftClassPtr<UObject>*)upropInstance.Data();
    if (propPtr->Type() == sfProperty::NUL)
    {
        softClassPtr = nullptr;
    }
    else
    {
        softClassPtr = FSoftObjectPath(ToString(propPtr));
    }
    return true;
}

// Compares the src list values in lock step with the dest list values. When there is a discrepancy we first check for
// an element removal (Current src value = Next dest value). Next we check for an element insertion (Next src value =
// Current dest value). Finally if neither of the above cases were found, we replace the current dest value with the
// current src value.
void sfPropertyUtil::CopyList(sfListProperty::SPtr destPtr, sfListProperty::SPtr srcPtr)
{
    std::vector<sfProperty::SPtr> toAdd;
    for (int i = 0; i < srcPtr->Size(); i++)
    {
        sfProperty::SPtr elementPtr = srcPtr->Get(i);
        if (destPtr->Size() <= i)
        {
            toAdd.push_back(elementPtr);
            continue;
        }
        if (elementPtr->Equals(destPtr->Get(i)))
        {
            continue;
        }
        // if the current src element matches the next next element, remove the current dest element.
        if (destPtr->Size() > i + 1 && elementPtr->Equals(destPtr->Get(i + 1)))
        {
            destPtr->Remove(i);
            continue;
        }
        // if the current dest element matches the next src element, insert the current src element.
        if (srcPtr->Size() > i + 1 && destPtr->Get(i)->Equals(srcPtr->Get(i + 1)))
        {
            destPtr->Insert(i, elementPtr);
            i++;
            continue;
        }
        if (!Copy(destPtr->Get(i), elementPtr))
        {
            destPtr->Set(i, elementPtr);
        }
    }
    if (toAdd.size() > 0)
    {
        destPtr->AddRange(toAdd);
    }
    else if (destPtr->Size() > srcPtr->Size())
    {
        destPtr->Resize(srcPtr->Size());
    }
}

void sfPropertyUtil::CopyDict(sfDictionaryProperty::SPtr destPtr, sfDictionaryProperty::SPtr srcPtr)
{
    std::vector<sfName> toRemove;
    for (const auto& iter : *destPtr)
    {
        if (!srcPtr->HasKey(iter.first))
        {
            toRemove.push_back(iter.second->Key());
        }
    }
    for (const sfName key : toRemove)
    {
        destPtr->Remove(key);
    }
    for (const auto& iter : *srcPtr)
    {
        sfProperty::SPtr destPropPtr;
        if (!destPtr->TryGet(iter.first, destPropPtr) || !Copy(destPropPtr, iter.second))
        {
            destPtr->Set(iter.first, iter.second);
        }
    }
}

void sfPropertyUtil::RegisterPropertyChangeHandlerForClass(FName className, PropertyChangeHandler handler)
{
    m_classNameToPropertyChangeHandler.Add(className, handler);
}

void sfPropertyUtil::UnregisterPropertyChangeHandlerForClass(FName className)
{
    m_classNameToPropertyChangeHandler.Remove(className);
}

#undef LOG_CHANNEL