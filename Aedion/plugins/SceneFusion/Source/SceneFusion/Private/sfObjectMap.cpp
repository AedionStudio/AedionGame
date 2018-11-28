#include  "sfObjectMap.h"
#include "Consts.h"
#include <sfDictionaryProperty.h>
#include <GameFramework/Actor.h>

TMap<const UObject*, sfObject::SPtr> sfObjectMap::m_uToSFObjectMap;
std::unordered_map<sfObject::SPtr, UObject*> sfObjectMap::m_sfToUObjectMap;

bool sfObjectMap::Contains(const UObject* uobjPtr)
{
    return uobjPtr != nullptr && m_uToSFObjectMap.Contains(uobjPtr);
}

bool sfObjectMap::Contains(const sfObject::SPtr objPtr)
{
    return objPtr != nullptr && m_sfToUObjectMap.find(objPtr) != m_sfToUObjectMap.end();
}

sfObject::SPtr sfObjectMap::GetSFObject(const UObject* uobjPtr)
{
    if (uobjPtr == nullptr)
    {
        return nullptr;
    }
    return m_uToSFObjectMap.FindRef(uobjPtr);
}

sfObject::SPtr sfObjectMap::GetOrCreateSFObject(UObject* uobjPtr, const sfName& type)
{
    if (uobjPtr == nullptr)
    {
        return nullptr;
    }
    sfObject::SPtr objPtr = m_uToSFObjectMap.FindRef(uobjPtr);
    if (objPtr != nullptr)
    {
        return objPtr;
    }
    objPtr = sfObject::Create(type, sfDictionaryProperty::Create());
    Add(objPtr, uobjPtr);
    return objPtr;
}

UObject* sfObjectMap::GetUObject(sfObject::SPtr objPtr)
{
    if (objPtr == nullptr)
    {
        return nullptr;
    }
    auto iter = m_sfToUObjectMap.find(objPtr);
    return iter == m_sfToUObjectMap.end() ? nullptr : iter->second;
}

void sfObjectMap::Add(sfObject::SPtr objPtr, UObject* uobjPtr)
{
    if (objPtr == nullptr || uobjPtr == nullptr)
    {
        return;
    }
    m_sfToUObjectMap[objPtr] = uobjPtr;
    m_uToSFObjectMap.Add(uobjPtr, objPtr);
}

sfObject::SPtr sfObjectMap::Remove(const UObject* uobjPtr)
{
    if (uobjPtr == nullptr)
    {
        return nullptr;
    }
    sfObject::SPtr objPtr = nullptr;
    if (m_uToSFObjectMap.RemoveAndCopyValue(uobjPtr, objPtr))
    {
        m_sfToUObjectMap.erase(objPtr);
    }
    return objPtr;
}

UObject* sfObjectMap::Remove(sfObject::SPtr objPtr)
{
    if (objPtr == nullptr)
    {
        return nullptr;
    }
    auto iter = m_sfToUObjectMap.find(objPtr);
    if (iter == m_sfToUObjectMap.end())
    {
        return nullptr;
    }
    UObject* uobjPtr = iter->second;
    m_sfToUObjectMap.erase(iter);
    m_uToSFObjectMap.Remove(uobjPtr);
    return uobjPtr;
}

void sfObjectMap::Clear()
{
    m_uToSFObjectMap.Empty();
    m_sfToUObjectMap.clear();
}

std::unordered_map<sfObject::SPtr, UObject*>::iterator sfObjectMap::Begin()
{
    return m_sfToUObjectMap.begin();
}

std::unordered_map<sfObject::SPtr, UObject*>::iterator sfObjectMap::End()
{
    return m_sfToUObjectMap.end();
}