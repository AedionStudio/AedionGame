#pragma once


#include "sfUIPanel.h"

#include <Runtime/Slate/Public/Framework/SlateDelegates.h>
#include <Widgets/Input/SEditableTextBox.h>

/**
 * Scene Fusion login panel
 */
class sfUILoginPanel : public sfUIPanel
{
public:
    /**
     * Delegate for UI login
     */
    DECLARE_DELEGATE(OnLoginDelegate);

    /**
     * Constructor
     */
    sfUILoginPanel();  

    /**
     * Authenticate
     */
    void Authenticate();

    /**
     * Login event handler
     */
    OnLoginDelegate OnLogin;
private:
    // UI Widgets
    TSharedPtr<SEditableTextBox> m_emailPtr;
    TSharedPtr<SEditableTextBox> m_passPtr;

    /**
     * Login if enter was pressed in a text box.
     *
     * @param   const FText& text
     * @param   ETextCommit::Type commitType
     */
    void OnCommit(const FText& text, ETextCommit::Type commitType);

    /**
     * Send a web request to login
     *
     * @param   FReply - UI event reply
     */
    FReply Login();

    /**
     * Handle the reply from a Login web request
     *
     * @param   const FString& - error message
     */
    void LoginReply(const FString& error);

    /**
     * Handle the reply from an automatic login web request
     *
     * @param   const FString& - error message
     */
    void AuthenticateReply(const FString& error);
};