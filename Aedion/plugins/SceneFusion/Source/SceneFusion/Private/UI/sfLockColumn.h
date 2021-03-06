#pragma once

#include "CoreMinimal.h"
#include <Editor/SceneOutliner/Public/ISceneOutlinerColumn.h>
#include <Editor/SceneOutliner/Public/SceneOutlinerVisitorTypes.h>
#include <Editor/SceneOutliner/Public/ActorTreeItem.h>

#include "sfOutlinerManager.h"

/**
 * A custom column for the SceneOutliner which shows the actor state in a session.
 */
class FsfLockColumn : public ISceneOutlinerColumn
{
public:
    /**
     * Constructor.
     *
     * @param   ISceneOutliner& outliner
     */
    FsfLockColumn(ISceneOutliner& outliner) { }

    /**
     * Constructor.
     *
     * @param   TSharedPtr<sfOutlinerManager> outlinerManagerPtr
     */
    FsfLockColumn(TSharedPtr<sfOutlinerManager> outlinerManagerPtr);

    /**
     * Destructor.
     */
    virtual ~FsfLockColumn();

    /**
     * Gets column id.
     *
     * @return  FName
     */
    static FName GetID();

    /**
     * Gets column id.
     *
     * @return  FName
     * @see     ISceneOutlinerColumn::GetColumnID
     */
    virtual FName GetColumnID() override;

    /**
     * Contstructs header row column.
     *
     * @return  SHeaderRow::FColumn::FArguments
     * @see     ISceneOutlinerColumn::ConstructHeaderRowColumn
     */
    virtual SHeaderRow::FColumn::FArguments ConstructHeaderRowColumn() override;

    /**
     * Contstructs row widget.
     *
     * @param   SceneOutliner::FTreeItemRef treeItem
     * @param   const STableRow<SceneOutliner::FTreeItemPtr>& row
     * @return  const TSharedRef<SWidget>
     * @see     ISceneOutlinerColumn::ConstructRowWidget
     */
    virtual const TSharedRef<SWidget> ConstructRowWidget(
        SceneOutliner::FTreeItemRef treeItem,
        const STableRow<SceneOutliner::FTreeItemPtr>& row) override;

private:
    TSharedPtr<sfOutlinerManager> m_outlinerManagerPtr;

    /**
     * Contstructs row widget.
     *
     * @param   const TWeakObjectPtr<AActor>& actor
     * @return  TSharedRef<SWidget>
     */
    TSharedRef<SWidget> ConstructRowWidget(const TWeakObjectPtr<AActor>& actor);

    /**
     * Column generator.
     */
    struct FColumnGenerator : SceneOutliner::FColumnGenerator
    {
        FsfLockColumn& Column;
        FColumnGenerator(FsfLockColumn& InColumn) : Column(InColumn) {}

        /**
         * Contstructs widget from the given actor tree item.
         *
         * @param   SceneOutliner::FActorTreeItem& ActorItem
         * @return  TSharedRef<SWidget>
         */
        virtual TSharedRef<SWidget> GenerateWidget(SceneOutliner::FActorTreeItem& ActorItem) const override
        {
            return Column.ConstructRowWidget(ActorItem.Actor);
        }
    };
};