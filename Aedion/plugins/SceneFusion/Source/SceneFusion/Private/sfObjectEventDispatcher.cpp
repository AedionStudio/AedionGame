#include "sfObjectEventDispatcher.h"
#include "SceneFusion.h"

#define LOG_CHANNEL "sfObjectEventDispatcher"

sfObjectEventDispatcher::SPtr sfObjectEventDispatcher::CreateSPtr()
{
    return std::make_shared<sfObjectEventDispatcher>();
}

sfObjectEventDispatcher::sfObjectEventDispatcher() :
    m_active{ false }
{

}

sfObjectEventDispatcher::~sfObjectEventDispatcher()
{

}

void sfObjectEventDispatcher::Register(const sfName& objectType, TSharedPtr<sfBaseObjectManager> managerPtr)
{
    m_managers[objectType] = managerPtr;
}

void sfObjectEventDispatcher::Initialize()
{
    if (m_active)
    {
        return;
    }
    m_active = true;
    sfSession::SPtr sessionPtr = SceneFusion::Service->Session();
    m_createEventPtr = sessionPtr->RegisterOnCreateHandler([this](sfObject::SPtr objPtr, int childIndex)
    {
        TSharedPtr<sfBaseObjectManager> managerPtr = GetManager(objPtr);
        if (managerPtr.IsValid())
        {
            managerPtr->OnCreate(objPtr, childIndex);
        }
    });
    m_deleteEventPtr = sessionPtr->RegisterOnDeleteHandler([this](sfObject::SPtr objPtr)
    {
        TSharedPtr<sfBaseObjectManager> managerPtr = GetManager(objPtr);
        if (managerPtr.IsValid())
        {
            managerPtr->OnDelete(objPtr);
        }
    });
    m_lockEventPtr = sessionPtr->RegisterOnLockHandler([this](sfObject::SPtr objPtr)
    {
        TSharedPtr<sfBaseObjectManager> managerPtr = GetManager(objPtr);
        if (managerPtr.IsValid())
        {
            managerPtr->OnLock(objPtr);
        }
    });
    m_unlockEventPtr = sessionPtr->RegisterOnUnlockHandler([this](sfObject::SPtr objPtr)
    {
        TSharedPtr<sfBaseObjectManager> managerPtr = GetManager(objPtr);
        if (managerPtr.IsValid())
        {
            managerPtr->OnUnlock(objPtr);
        }
    });
    m_lockOwnerChangeEventPtr = sessionPtr->RegisterOnLockOwnerChangeHandler([this](sfObject::SPtr objPtr)
    {
        TSharedPtr<sfBaseObjectManager> managerPtr = GetManager(objPtr);
        if (managerPtr.IsValid())
        {
            managerPtr->OnLockOwnerChange(objPtr);
        }
    });
    m_directLockChangeEventPtr = sessionPtr->RegisterOnDirectLockChangeHandler([this](sfObject::SPtr objPtr)
    {
        TSharedPtr<sfBaseObjectManager> managerPtr = GetManager(objPtr);
        if (managerPtr.IsValid())
        {
            managerPtr->OnDirectLockChange(objPtr);
        }
    });
    m_parentChangeEventPtr = sessionPtr->RegisterOnParentChangeHandler([this](sfObject::SPtr objPtr, int childIndex)
    {
        TSharedPtr<sfBaseObjectManager> managerPtr = GetManager(objPtr);
        if (managerPtr.IsValid())
        {
            managerPtr->OnParentChange(objPtr, childIndex);
        }
    });
    m_propertyChangeEventPtr = sessionPtr->RegisterOnPropertyChangeHandler(
        [this](sfProperty::SPtr propertyPtr)
    {
        if (propertyPtr->GetContainerObject() == nullptr)
        {
            KS::Log::Error("Container object is null. Property path: " + propertyPtr->GetPath(), LOG_CHANNEL);
            return;
        }
        TSharedPtr<sfBaseObjectManager> managerPtr = GetManager(propertyPtr->GetContainerObject());
        if (managerPtr.IsValid())
        {
            managerPtr->OnPropertyChange(propertyPtr);
        }
    });
    m_removeFieldEventPtr = sessionPtr->RegisterOnDictionaryRemoveHandler(
        [this](sfDictionaryProperty::SPtr dictPtr, sfName name)
    {
        TSharedPtr<sfBaseObjectManager> managerPtr = GetManager(dictPtr->GetContainerObject());
        if (managerPtr.IsValid())
        {
            managerPtr->OnRemoveField(dictPtr, name);
        }
    });
    m_listAddEventPtr = sessionPtr->RegisterOnListAddHandler(
        [this](sfListProperty::SPtr listPtr, int index, int count)
    {
        TSharedPtr<sfBaseObjectManager> managerPtr = GetManager(listPtr->GetContainerObject());
        if (managerPtr.IsValid())
        {
            managerPtr->OnListAdd(listPtr, index, count);
        }
    });
    m_listRemoveEventPtr = sessionPtr->RegisterOnListRemoveHandler(
        [this](sfListProperty::SPtr listPtr, int index, int count)
    {
        TSharedPtr<sfBaseObjectManager> managerPtr = GetManager(listPtr->GetContainerObject());
        if (managerPtr.IsValid())
        {
            managerPtr->OnListRemove(listPtr, index, count);
        }
    });

    for (auto iter : m_managers)
    {
        iter.second->Initialize();
    }
}

