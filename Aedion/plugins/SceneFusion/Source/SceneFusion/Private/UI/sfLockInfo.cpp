#include "sfLockInfo.h"

#include <Widgets/Images/SImage.h>
#include "sfUIStyles.h"


sfLockInfo::sfLockInfo() :
    LockType{ sfActorManager::NotSynced },
    LockOwner{ nullptr }
{
    SAssignNew(Icon, SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth()
        .Padding(2.0f, 2.0f, 2.0f, 2.0f)
        [
            SNew(SImage)
            .Image_Raw(this, &sfLockInfo::GetLockIcon)
            .ColorAndOpacity_Raw(this, &sfLockInfo::GetLockColor)
            .ToolTipText_Raw(this, &sfLockInfo::GetLockTooltip)
        ];
}


const FSlateBrush* sfLockInfo::GetLockIcon() const
{
    if (LockType != sfActorManager::Unlocked)
    {
        return sfUIStyles::Get().GetBrush("SceneFusion.Locked");
    }
    return sfUIStyles::Get().GetBrush("SceneFusion.Unlocked");
}


FSlateColor sfLockInfo::GetLockColor() const
{
    switch (LockType)
    {
        case sfActorManager::Unlocked:
        {
            return FSlateColor(FLinearColor::Gray);
        }
        case sfActorManager::PartiallyLocked:
        {
            return FSlateColor(FLinearColor::White);
        }
        case sfActorManager::FullyLocked:
        {
            if (LockOwner != nullptr)
            {
                ksColor userColor = LockOwner->Color();
                return FSlateColor(FLinearColor(userColor.R(), userColor.G(), userColor.B()));
            }
            return FSlateColor(FLinearColor::Red);
        }
        default:
        {
            return FSlateColor(FLinearColor::Transparent);
        }
    }
}


FText sfLockInfo::GetLockTooltip() const
{
    switch (LockType)
    {
        case sfActorManager::Unlocked:
        {
            return FText::FromString("Synced and unlocked.");
        }
        case sfActorManager::PartiallyLocked:
        {
            return FText::FromString("Partially locked. Property editing disabled.");
        }
        case sfActorManager::FullyLocked:
        {
            FString tooltip = "Fully locked";
            if (LockOwner != nullptr)
            {
                tooltip += " by " + FString(UTF8_TO_TCHAR(LockOwner->Name().c_str()));
            }
            tooltip += ". Property and child editing disabled.";
            return FText::FromString(tooltip);
        }
        default:
        {
            return FText();
        }
    }
}