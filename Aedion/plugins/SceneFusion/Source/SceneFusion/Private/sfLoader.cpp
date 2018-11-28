#include "sfLoader.h"
#include "sfPropertyUtil.h"
#include "sfObjectMap.h"
#include "sfConfig.h"
#include "sfUtils.h"
#include "consts.h"

#include <Editor.h>
#include <Log.h>
#include <sfPropertyIterator.h>
#include <sfConfig.h>
#include <ConstructorHelpers.h>
#include <CoreRedirects.h>
#include <SkeletalMeshRenderData.h>
#include <Engine/SkeletalMesh.h>
#include <../Plugins/2D/Paper2D/Source/Paper2D/Classes/PaperSprite.h>
#include <Developer/RawMesh/Public/RawMesh.h>

#define LOG_CHANNEL "sfLoader"

TSharedPtr<sfLoader> sfLoader::m_instancePtr = nullptr;

sfLoader& sfLoader::Get()
{
    if (!m_instancePtr.IsValid())
    {
        m_instancePtr = MakeShareable(new sfLoader);
    }
    return *m_instancePtr;
}

sfLoader::sfLoader() :
    m_replaceTimer{ 0.0f },
    m_isMouseDown{ false },
    m_overrideIdle{ false }
{
    m_standInPaths.Add(USkeletalMesh::StaticClass(), "/SceneFusion/StandIns/Skeletal");
    m_standInPaths.Add(UMaterial::StaticClass(), "/SceneFusion/StandIns/Material");
    m_standInPaths.Add(UTexture::StaticClass(), "/SceneFusion/StandIns/Question");
    m_standInPaths.Add(UPaperSprite::StaticClass(), "/SceneFusion/StandIns/Sprite");
}

void sfLoader::Start()
{
    FSlateApplication::Get().RegisterInputPreProcessor(m_instancePtr);
    m_onNewAssetHandle = UAssetManager::Get().GetAssetRegistry().OnAssetAdded().AddRaw(this, &sfLoader::OnNewAsset);
}

void sfLoader::Stop()
{
    m_isMouseDown = false;
    m_delayedAssets.clear();
    for (auto iter : m_standIns)
    {
        iter.Value->ClearFlags(RF_Standalone);// Allow Unreal to destroy the stand-ins
    }
    m_standIns.Empty();
    m_standInsToReplace.Empty();
    FSlateApplication::Get().UnregisterInputPreProcessor(m_instancePtr);
    UAssetManager::Get().GetAssetRegistry().OnAssetAdded().Remove(m_onNewAssetHandle);
}

void sfLoader::RegisterStandInGenerator(UClass* classPtr, TSharedPtr<sfIStandInGenerator> generatorPtr)
{
    m_standInGenerators.Add(classPtr, generatorPtr);
}

bool sfLoader::IsUserIdle()
{
    return m_overrideIdle || (!m_isMouseDown &&
        FSlateApplication::Get().GetCurrentTime() - FSlateApplication::Get().GetLastUserInteractionTime() > 
        sfConfig::Get().IdleTime);
}

void sfLoader::LoadWhenIdle(sfProperty::SPtr propPtr)
{
    std::vector<sfProperty::SPtr>& properties = m_delayedAssets[propPtr->GetContainerObject()];
    if (std::find(properties.begin(), properties.end(), propPtr) == properties.end())
    {
        properties.emplace_back(propPtr);
    }
}

void sfLoader::LoadAssetsFor(sfObject::SPtr objPtr)
{
    auto iter = m_delayedAssets.find(objPtr);
    if (iter != m_delayedAssets.end())
    {
        m_overrideIdle = true;
        for (sfProperty::SPtr propPtr : iter->second)
        {
            LoadProperty(propPtr);
        }
        m_overrideIdle = false;
        m_delayedAssets.erase(iter);
    }
    for (sfObject::SPtr childPtr : objPtr->Children())
    {
        if (childPtr->Type() == sfType::Component)
        {
            LoadAssetsFor(childPtr);
        }
    }
}

