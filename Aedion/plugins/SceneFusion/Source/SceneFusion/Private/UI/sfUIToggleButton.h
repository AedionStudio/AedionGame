#pragma once

#include <Widgets/Input/SButton.h>
#include <Widgets/Text/STextBlock.h>
#include <Widgets/Images/SImage.h>
#include <EditorStyleSet.h>

/**
 * Toggle button.
 */
class sfUIToggleButton : public SButton
{
public:
    /**
     * Sets toggle button state and text.
     *
     * @param   bool isPressed
     * @param   FString text on button
     */
    void SetIsPressed(bool isPressed, FString text)
    {
        if (isPressed)
        {
            NormalImage = &FEditorStyle::Get().GetWidgetStyle<FButtonStyle>("FlatButton").Pressed;
            HoverImage = &FEditorStyle::Get().GetWidgetStyle<FButtonStyle>("FlatButton").Hovered;
            StaticCastSharedRef<STextBlock>(GetContent())->SetText(text);
        }
        else
        {
            NormalImage = &FEditorStyle::Get().GetWidgetStyle<FButtonStyle>("Button").Normal;
            HoverImage = &FEditorStyle::Get().GetWidgetStyle<FButtonStyle>("Button").Hovered;
            StaticCastSharedRef<STextBlock>(GetContent())->SetText(text);
        }
    }
};