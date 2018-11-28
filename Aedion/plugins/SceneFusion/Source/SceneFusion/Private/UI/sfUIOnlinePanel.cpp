#include "sfUIOnlinePanel.h"
#include "../SceneFusion.h"
#include "../sfConfig.h"
#include "../Includes/ksMultiType.h"

#include <Widgets/Input/SButton.h>

using namespace KS::SceneFusion2;

sfUIOnlinePanel::sfUIOnlinePanel() :
    sfUIPanel("Online")
{
    m_showAvatar = sfConfig::Get().ShowAvatar;

    // Leave Session Button
    m_contentPtr->AddSlot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(5, 2).AutoHeight()
    [
        SNew(SButton)
        .Text(FText::FromString("Leave Session"))
        .OnClicked(FOnClicked::CreateLambda([this]()->FReply {
            OnLeaveSession.Execute();
            return FReply::Handled();
        }))
    ];

    InfoArea();
    PreferenceArea();
    UserArea();
}

void sfUIOnlinePanel::InfoArea()
{
    m_contentPtr->AddSlot()
    .HAlign(HAlign_Fill)
    .VAlign(VAlign_Center)
    .Padding(5, 2)
    .AutoHeight()
    [
        SNew(SBorder)
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Center)
        .BorderImage(new FSlateColorBrush(FLinearColor(FColor(0, 0, 0, 128))))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Center).AutoHeight().Padding(2)
            [
                SNew( STextBlock)
                .Text(FText::FromString("Info"))
                .Font(sfUIStyles::GetDefaultFontStyle("Bold", 10))
            ]
            + SVerticalBox::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Center).AutoHeight().Padding(10, 0)
            [
                SNew(STextBlock)
                .Text_Lambda([]()->const FText {
                    FString info = "Synced Actors: ";
                    info.AppendInt(SceneFusion::ActorManager->NumSyncedActors());
                    info.Append("\nSynced Objects: ");
                    info.AppendInt(SceneFusion::Service->Session()->NumObjects());
                    return FText::FromString(info); 
                })
            ]
        ]
    ];
}

void sfUIOnlinePanel::PreferenceArea()
{
   m_contentPtr->AddSlot()
    .HAlign(HAlign_Fill)
    .VAlign(VAlign_Center)
    .Padding(5, 10)
    .AutoHeight()
    [
        SNew(SBorder)
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Center)
        .BorderImage(new FSlateColorBrush(FLinearColor(FColor(0, 0, 0, 128))))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Center).AutoHeight().Padding(2)
            [
                SNew( STextBlock)
                .Text(FText::FromString("Preferences"))
                .Font(sfUIStyles::GetDefaultFontStyle("Bold", 10))
            ]
            + SVerticalBox::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Center).AutoHeight().Padding(10, 0)
            [
                SNew(SCheckBox)
                .OnCheckStateChanged_Raw(this, &sfUIOnlinePanel::OnShowAvatarsCheckboxChanged)
                .IsChecked_Lambda([this]()-> const ECheckBoxState {
                    return m_showAvatar ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; 
                })
                .ToolTipText(FText::FromString("Avatars are meshes rendered in the viewport to show the position of other users."))
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("Show Avatars"))
                ]
            ]
        ]
    ];
}

void sfUIOnlinePanel::UserArea()
{
    // UserList
    m_contentPtr->AddSlot()
    .HAlign(HAlign_Fill)
    .VAlign(VAlign_Center)
    .Padding(5, 2)
    .AutoHeight()
    [
        SAssignNew(m_userListPtr, SListView<TSharedPtr<sfUIUserInfo>>)
        .ItemHeight(24)
        .ListItemsSource(&m_users)
        .OnGenerateRow(TSlateDelegates<TSharedPtr<sfUIUserInfo>>::FOnGenerateRow::CreateLambda(
            [this](TSharedPtr<sfUIUserInfo> userInfoPtr, const TSharedRef<STableViewBase>& owner)->TSharedRef<ITableRow> {
                return SNew(sfUIUserRow, owner)
                    .Item(userInfoPtr)
                    .OnGoto(sfUIUserRow::OnGoto::CreateRaw(this, &sfUIOnlinePanel::Goto))
                    .OnFollow(sfUIUserRow::OnFollow::CreateRaw(this, &sfUIOnlinePanel::Follow))
                    .OnChangeColor(sfUIUserRow::OnChangeColor::CreateRaw(this, &sfUIOnlinePanel::SetUserColor));
            })
        )
        .SelectionMode(ESelectionMode::None)
        .HeaderRow (
            SNew(SHeaderRow)
            + SHeaderRow::Column("Icon").DefaultLabel(FText::FromString("")).FixedWidth(24)
            + SHeaderRow::Column("User").DefaultLabel(FText::FromString("User"))
            + SHeaderRow::Column("Goto").DefaultLabel(FText::FromString("")).FixedWidth(75)
            + SHeaderRow::Column("Follow").DefaultLabel(FText::FromString("")).FixedWidth(75)
        )
    ];
}

