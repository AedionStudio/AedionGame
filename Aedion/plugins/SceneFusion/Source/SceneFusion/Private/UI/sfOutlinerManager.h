#pragma once

#include <CoreMinimal.h>

#include <TabManager.h>
#include <Editor/SceneOutliner/Public/ISceneOutliner.h>
#include <Editor/SceneOutliner/Public/ISceneOutlinerColumn.h>
#include <Widgets/SWidget.h>
#include <Widgets/Images/SImage.h>
#include <Widgets/Layout/SBox.h>

#include "../ObjectManagers/sfActorManager.h"
#include "sfUIStyles.h"
#include "sfLockInfo.h"

/**
 * Manages a column to show Scene Fusion icons in the world outliner.
 */
class sfOutlinerManager : public TSharedFromThis<sfOutlinerManager>
{
public:
    /**
     * Constructor
     */
    sfOutlinerManager();

    /**
     * Destructor
     */
    virtual ~sfOutlinerManager();

    /**
     * Initialization. Called after connecting to a session.
     */
    virtual void Initialize();

    /**
     * Deinitialization. Called after disconnecting from a session.
     */
    virtual void CleanUp();

    /**
     * Sets lock state.
     *
     * @param   AActor* actorPtr
     * @param   sfActorManager::LockType lockType
     * @param   sfUser::SPtr lockOwner
     */
    void SetLockState(AActor* actorPtr, sfActorManager::LockType lockType, sfUser::SPtr lockOwner);

    /**
     * Construct lock icon widget for the row of the given actor.
     *
     * @param   AActor* actorPtr
     * @return  TSharedRef<SWidget>
     */
    const TSharedRef<SWidget> ConstructRowWidget(AActor* actorPtr);

private:
    TSharedPtr<FTabManager> m_tabManager;
    TMap<AActor*, TSharedPtr<sfLockInfo>> m_actorLockInfos;
    FDelegateHandle m_onActorDeletedHandle;
    FDelegateHandle m_onObjectsReplacedHandle;

    /**
     * Creates column for lock icons.
     *
     * @param   ISceneOutliner& SceneOutliner
     * @return  TSharedRef<ISceneOutlinerColumn>
     */
    TSharedRef<ISceneOutlinerColumn> CreateLockColumn(ISceneOutliner& SceneOutliner);

    /**
     * Reconstructs the world outliner tab.
     */
    void ReconstructWorldOutliner();

    /**
     * Called when an actor is deleted from the level.
     *
     * @param   AActor* actorPtr
     */
    void OnActorDeleted(AActor* actorPtr);

    /**
     * Called when Unreal deletes objects and replaces them with new ones, such as when rerunning the construction
     * script to recreate components in a blueprint whenever a property changes. If old objects were in the
     * lock state map, replaces them with the new ones.
     *
     * @param   const TMap<UObject*, UObject*>& replacementMap mapping old objects to new objects.
     */
    void OnObjectsReplaced(const TMap<UObject*, UObject*>& replacementMap);

    /**
     * Finds lock info for the given actor. Adds one if there is none.
     *
     * @param   AActor* actorPtr
     * @return  TSharedPtr<sfLockInfo>
     */
    TSharedPtr<sfLockInfo> FindOrAddLockInfo(AActor* actorPtr);
};