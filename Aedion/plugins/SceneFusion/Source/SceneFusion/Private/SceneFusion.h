#pragma once

#include "ISceneFusion.h"
#include "UI/sfUI.h"
#include "Web/sfBaseWebService.h"
#include "Log.h"
#include "sfService.h"
#include "sfObjectEventDispatcher.h"
#include "sfUndoManager.h"
#include "sfMissingObjectManager.h"
#include "sfSessionInfo.h"
#include "ObjectManagers/sfActorManager.h"
#include "ObjectManagers/sfAvatarManager.h"
#include "ObjectManagers/sfComponentManager.h"
#include "ObjectManagers/sfLevelManager.h"
#include "ObjectManagers/sfMeshStandInManager.h"

#include <CoreMinimal.h>

// Log setup
DECLARE_LOG_CATEGORY_EXTERN(LogSceneFusion, Log, All)

/**
 * Scene Fusion Plugin Module
 */
class SceneFusion : public ISceneFusion
{
public:
    static TSharedPtr<sfBaseWebService> WebService;
    static sfService::SPtr Service;
    static sfObjectEventDispatcher::SPtr ObjectEventDispatcher;
    static TSharedPtr<sfMissingObjectManager> MissingObjectManager;
    static TSharedPtr<sfActorManager> ActorManager;
    static TSharedPtr<sfAvatarManager> AvatarManager;
    static TSharedPtr<sfComponentManager> ComponentManager;
    static TSharedPtr<sfLevelManager> LevelManager;
    static bool IsSessionCreator;

    /**
     * Module entry point
     */
    void StartupModule();
    
    /**
     * Module cleanup
     */
    void ShutdownModule();

    /**
     * Updates the service
     *
     * @param   float deltaTime since the last tick
     * @return  bool true to keep the Tick function registered
     */
    bool Tick(float deltaTime);

    /**
     * Initialize the webservice and associated console commands
     */
    void InitializeWebService();

    /**
     * Writes a log message to Unreal's log system.
     *
     * @param   KS::LogLevel level
     * @param   const char* channel that was logged to.
     * @param   const char* message
     */
    static void HandleLog(KS::LogLevel level, const char* channel, const char* message);

    /**
     * Flags the active viewport to be redrawn during the next update SceneFusion tick.
     */
    static void RedrawActiveViewport();

    /**
     * Gets the lock material for a user. Creates the material if it does not already exist.
     *
     * @param   sfUser::SPtr userPtr to get lock material for. May be nullptr.
     * @return  UMaterialInterface* lock material for the user.
     */
    static UMaterialInterface* GetLockMaterial(sfUser::SPtr userPtr);

    /**
     * Connects to a session.
     *
     * @param   TSharedPtr<sfSessionInfo> sessionInfoPtr - determines where to connect.
     */
    static void JoinSession(TSharedPtr<sfSessionInfo> sessionInfoPtr);

    /**
     * Called after connecting to a session.
     */
    static void OnConnect();

    /**
     * Called after disconnecting from a session.
     */
    static void OnDisconnect();

private:
    static IConsoleCommand* m_mockWebServiceCommand;
    static bool m_redrawActiveViewport;
    static TSharedPtr<sfUI> m_sfUIPtr;
    static TSharedPtr<sfUndoManager> m_undoManagerPtr;
    static TMap<uint32_t, UMaterialInstanceDynamic*> m_lockMaterials;
    static UMaterialInterface* m_lockMaterialPtr;
    static TArray<UObject*> m_replacedObjects;
    static ksEvent<sfUser::SPtr&>::SPtr m_onUserColorChangeEventPtr;
    static ksEvent<sfUser::SPtr&>::SPtr m_onUserLeaveEventPtr;
    static FDelegateHandle m_onObjectsReplacedHandle;
    static FDelegateHandle m_onHotReloadHandle;

    FDelegateHandle m_updateHandle;

    /**
     * Called when a user's color changes.
     *
     * @param   sfUser::SPtr userPtr
     */
    static void OnUserColorChange(sfUser::SPtr userPtr);

    /**
     * Called when a user disconnects.
     *
     * @param   sfUser::SPtr userPtr
     */
    static void OnUserLeave(sfUser::SPtr userPtr);

    /**
     * Called when Unreal deletes objects and replaces them with new ones, such as when rerunning the construction
     * script to recreate components in a blueprint whenever a property changes. If old objects were in the
     * sfObjectMap, replaces them with the new ones.
     *
     * @param   const TMap<UObject*, UObject*>& replacementMap mapping old objects to new objects.
     */
    static void OnObjectsReplaced(const TMap<UObject*, UObject*>& replacementMap);

    /**
     * Called after a hot reload. Syncs actors/components that were changed by the hot reload.
     *
     * @param   bool automatic - true if the hot reload was triggered automatically.
     */
    static void OnHotReload(bool automatic);
};