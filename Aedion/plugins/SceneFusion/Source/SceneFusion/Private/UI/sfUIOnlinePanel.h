#pragma once

#include "sfUIPanel.h"
#include "sfUIUserRow.h"

#include "sfUser.h"

#include <Runtime/Slate/Public/Framework/SlateDelegates.h>

using namespace KS::SceneFusion2;

class sfUIOnlinePanel : public sfUIPanel
{
public:
    /**
     * Delegate for UI leave session
     */
    DECLARE_DELEGATE(OnLeaveSessionDelegate);

    /**
     * Delegate for go to camera.
     *
     * @param   uint32_t userId
     */
    DECLARE_DELEGATE_OneParam(OnGoToDelegate, uint32_t);

    /**
     * Delegate for follow camera.
     *
     * @param   uint32_t userId
     * @return  uint32_t id of user it is following. Return 0 if it is not following any user.
     */
    DECLARE_DELEGATE_RetVal_OneParam(uint32_t, OnFollowDelegate, uint32_t);

    /**
     * Constructor
     */
    sfUIOnlinePanel();

    /**
     * Leave session event handler
     */
    OnLeaveSessionDelegate OnLeaveSession;

    /**
     * Goto button click event handler.
     */
    OnGoToDelegate OnGoTo;

    /**
     * Follow button click event handler.
     */
    OnFollowDelegate OnFollow;

    /**
     * Add a user to the user list
     *
     * @param   sfUser::SPtr - user pointer
     */
    void AddUser(sfUser::SPtr userPtr);

    /**
     * Remove a user from the user list
     *
     * @param   sfUser::SPtr - user pointer
     */
    void RemoveUser(sfUser::SPtr userPtr);

    /**
     * Update the UI color for a user
     *
     * @param   sfUser::SPtr - user pointer
     */
    void UpdateUserColor(sfUser::SPtr userPtr);

    /**
     * Clear the user list.
     */
    void ClearUsers();

    /**
     * Unfollow camera.
     */
    void UnfollowCamera();

private:
    TSharedPtr<SListView<TSharedPtr<sfUIUserInfo>>> m_userListPtr;
    
    TMap<uint32, TSharedPtr<sfUIUserInfo>> m_userMap;
    TSharedPtr<sfUIUserInfo> m_localUserPtr;
    TArray<TSharedPtr<sfUIUserInfo>> m_users;
    bool m_showAvatar;

    /**
     * UI for session info.
     */
    void InfoArea();

    /**
     * UI for session preferences.
     */
    void PreferenceArea();

    /**
     * UI for session users.
     */
    void UserArea();

    /**
     * Goto another user's camera
     *
     * @param   TSharedPtr<sfSessionInfo> sessionPtr
     */
    void Goto(TSharedPtr<sfUIUserInfo> userInfoPtr);

    /**
     * Follow another user's camera
     *
     * @param   TSharedPtr<sfSessionInfo> sessionPtr
     */
    void Follow(TSharedPtr<sfUIUserInfo> userInfoPtr);

    /**
     * Handle a user color change action.
     *
     * @param   TSharedPtr<sfUserInfo> - UI user info
     * @param   FLinearColor - new color
     */
    void SetUserColor(TSharedPtr<sfUIUserInfo> userInfoPtr, FLinearColor color);

    /**
     * Handle the reply from the webserver for setting user color.
     *
     * @param   FLinearColor - new color
     * @param   FString - error message
     */
    void SetUserColorReply(FLinearColor color, const FString& error);

    /**
     * Handles show avatars checkbox change.
     *
     * @param   ECheckBoxState newCheckedState
     */
    void OnShowAvatarsCheckboxChanged(ECheckBoxState newCheckedState);
};