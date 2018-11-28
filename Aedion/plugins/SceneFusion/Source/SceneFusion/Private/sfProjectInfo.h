#pragma once

#include "sfSessionInfo.h"
#include <CoreMinimal.h>

/**
 * Scene Fusion project information.  Includes limits and a list of running sessions.
 */
struct sfProjectInfo
{
public:
    FString Name;
    int SessionLimit;
    int UserLimit;
    int ObjectLimit;
    int SessionCount;
    int UserCount;
    bool IsTrial;
    int DaysRemaining;
    TArray<TSharedPtr<sfSessionInfo>> Sessions;

    /**
     * Constructor
     */
    sfProjectInfo() :
        Name{ "" },
        SessionLimit{ -1 },
        UserLimit{ -1 },
        ObjectLimit{ -1 },
        SessionCount{ 0 },
        UserCount{ 0 },
        IsTrial{ false },
        DaysRemaining{ -1 }
    {
    }
};

typedef TMap<FString, TSharedPtr<sfProjectInfo>> ProjectMap;