void sfUIOnlinePanel::Goto(TSharedPtr<sfUIUserInfo> userInfoPtr)
{
    OnGoTo.ExecuteIfBound(userInfoPtr->Id());
}

void sfUIOnlinePanel::Follow(TSharedPtr<sfUIUserInfo> userInfoPtr)
{
    uint32_t followingUserId = OnFollow.Execute(userInfoPtr.IsValid() ? userInfoPtr->Id() : 0);
    for (auto user : m_users)
    {
        user->SetIsFollowed(user->Id() == followingUserId);
    }
}

void sfUIOnlinePanel::UnfollowCamera()
{
    for (auto user : m_users)
    {
        user->SetIsFollowed(false);
    }
}

void sfUIOnlinePanel::AddUser(sfUser::SPtr userPtr)
{
    if (userPtr != nullptr) {
        if (!m_userMap.Contains(userPtr->Id())) {
            TSharedPtr<sfUIUserInfo> userInfoPtr = MakeShareable(new sfUIUserInfo(userPtr));
            m_userMap.Add(userInfoPtr->Id(), userInfoPtr);
            if (userPtr->IsLocal()) {
                m_localUserPtr = userInfoPtr;
                m_users.Insert(userInfoPtr, 0);
            }
            else {
                m_users.Add(userInfoPtr);
            }
            m_userListPtr->RequestListRefresh();
            m_widgetPtr->Invalidate(EInvalidateWidget::Layout);
        }
    }
}

void sfUIOnlinePanel::RemoveUser(sfUser::SPtr userPtr)
{
    if (userPtr != nullptr) {
        TSharedPtr<sfUIUserInfo> userInfoPtr;
        if (m_userMap.RemoveAndCopyValue(userPtr->Id(), userInfoPtr))
        {
            m_users.Remove(userInfoPtr);
            m_userListPtr->RequestListRefresh();
            m_widgetPtr->Invalidate(EInvalidateWidget::Layout);
        }
    }
}

void sfUIOnlinePanel::UpdateUserColor(sfUser::SPtr userPtr)
{
    if (m_userMap.Contains(userPtr->Id())) {
        m_userMap[userPtr->Id()]->Refresh();
        m_userListPtr->RequestListRefresh();
    }
}

void sfUIOnlinePanel::ClearUsers()
{
    m_userMap.Empty();
    m_users.Empty();
    m_localUserPtr = nullptr;
    m_userListPtr->RequestListRefresh();
    m_widgetPtr->Invalidate(EInvalidateWidget::Layout);
}

void sfUIOnlinePanel::SetUserColor(TSharedPtr<sfUIUserInfo> userInfoPtr, FLinearColor color)
{
    DisplayMessage("Setting User Color...", sfUIMessageBox::INFO);
    SceneFusion::WebService->SetUserColor(
        sfConfig::Get().CompanyProject,
        color,
        sfBaseWebService::OnSetUserColorDelegate::CreateRaw(this, &sfUIOnlinePanel::SetUserColorReply)
    );
}

void sfUIOnlinePanel::SetUserColorReply(FLinearColor color, const FString& error)
{
    if (!error.IsEmpty()) {
        DisplayMessage(error, sfUIMessageBox::ERROR);
        return;
    }

    DisplayMessage("", sfUIMessageBox::INFO);
    SceneFusion::Service->Session()->SetUserColor(color.R, color.G, color.B);
}

void sfUIOnlinePanel::OnShowAvatarsCheckboxChanged(ECheckBoxState newCheckedState)
{
    m_showAvatar = newCheckedState == ECheckBoxState::Checked;
    SceneFusion::AvatarManager->SetAvatarVisibility(m_showAvatar);
    sfConfig& config = sfConfig::Get();
    config.ShowAvatar = m_showAvatar;
    config.Save();
}