UObject* sfLoader::Load(const FString& path, const FString& className)
{
    UObject* assetPtr = LoadObject<UObject>(nullptr, *path);
    if (assetPtr != nullptr)
    {
        return assetPtr;
    }

    // Try to find a stand-in
    assetPtr = m_standIns.FindRef(path);
    if (assetPtr != nullptr)
    {
        return assetPtr;
    }

    UClass* classPtr = sfUtils::LoadClass(className);
    if (classPtr == nullptr)
    {
        return nullptr;
    }
    FString str = "Could not find " + className + " '" + path + "'. Creating stand-in object.";
    KS::Log::Warning(TCHAR_TO_UTF8(*str), LOG_CHANNEL);

    // Names containing '.'s don't deserialize properly so replace "." with "+"
    // All delimiters were chosen because they work with soft pointer copy/paste. If you change anything, make sure
    // copy/paste still works with all pointer types.
    FString name = "Missing_" + className  + "+" + path.Replace(*FString("."), *FString("+"));

    // Public makes copy/paste work. Standalone prevents it from being garbage collected.
    EObjectFlags flags = RF_Public | RF_Standalone | RF_Transient;
    UObject* standInPtr = nullptr;
    FString* pathPtr = m_standInPaths.Find(classPtr);
    if (pathPtr != nullptr)
    {
        UObject* standInAssetPtr = LoadObject<UObject>(nullptr, **pathPtr);
        if (standInAssetPtr != nullptr)
        {
            standInPtr = DuplicateObject(standInAssetPtr, GEditor->GetEditorWorldContext().World(), *name);
            standInPtr->SetFlags(flags);
        }
    }
    if (standInPtr == nullptr)
    {
        standInPtr = NewObject<UObject>(GEditor->GetEditorWorldContext().World(), classPtr, *name, flags);
    }
    
    TSharedPtr<sfIStandInGenerator> generatorPtr;
    generatorPtr = m_standInGenerators.FindRef(classPtr);
    if (generatorPtr.IsValid())
    {
        generatorPtr->Generate(path, standInPtr);
    }

    m_standIns.Add(path, standInPtr);

    return standInPtr;
}

FString sfLoader::GetPathFromStandIn(UObject* standInPtr)
{
    FString name = standInPtr->GetName();
    if (!name.StartsWith("Missing_"))
    {
        return "";
    }
    // Remove "Missing_"
    FString path = name.RightChop(8);
    // We had to replace ";" and "." with "+" earlier for Unreal deserialization to work. The first occurence is always
    // a ";" and the rest are "."
    int index;
    if (path.FindChar('+', index))
    {
        path[index] = ';';
        path.ReplaceInline(*FString("+"), *FString("."));
    }
    return path;
}

// This code is mostly copied from Unreal's ResolveName function and modified to not load the asset if it's not already
// in memory, and to fit our style.
UObject* sfLoader::LoadFromCache(FString path)
{
    UObject* assetPtr = m_standIns.FindRef(path);
    if (assetPtr != nullptr)
    {
        return assetPtr;
    }

    // Strip off the object class.
    ConstructorHelpers::StripObjectClass(path);

    path = FPackageName::GetDelegateResolvedPackagePath(path);
    bool isSubobjectPath = false;
    int dotIndex = INDEX_NONE;

    // to make parsing the name easier, replace the subobject delimiter with an extra dot
    path.ReplaceInline(SUBOBJECT_DELIMITER, TEXT(".."), ESearchCase::CaseSensitive);
    while ((dotIndex = path.Find(TEXT("."), ESearchCase::CaseSensitive)) != INDEX_NONE)
    {
        FString partialName = path.Left(dotIndex);

        // if the next part of path ends in two dots, it indicates that the next object in the path name
        // is not a top-level object (i.e. it's a subobject).  e.g. SomePackage.SomeGroup.SomeObject..Subobject
        if (path.IsValidIndex(dotIndex + 1) && path[dotIndex + 1] == TEXT('.'))
        {
            path.RemoveAt(dotIndex, 1, false);
            isSubobjectPath = true;
        }

        FName* scriptPackageName = nullptr;
        if (!isSubobjectPath)
        {
            // In case this is a short script package name, convert to long name before passing to
            // CreatePackage/FindObject.
            scriptPackageName = FPackageName::FindScriptPackageName(*partialName);
            if (scriptPackageName)
            {
                partialName = scriptPackageName->ToString();
            }
        }

        // Process any package redirects before calling CreatePackage/FindObject
        {
            const FCoreRedirectObjectName newPackageName = FCoreRedirects::GetRedirectedName(
                ECoreRedirectFlags::Type_Package, FCoreRedirectObjectName(NAME_None, NAME_None, *partialName));
            partialName = newPackageName.PackageName.ToString();
        }

        // Only long package names are allowed so don't even attempt to create one because whatever the name represents
        // it's not a valid package name anyway.

        if (isSubobjectPath)
        {
            UObject* newPackage = FindObject<UPackage>(assetPtr, *partialName);
            if (!newPackage)
            {
                newPackage = FindObject<UObject>(assetPtr == NULL ? ANY_PACKAGE : assetPtr, *partialName);
                if (!newPackage)
                {
                    return nullptr;
                }
            }
            assetPtr = newPackage;
        }
        else if (!FPackageName::IsShortPackageName(partialName))
        {
            // Try to find the package in memory
            assetPtr = StaticFindObjectFast(UPackage::StaticClass(), assetPtr, *partialName);
            if (!assetPtr)
            {
                return nullptr;
            }
        }
        path.RemoveAt(0, dotIndex + 1, false);
    }

    if (assetPtr == nullptr)
    {
        return nullptr;
    }
    assetPtr = StaticFindObjectFast(UObject::StaticClass(), assetPtr, *path);
    if (assetPtr && assetPtr->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad | RF_NeedPostLoadSubobjects | RF_WillBeLoaded))
    {
        // Object needs loading
        return nullptr;
    }
    return assetPtr;
}

