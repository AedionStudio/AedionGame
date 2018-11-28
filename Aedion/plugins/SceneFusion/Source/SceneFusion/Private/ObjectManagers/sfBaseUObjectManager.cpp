#include "sfBaseUObjectManager.h"
#include "../sfObjectMap.h"
#include "../sfPropertyUtil.h"
#include "../SceneFusion.h"
#include "../sfUtils.h"

using namespace KS::SceneFusion2;

void sfBaseUObjectManager::OnPropertyChange(sfProperty::SPtr propertyPtr)
{
    UObject* uobjPtr = GetUObject(propertyPtr->GetContainerObject());
    if (uobjPtr == nullptr)
    {
        return;
    }

    // Get property at depth 1
    int depth = propertyPtr->GetDepth();
    sfProperty::SPtr currentPtr = propertyPtr;
    while (depth > 1)
    {
        currentPtr = currentPtr->GetParentProperty();
        depth--;
    }
    auto handlerIter = m_propertyChangeHandlers.find(currentPtr->Key());
    if (handlerIter != m_propertyChangeHandlers.end())
    {
        // Call property-specific handler
        bool handled = false;
        sfUtils::PreserveUndoStack([handlerIter, uobjPtr, currentPtr, &handled]()
        {
            handled = handlerIter->second(uobjPtr, currentPtr);
        });
        if (handled)
        {
            // Event was handled by the property-specific handler
            return;
        }
    }

    sfUPropertyInstance upropInstance = sfPropertyUtil::FindUProperty(uobjPtr, propertyPtr);
    if (upropInstance.IsValid())
    {
        sfPropertyUtil::SetValue(uobjPtr, upropInstance, propertyPtr);
    }
}

void sfBaseUObjectManager::OnRemoveField(sfDictionaryProperty::SPtr dictPtr, const sfName& name)
{
    UObject* uobjPtr = GetUObject(dictPtr->GetContainerObject());
    if (uobjPtr == nullptr)
    {
        return;
    }

    auto handlerIter = m_propertyChangeHandlers.find(name);
    if (handlerIter != m_propertyChangeHandlers.end())
    {
        // Call property-specific handler
        bool handled = false;
        sfUtils::PreserveUndoStack([handlerIter, uobjPtr, &handled]()
        {
            handled = handlerIter->second(uobjPtr, nullptr);
        });
        if (handled)
        {
            // Event was handled by the property-specific handler
            return;
        }
    }

    UProperty* upropPtr = uobjPtr->GetClass()->FindPropertyByName(FName(UTF8_TO_TCHAR(name->c_str())));
    if (upropPtr != nullptr)
    {
        sfPropertyUtil::SetToDefaultValue(uobjPtr, upropPtr);
    }
}

void sfBaseUObjectManager::OnListAdd(sfListProperty::SPtr listPtr, int index, int count)
{
    UObject* uobjPtr = GetUObject(listPtr->GetContainerObject());
    if (uobjPtr == nullptr)
    {
        return;
    }
    sfUPropertyInstance upropInstance = sfPropertyUtil::FindUProperty(uobjPtr, listPtr);
    if (!upropInstance.IsValid())
    {
        return;
    }
    ArrayInsert(uobjPtr, upropInstance, listPtr, index, count) ||
        SetInsert(uobjPtr, upropInstance, listPtr, index, count) ||
        MapInsert(uobjPtr, upropInstance, listPtr, index, count);
}

bool sfBaseUObjectManager::ArrayInsert(
    UObject* uobjPtr,
    const sfUPropertyInstance& upropInstance,
    sfListProperty::SPtr listPtr,
    int index,
    int count)
{
    UArrayProperty* arrayPropPtr = Cast<UArrayProperty>(upropInstance.Property());
    if (arrayPropPtr == nullptr)
    {
        return false;
    }
    FScriptArrayHelper array(arrayPropPtr, upropInstance.Data());
    array.InsertValues(index, count);
    for (int i = index; i < index + count; i++)
    {
        sfPropertyUtil::SetValue(uobjPtr, sfUPropertyInstance(arrayPropPtr->Inner, array.GetRawPtr(i)),
            listPtr->Get(i));
    }
    return true;
}

bool sfBaseUObjectManager::SetInsert(
    UObject* uobjPtr,
    const sfUPropertyInstance& upropInstance,
    sfListProperty::SPtr listPtr,
    int index,
    int count)
{
    USetProperty* setPropPtr = Cast<USetProperty>(upropInstance.Property());
    if (setPropPtr == nullptr)
    {
        return false;
    }
    TSharedPtr<FScriptSetHelper> setPtr = MakeShareable(new FScriptSetHelper(setPropPtr, upropInstance.Data()));
    int firstInsertIndex = setPtr->GetMaxIndex();
    int lastInsertIndex = 0;
    for (int i = 0; i < count; i++)
    {
        int insertIndex = setPtr->AddDefaultValue_Invalid_NeedsRehash();
        firstInsertIndex = FMath::Min(firstInsertIndex, insertIndex);
        lastInsertIndex = FMath::Max(lastInsertIndex, insertIndex);
    }
    int listIndex = -1;
    for (int i = 0; i < setPtr->GetMaxIndex(); i++)
    {
        if (!setPtr->IsValidIndex(i))
        {
            continue;
        }
        listIndex++;
        if (listIndex < index && i < firstInsertIndex)
        {
            continue;
        }
        sfPropertyUtil::SetValue(uobjPtr, sfUPropertyInstance(setPropPtr->ElementProp, setPtr->GetElementPtr(i)),
            listPtr->Get(listIndex));
        if (listIndex >= index + count - 1 && i >= lastInsertIndex)
        {
            break;
        }
    }
    sfPropertyUtil::MarkHashStale(upropInstance);
    return true;
}