void sfObjectEventDispatcher::CleanUp()
{
    if (!m_active)
    {
        return;
    }
    m_active = false;
    sfSession::SPtr sessionPtr = SceneFusion::Service->Session();
    sessionPtr->UnregisterOnCreateHandler(m_createEventPtr);
    sessionPtr->UnregisterOnDeleteHandler(m_deleteEventPtr);
    sessionPtr->UnregisterOnLockHandler(m_lockEventPtr);
    sessionPtr->UnregisterOnUnlockHandler(m_unlockEventPtr);
    sessionPtr->UnregisterOnLockOwnerChangeHandler(m_lockOwnerChangeEventPtr);
    sessionPtr->UnregisterOnDirectLockChangeHandler(m_directLockChangeEventPtr);
    sessionPtr->UnregisterOnParentChangeHandler(m_parentChangeEventPtr);
    sessionPtr->UnregisterOnPropertyChangeHandler(m_propertyChangeEventPtr);
    sessionPtr->UnregisterOnDictionaryRemoveHandler(m_removeFieldEventPtr);
    sessionPtr->UnregisterOnListAddHandler(m_listAddEventPtr);
    sessionPtr->UnregisterOnListRemoveHandler(m_listRemoveEventPtr);

    for (auto iter : m_managers)
    {
        iter.second->CleanUp();
    }
}

bool sfObjectEventDispatcher::OnUPropertyChange(sfObject::SPtr objPtr, UObject* uobjPtr, UProperty* upropPtr)
{
    if (objPtr == nullptr)
    {
        return false;
    }
    TSharedPtr<sfBaseObjectManager> managerPtr = GetManager(objPtr);
    return managerPtr.IsValid() && managerPtr->OnUPropertyChange(objPtr, uobjPtr, upropPtr);
}

void sfObjectEventDispatcher::OnUndoRedo(sfObject::SPtr objPtr, UObject* uobjPtr)
{
    if (objPtr == nullptr)
    {
        // Call OnUndoRedo on all managers until one of them handles it.
        for (auto iter : m_managers)
        {
            if (iter.second->OnUndoRedo(nullptr, uobjPtr))
            {
                return;
            }
        }
    }
    else
    {
        TSharedPtr<sfBaseObjectManager> managerPtr = GetManager(objPtr);
        if (managerPtr.IsValid())
        {
            managerPtr->OnUndoRedo(objPtr, uobjPtr);
        }
    }
}

TSharedPtr<sfBaseObjectManager> sfObjectEventDispatcher::GetManager(sfObject::SPtr objPtr)
{
    auto iter = m_managers.find(objPtr->Type());
    if (iter == m_managers.end())
    {
        KS::Log::Error("Unknown object type '" + *objPtr->Type() + "'.", LOG_CHANNEL);
        return nullptr;
    }
    return iter->second;
}

#undef LOG_CHANNEL