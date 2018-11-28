#pragma once

#include <CoreMinimal.h>
#include <IDetailsView.h>
#include <PropertyEditorModule.h>
#include <EdMode.h>
#include <Engine/Selection.h>
#include <LevelEditor.h>
#include <SSCSEditor.h>

#include "../sfObjectMap.h"

#include <sfSession.h>

/**
 * Manages details panels. Disable editing when selected objects are locked.
 */
class sfDetailsPanelManager
{
public:
    /**
     * @return  sfDetailsPanelManager& singleton instance.
     */
    static sfDetailsPanelManager& Get();

    /**
     * Constructor
     *
     * @param   FPropertyEditorModule& propertyEditorModule
     */
    sfDetailsPanelManager(FPropertyEditorModule& propertyEditorModule);

    /**
     * Destructor
     */
    virtual ~sfDetailsPanelManager();

    /**
     * Initialization. Called after connecting to a session.
     */
    virtual void Initialize();

    /**
     * Deinitialization. Called after disconnecting from a session.
     */
    virtual void CleanUp();
    
    /**
     * Updates the tree of the SSCSEditor in the details panel.
     */
    void UpdateDetailsPanelTree();

    /**
     * Returns all selected actors including actors inspected by locked detail panels.
     *
     * @return  TSet<AActor*>
     */
    TSet<AActor*> GetSelectedActors();

private:
    /**
     * Edit mode class to prevent editing when selected actors contains locked actors.
     */
    class FSceneFusionEdMode : public FEdMode
    {
    public:
        // FEdMode interface

        /**
         * Always returns true so this edit mode's existence would not affect other edit modes.
         *
         * @param   FEditorModeID otherModeID
         * @return  bool
         */
        virtual bool IsCompatibleWith(FEditorModeID otherModeID) const override { return true; }

        /**
         * If selected actors are all locked, returns EEditAction::Halt to prevent duplicating.
         *
         * @return  EEditAction::Type
         */
        virtual EEditAction::Type GetActionEditDuplicate() override { return IsEditable(); }

        /**
         * If selected actors are all locked, returns EEditAction::Halt to prevent deleting.
         *
         * @return  EEditAction::Type
         */
        virtual EEditAction::Type GetActionEditDelete() override { return IsEditable(); }

        /**
         * If selected actors are all locked, returns EEditAction::Halt to prevent cutting.
         *
         * @return  EEditAction::Type
         */
        virtual EEditAction::Type GetActionEditCut() override { return IsEditable(); }

        /**
         * If selected actors are all locked, returns EEditAction::Halt to prevent pasting.
         *
         * @return  EEditAction::Type
         */
        virtual EEditAction::Type GetActionEditPaste() override { return IsEditable(); }

        // End of FEdMode interface

    private:
        /**
         * If selected actors are all locked, returns EEditAction::Halt. Otherwise, returns EEditAction::Skip.
         *
         * @return  EEditAction::Type
         */
        EEditAction::Type IsEditable()
        {
            for (auto iter = GEditor->GetSelectedActorIterator(); iter; ++iter)
            {
                AActor* actorPtr = Cast<AActor>(*iter);
                if (actorPtr == nullptr)
                {
                    continue;
                }
                sfObject::SPtr objPtr = sfObjectMap::GetSFObject(actorPtr);
                if (objPtr == nullptr || objPtr->CanEdit())
                {
                    return EEditAction::Skip;
                }
            }
            return EEditAction::Halt;
        }
    };

    /**
     * Callback for iterating SSCSEditor.
     *
     * @param   TSharedPtr<IDetailsView> pointer of the details view
     * @param   TSharedPtr<SSCSEditor> pointer of the SSCSEditor
     */
    typedef std::function<void(
        TSharedPtr<IDetailsView> detailsViewPtr,
        TSharedPtr<SSCSEditor> sscsEditorPtr)> ForEachSSCSEditorCallback;

    static TSharedPtr<sfDetailsPanelManager> m_instancePtr;

    sfSession::SPtr m_sessionPtr;
    FPropertyEditorModule& m_propertyEditorModule;
    TArray<TSharedPtr<IDetailsView>> m_detailsViews;

    FDelegateHandle m_onPropertyEditorOpened;
    FAreObjectsEditable m_editableObjectPredicate;

    /**
     * Register selection predicate for detail panel.
     */
    void RegisterEditableObjectPredicates();

    /**
     * Unregister selection predicate for detail panel.
     */
    void UnregisterEditableObjectPredicates();

    /**
     * Fetches details views in all open details panels.
     */
    void FetchDetailsViews();

    /**
     * Calls the given callback function for every SSCSEditor.
     *
     * @param   ForEachSSCSEditorCallback callback
     */
    void ForEachSSCSEditor(ForEachSSCSEditorCallback callback);

    /**
     * Overrides SSCSEditor's AllowEditing, name area and AddComponent button's IsEnabled.
     */
    void OverrideAllowEditingAndIsEnabled();

    /**
     * Check if a selection of UObjects is editable.
     *
     * @param   const TArray<TWeakObjectPtr<UObject>>& objects
     */
    bool AreObjectsEditable(const TArray<TWeakObjectPtr<UObject>>& objects);

    /**
     * Returns true if all actors in the given objects are unlocked.
     *
     * @return  const TArray<TWeakObjectPtr<UObject>>& objects
     */
    bool CanEdit(const TArray<TWeakObjectPtr<UObject>>& objects);

    /**
     * Allows component tree editing if all actors in the given objects are unlocked.
     *
     * @param   const TArray<TWeakObjectPtr<UObject>>& objects
     */
    bool AllowComponentTreeEditing(const TArray<TWeakObjectPtr<UObject>>& objects);

    /**
     * Set details panel's name area and AddComponent button enabled flag.
     *
     * @param   bool enable
     */
    void SetDetailsPanelEnabled(bool enabled);
    
    /**
     * Recursively iterate through given widget and its descendants.
     * If the type is in the given types, set its enabled flag as given.
     *
     * @param   TSharedPtr<SWidget> widget - widget to iterate
     * @param   TArray<FName> disabledWidgetTypes - array of widget types to set enable flag on
     * @param   bool enabled
     */
    void SetEnabledRecursive(TSharedPtr<SWidget> widget, TArray<FName> disabledWidgetTypes, bool enabled);

    /**
     * Recursively iterate through given widget and its descendants to find widget with of given type.
     * Returns the first widget found.
     *
     * @param   TSharedPtr<SWidget> widget - widget to iterate
     * @param   FName widgetType - widget type to look for
     */
    TSharedPtr<SWidget> FindWidgetRecursive(TSharedPtr<SWidget> widget, FName widgetType);

    /**
     * Called when a new property editor is opened.
     * Fetches details views. Overrides SSCSEditor's AllowEditing, name area and AddComponent button's IsEnabled.
     */
    void OnPropertyEditorOpened();
};