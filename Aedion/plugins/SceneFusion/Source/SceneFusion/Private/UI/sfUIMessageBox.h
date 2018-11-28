#pragma once

#include "sfUIStyles.h"

#include <Widgets/Text/STextBlock.h>
#include <Widgets/Images/SImage.h>
#include <Widgets/Layout/SBorder.h>
#include <Widgets/SBoxPanel.h>

/**
 * Display a message bow with an error, warning, or info icon.
 */
class sfUIMessageBox : public SBorder
{
public:
    SLATE_BEGIN_ARGS(sfUIMessageBox) {}
    SLATE_END_ARGS()

public:
    enum Icon { INFO, WARNING, ERROR };

    /**
     * Construct the widget
     *
     * @param   const FArguments& - slate arguments
     */
    void Construct(const FArguments& inArgs)
    {
        SBorder::Construct(SBorder::FArguments());
        SetBorderImage(new FSlateColorBrush(FLinearColor(FColor(255, 255, 255, 32))));

        m_widgetPtr = SNew(SHorizontalBox)
        + SHorizontalBox::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(5, 5).AutoWidth()
        [
            SAssignNew(m_imagePtr, SImage)
        ]
        + SHorizontalBox::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Center).Padding(5, 5, 10, 5)
        [
            SAssignNew(m_textPtr, STextBlock)
            .WrapTextAt(300)
            .Justification(ETextJustify::Left)
            .Text(FText::FromString("Test Message"))
        ];
    }

    /**
     * Display a message or hide the widget if the message is blank.
     *
     * @param   const FString& - message
     * @param   MessageType
     */
    void SetMessage(const FString& message, Icon type)
    {
        if (message.IsEmpty()) {
            SetVisibility(EVisibility::Hidden);
        }
        else {
            SetVisibility(EVisibility::Visible);
            SetContent(m_widgetPtr.ToSharedRef());
            m_textPtr->SetText(message);
            switch (type)
            {
                case WARNING: m_imagePtr->SetImage(sfUIStyles::Get().GetBrush("SceneFusion.Warning")); break;
                case ERROR: m_imagePtr->SetImage(sfUIStyles::Get().GetBrush("SceneFusion.Error")); break;
                default: m_imagePtr->SetImage(sfUIStyles::Get().GetBrush("SceneFusion.Info")); break;
            }
        }
    }
private:
    TSharedPtr<SImage> m_imagePtr;
    TSharedPtr<STextBlock> m_textPtr;
    TSharedPtr<SWidget> m_widgetPtr;
};