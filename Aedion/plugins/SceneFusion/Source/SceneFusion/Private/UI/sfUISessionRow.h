#pragma once

#include "../sfSessionInfo.h"

#include <CoreMinimal.h>
#include <Widgets/Input/SButton.h>
#include <Widgets/Text/STextBlock.h>
#include <Widgets/Views/STableRow.h>
#include <Editor/EditorStyle/Public/EditorStyleSet.h>
#include <Runtime/SlateCore/Public/Brushes/SlateColorBrush.h>

/**
 * Handle the generation and events for a row in the sessions table.
 */
class sfUISessionRow : public SMultiColumnTableRow<TSharedPtr<sfSessionInfo>>
{
public:
    DECLARE_DELEGATE_OneParam(OnJoinSession, TSharedPtr<sfSessionInfo>);

    SLATE_BEGIN_ARGS(sfUISessionRow) {}
    SLATE_ARGUMENT(TSharedPtr<sfSessionInfo>, Item)
    SLATE_ARGUMENT(OnJoinSession, OnClicked)
    SLATE_END_ARGS()

public:
    /**
     * Construct a multi-column table row and track slate arguments
     *
     * @param   const FArguments& - slate arguments
     * @param   const TSharedRef<STableViewBase>& - table view that owns this row
     */
    void Construct(const FArguments& inArgs, const TSharedRef<STableViewBase>& owner)
    {
        Item = inArgs._Item;
        OnClicked = inArgs._OnClicked;
        SMultiColumnTableRow<TSharedPtr<sfSessionInfo>>::Construct(FSuperRowType::FArguments(), owner);
    }

    /**
     * Generate a widget for the column name.
     *
     * @param   const FName& - column name
     * @param   TSharedRef<SWidget>
     */
    virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& columnName) override
    {
        // iterate struct properties and generate a widget for it
        if (columnName == "Level")
        {
            return TextWidget(Item->LevelName);
        }

        if (columnName == "Creator")
        {
            return TextWidget(Item->Creator);
        }

        if (columnName == "Version")
        {
            return TextWidget(Item->RequiredVersion);
        }

        if (columnName == "Join")
        {
            return SNew(SBorder)
                .Padding(FMargin(5, 0))
                .HAlign(HAlign_Right).VAlign(VAlign_Center)
                .BorderImage(new FSlateColorBrush(FLinearColor(1.0f, 1.0f, 1.0f)))
                .BorderBackgroundColor(FColor::Black)
                [
                    SNew(SButton)
                    .HAlign(HAlign_Center)
                        .Text(FText::FromString("Join"))
                        .OnClicked(this, &sfUISessionRow::HandleOnClicked)
                ];
        }

        // default to null widget if property cannot be found
        return SNullWidget::NullWidget;
    }

    /**
     * Create a text widget
     *
     * @param   const FName& - text value
     */
    TSharedRef<SWidget> TextWidget(const FString& value)
    {
        return SNew(SBorder)
            .Padding(FMargin(5, 0))
            .VAlign(VAlign_Center)
            .BorderImage(new FSlateColorBrush(FLinearColor(1.0f, 1.0f, 1.0f)))
            .BorderBackgroundColor(FColor::Black)
            [
                SNew(STextBlock).Text(FText::FromString(value))
            ];
    }

    /**
     * Handle the click event for the join button
     *
     * @param   FReply - event state
     */
    FReply HandleOnClicked()
    {
        OnClicked.Execute(Item); 
        return FReply::Handled();
    }

protected:
    TSharedPtr<sfSessionInfo> Item;
    OnJoinSession OnClicked;
};