void sfLoader::LoadProperty(sfProperty::SPtr propPtr)
{
    sfObject::SPtr objPtr = propPtr->GetContainerObject();
    if (objPtr == nullptr)
    {
        // This happens if the object was deleted or the property was changed before we loaded it.
        return;
    }
    UObject* uobjPtr = sfObjectMap::GetUObject(objPtr);
    if (uobjPtr == nullptr)
    {
        return;
    }
    sfUPropertyInstance uprop = sfPropertyUtil::FindUProperty(uobjPtr, propPtr);
    if (uprop.IsValid())
    {
        // This will load the asset
        sfPropertyUtil::SetValue(uobjPtr, uprop, propPtr);
    }
}

void sfLoader::OnNewAsset(const FAssetData& assetData)
{
    UObject* standInPtr;
    FString path = assetData.ObjectPath.ToString();
    if (!m_standIns.RemoveAndCopyValue(path, standInPtr))
    {
        return;
    }
    KS::Log::Debug("New asset found for stand-in '" + std::string(TCHAR_TO_UTF8(*path)) + "'.", LOG_CHANNEL);
    standInPtr->ClearFlags(RF_Standalone);// Allow Unreal to destroy the stand-in 
    m_standInsToReplace.Add(standInPtr);
    // Wait .1 seconds to see if more assets are created that we can swap in all at once.
    m_replaceTimer = 0.1f;
}

void sfLoader::ReplaceStandIns()
{
    if (m_standInsToReplace.Num() == 0 || SceneFusion::Service->Session() == nullptr)
    {
        return;
    }
    TSet<uint32_t> pathIds;
    for (UObject* standInPtr : m_standInsToReplace)
    {
        pathIds.Add(SceneFusion::Service->Session()->GetStringTableId(TCHAR_TO_UTF8(*GetPathFromStandIn(standInPtr))));
    }
    int count = 0;
    for (auto iter = sfObjectMap::Begin(); iter != sfObjectMap::End(); ++iter)
    {
        for (auto propIter = iter->first->Property()->Iterate(); propIter.Value() != nullptr; propIter.Next())
        {
            if (propIter.Value()->Type() != sfProperty::VALUE)
            {
                continue;
            }
            sfValueProperty::SPtr valuePtr = propIter.Value()->AsValue();
            if (valuePtr->GetValue().GetType() != ksMultiType::UINT)
            {
                continue;
            }
            if (pathIds.Contains(valuePtr->GetValue()))
            {
                sfUPropertyInstance upropInstance = sfPropertyUtil::FindUProperty(iter->second, valuePtr);
                if (sfPropertyUtil::SetValue(iter->second, upropInstance, valuePtr))
                {
                    count++;
                }
            }
        }
    }
    m_standInsToReplace.Empty();
    KS::Log::Debug("Replaced " + std::to_string(count) + " stand-in reference(s).", LOG_CHANNEL);
}

void sfLoader::LoadDelayedAssets()
{
    if (m_delayedAssets.size() == 0)
    {
        return;
    }
    for (auto iter : m_delayedAssets)
    {
        for (sfProperty::SPtr propPtr : iter.second)
        {
            LoadProperty(propPtr);
        }
    }
    m_delayedAssets.clear();
}

void sfLoader::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
    if (m_replaceTimer <= 0)
    {
        ReplaceStandIns();
    }
    else
    {
        m_replaceTimer -= DeltaTime;
    }

    if (IsUserIdle())
    {
        LoadDelayedAssets();
    }
}

bool sfLoader::HandleMouseButtonDownEvent(FSlateApplication& slateApp, const FPointerEvent& mouseEvent)
{
    if (mouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        m_isMouseDown = true;
    }
    return false;
}

bool sfLoader::HandleMouseButtonUpEvent(FSlateApplication& slateApp, const FPointerEvent& mouseEvent)
{
    if (mouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        m_isMouseDown = false;
    }
    return false;
}

#undef LOG_CHANNEL