bool sfBaseUObjectManager::MapInsert(
    UObject* uobjPtr,
    const sfUPropertyInstance& upropInstance,
    sfListProperty::SPtr listPtr,
    int index,
    int count)
{
    UMapProperty* mapPropPtr = Cast<UMapProperty>(upropInstance.Property());
    if (mapPropPtr == nullptr)
    {
        return false;
    }
    TSharedPtr<FScriptMapHelper> mapPtr = MakeShareable(new FScriptMapHelper(mapPropPtr, upropInstance.Data()));
    int firstInsertIndex = mapPtr->GetMaxIndex();
    int lastInsertIndex = 0;
    for (int i = 0; i < count; i++)
    {
        int insertIndex = mapPtr->AddDefaultValue_Invalid_NeedsRehash();
        firstInsertIndex = FMath::Min(firstInsertIndex, insertIndex);
        lastInsertIndex = FMath::Max(lastInsertIndex, insertIndex);
    }
    int listIndex = -1;
    for (int i = 0; i < mapPtr->GetMaxIndex(); i++)
    {
        if (!mapPtr->IsValidIndex(i))
        {
            continue;
        }
        listIndex++;
        if (listIndex < index && i < firstInsertIndex)
        {
            continue;
        }
        sfListProperty::SPtr pairPtr = listPtr->Get(listIndex)->AsList();
        sfPropertyUtil::SetValue(uobjPtr, sfUPropertyInstance(mapPropPtr->KeyProp, mapPtr->GetKeyPtr(i)),
            pairPtr->Get(0));
        sfPropertyUtil::SetValue(uobjPtr, sfUPropertyInstance(mapPropPtr->ValueProp, mapPtr->GetValuePtr(i)),
            pairPtr->Get(1));
        if (listIndex >= index + count - 1 && i >= lastInsertIndex)
        {
            break;
        }
    }
    sfPropertyUtil::MarkHashStale(upropInstance);
    return true;
}

void sfBaseUObjectManager::OnListRemove(sfListProperty::SPtr listPtr, int index, int count)
{
    UObject* uobjPtr = GetUObject(listPtr->GetContainerObject());
    if (uobjPtr == nullptr)
    {
        return;
    }
    sfUPropertyInstance upropInstance = sfPropertyUtil::FindUProperty(uobjPtr, listPtr);
    if (!upropInstance.IsValid())
    {
        return;
    }
    ArrayRemove(upropInstance, index, count) || SetRemove(upropInstance, index, count) ||
        MapRemove(upropInstance, index, count);
    sfPropertyUtil::MarkPropertyChanged(uobjPtr, upropInstance.Property(), listPtr);
}

bool sfBaseUObjectManager::ArrayRemove(const sfUPropertyInstance& upropInstance, int index, int count)
{
    UArrayProperty* arrayPropPtr = Cast<UArrayProperty>(upropInstance.Property());
    if (arrayPropPtr == nullptr)
    {
        return false;
    }
    FScriptArrayHelper array(arrayPropPtr, upropInstance.Data());
    array.RemoveValues(index, count);
    return true;
}

bool sfBaseUObjectManager::SetRemove(const sfUPropertyInstance& upropInstance, int index, int count)
{
    USetProperty* setPropPtr = Cast<USetProperty>(upropInstance.Property());
    if (setPropPtr == nullptr)
    {
        return false;
    }
    FScriptSetHelper set(setPropPtr, upropInstance.Data());
    int i = 0;
    for (; i < set.GetMaxIndex(); i++)
    {
        if (set.IsValidIndex(i))
        {
            index--;
            if (index < 0)
            {
                break;
            }
        }
    }
    set.RemoveAt(i, count);
    return true;
}

bool sfBaseUObjectManager::MapRemove(const sfUPropertyInstance& upropInstance, int index, int count)
{
    UMapProperty* mapPropPtr = Cast<UMapProperty>(upropInstance.Property());
    if (mapPropPtr == nullptr)
    {
        return false;
    }
    FScriptMapHelper map(mapPropPtr, upropInstance.Data());
    int i = 0;
    for (; i < map.GetMaxIndex(); i++)
    {
        if (map.IsValidIndex(i))
        {
            index--;
            if (index < 0)
            {
                break;
            }
        }
    }
    map.RemoveAt(i, count);
    return true;
}

UObject* sfBaseUObjectManager::GetUObject(sfObject::SPtr objPtr)
{
    return sfObjectMap::GetUObject(objPtr);
}
