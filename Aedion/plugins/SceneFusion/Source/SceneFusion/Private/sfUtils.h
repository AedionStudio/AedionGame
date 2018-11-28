#pragma once
#include <CoreMinimal.h>
#include <Editor.h>
#include <Editor/TransBuffer.h>
#include <functional>
#include <Log.h>

#define LOG_CHANNEL "sfUtils"

/**
 * Utility functions.
 */
class sfUtils
{
public:
    typedef std::function<void()> Callback;

    /**
     * Calls a delegate then clears any undo transactions that were added during the delegate execution.
     *
     * @param   Callback callback
     */
    static void PreserveUndoStack(Callback callback)
    {
        UTransBuffer* undoBufferPtr = Cast<UTransBuffer>(GEditor->Trans);
        int undoCount = 0;
        int undoNum = 0;
        if (undoBufferPtr != nullptr)
        {
            undoCount = undoBufferPtr->UndoCount;
            undoBufferPtr->UndoCount = 0;
            undoNum = undoBufferPtr->UndoBuffer.Num();
        }
        callback();
        if (undoBufferPtr != nullptr)
        {
            while (undoBufferPtr->UndoBuffer.Num() > undoNum)
            {
                undoBufferPtr->UndoBuffer.Pop();
            }
            undoBufferPtr->UndoCount = undoCount;
        }
    }

    /**
     * Gets name or blueprint path for the given class.
     *
     * @param   UClass* classPtr
     * @return  FString
     */
    static FString ClassToFString(UClass* classPtr)
    {
        if (classPtr->IsInBlueprint())
        {
            // Path to blueprint
            return classPtr->GetOuter()->GetName();
        }
        return classPtr->GetName();
    }

    /**
     * Loads a class by name or blueprint path.
     *
     * @param   const FString* className - name of the class or blueprint path.
     * @param   bool silent - if false, will log a warning if the class was not found.
     * @return  UClass* loaded class, or nullptr if the class was not found.
     */
    static UClass* LoadClass(const FString& className, bool silent = false)
    {
        UClass* classPtr = nullptr;
        if (className.Contains("/"))
        {
            // If it contains a '/' it's a blueprint path
            // Disable loading dialog that causes a crash if we are dragging objects
            GIsSlowTask = true;
            UBlueprint* blueprintPtr = LoadObject<UBlueprint>(nullptr, *className);
            GIsSlowTask = false;
            if (blueprintPtr == nullptr)
            {
                if (!silent)
                {
                    KS::Log::Warning("Unable to load blueprint " + std::string(TCHAR_TO_UTF8(*className)),
                        LOG_CHANNEL);
                }
                return nullptr;
            }
            classPtr = blueprintPtr->GeneratedClass;
        }
        else
        {
            classPtr = FindObject<UClass>(ANY_PACKAGE, *className);
        }
        if (classPtr == nullptr && !silent)
        {
            KS::Log::Warning("Unable to find class " + std::string(TCHAR_TO_UTF8(*className)), LOG_CHANNEL);
        }
        return classPtr;
    }

    /**
     * Renames an object. If the name is not available, appends random digits to the name until it finds a name that is
     * available.
     *
     * @param   UObject* uobjPtr to rename.
     * @param   FString name to set. If this name is already used, random digits will be appended to the name.
     */
    static void Rename(UObject* uobjPtr, FString name)
    {
        while (!uobjPtr->Rename(*name, nullptr, REN_Test))
        {
            name += FString::FromInt(rand() % 10);
        }
        uobjPtr->Rename(*name);
    }

    /**
     * Tries to rename an object. Logs a warning if the object could not be renamed because the name is already in use.
     * If a deleted object is using the name, renames the deleted object to make the name available.
     *
     * @param   UObject* uobjPtr to rename.
     * @param   const FString& name
     */
    static void TryRename(UObject* uobjPtr, const FString& name)
    {
        UObject* currentPtr = StaticFindObjectFast(UObject::StaticClass(), uobjPtr->GetOuter(), FName(*name));
        if (currentPtr == uobjPtr)
        {
            return;
        }
        if (currentPtr != nullptr && currentPtr->IsPendingKill())
        {
            // Rename the deleted actor so we can reuse its name
            Rename(currentPtr, name + " (deleted)");
            currentPtr = nullptr;
        }

        if (currentPtr == nullptr && uobjPtr->Rename(*name, nullptr, REN_Test))
        {
            uobjPtr->Rename(*name);
        }
        else
        {
            KS::Log::Warning("Cannot rename object to " + std::string(TCHAR_TO_UTF8(*name)) +
                " because another object with that name already exists.", LOG_CHANNEL);
        }
    }

    /**
     * Converts FString to std::string.
     *
     * @param   const FString& inString
     * @return  std::string
     */
    static std::string FToStdString(const FString& inString)
    {
        return std::string(TCHAR_TO_UTF8(*inString));
    }

    /**
     * Returns the first widget found of the given type using depth-first search.
     *
     * @param   TSharedRef<SWidget> widgetPtr to search from.
     * @param   FName widgetType to look for.
     * @return  TSharedPtr<SWidget> widget of the given type, or invalid pointer if none was found.
     */
    static TSharedPtr<SWidget> FindWidget(TSharedRef<SWidget> widgetPtr, FName widgetType)
    {
        if (widgetPtr->GetType() == widgetType)
        {
            return widgetPtr;
        }
        FChildren* childrenPtr = widgetPtr->GetChildren();
        if (childrenPtr != nullptr)
        {
            for (int i = 0; i < childrenPtr->Num(); i++)
            {
                TSharedPtr<SWidget> resultPtr = FindWidget(childrenPtr->GetChildAt(i), widgetType);
                if (resultPtr.IsValid())
                {
                    return resultPtr;
                }
            }
        }
        return nullptr;
    }
};

#undef LOG_CHANNEL
