#include "sfBaseWebService.h"
#include "../SceneFusion.h"
#include "../sfConfig.h"

#define LOG_CHANNEL "sfBaseWebService"

FString sfBaseWebService::URL = "https://matchmaker-console.kinematicsoup.com/api/";

bool sfBaseWebService::IsLoggedIn()
{
    return m_isLoggedIn; 
}

void sfBaseWebService::HandleLogoutResponse(const FString& error, OnLogoutDelegate onLogout)
{
    m_isLoggedIn = false;
    sfConfig& settings = sfConfig::Get();
    settings.Token = "";
    settings.Save();
    onLogout.Execute();
}

void sfBaseWebService::HandleLoginResponse(const FString& email, const FString& loginToken, TSharedPtr<FJsonObject> jsonPtr, const FString& error, OnLoginDelegate onLogin)
{
    m_isLoggingIn = false;
        
    // Success
    if (error.IsEmpty() && jsonPtr.IsValid()) {
        m_isLoggedIn = true;
        sfConfig& settings = sfConfig::Get();
        settings.Name = jsonPtr->GetStringField("name");
        settings.Email = email;
        settings.Token = loginToken;
        settings.SFToken = jsonPtr->GetStringField("token");
        settings.Save();
        onLogin.Execute(error);
        return;
    }

    // Clear token on failed logins
    sfConfig& settings = sfConfig::Get();
    settings.Token = "";
    settings.SFToken = "";
    settings.Save();

    // Error
    if (!error.IsEmpty()) {
        onLogin.Execute(error);
        return;
    }

    // Unexpected error
    KS::Log::Error("Login response did not contain an error message or valid json response.", LOG_CHANNEL);
    onLogin.Execute("Unexpected login error.");
}

void sfBaseWebService::HandleGetSessionsResponse(
    TSharedPtr<FJsonObject> jsonPtr, 
    const FString& error, 
    OnGetSessionsDelegate onGetSessionsDelegate)
{
    m_isFetchingSessions = false;
    ProjectMap projectMap;

    // Success
    if (error.IsEmpty() && jsonPtr.IsValid()) {
        for (auto pair : jsonPtr->Values) {
            TSharedPtr<FJsonObject> jsonProject = pair.Value->AsObject();
            TSharedPtr<sfProjectInfo> projectInfoPtr = MakeShareable(new sfProjectInfo());
            projectInfoPtr->Name = pair.Key;

            // Get session data
            if (jsonProject->HasField("sessions")) {
                TSharedPtr<FJsonObject> jsonSessionsPtr = jsonProject->GetObjectField("sessions");
                for (auto pair : jsonSessionsPtr->Values) {
                    TSharedPtr<sfSessionInfo> sessionInfoPtr = ParseJsonSession(pair.Value->AsObject());
                    if (sessionInfoPtr.IsValid()) {
                        sessionInfoPtr->ProjectName = projectInfoPtr->Name;
                        projectInfoPtr->Sessions.Add(sessionInfoPtr);
                    }
                }
            }

            // Get usage limits
            if (jsonProject->HasField("usagelimits")) {
                TSharedPtr<FJsonObject> jsonUsageLimits = jsonProject->GetObjectField("usagelimits");
                projectInfoPtr->SessionLimit = jsonUsageLimits->GetIntegerField("session limit");
                projectInfoPtr->UserLimit = jsonUsageLimits->GetIntegerField("user limit");
                projectInfoPtr->ObjectLimit = jsonUsageLimits->GetIntegerField("object limit");
                projectInfoPtr->UserCount = jsonUsageLimits->GetIntegerField("users");
                projectInfoPtr->SessionCount = jsonUsageLimits->GetIntegerField("sessions");
            
                if (projectInfoPtr->SessionLimit == 0 && projectInfoPtr->UserLimit != 0) {
                    projectInfoPtr->SessionLimit = projectInfoPtr->UserLimit;
                }
            }
            else {
                projectInfoPtr->SessionLimit = -1;
                projectInfoPtr->UserLimit = 2;
                projectInfoPtr->ObjectLimit = 0;
                projectInfoPtr->UserCount = 0;
                projectInfoPtr->SessionCount = 0;
            }

            // Calculate remaining subscription days
            if (jsonProject->HasField("trial_end")) {
                projectInfoPtr->IsTrial = true;
                FDateTime trialEndDate;
                FDateTime::Parse(jsonProject->GetStringField("trial_end"), trialEndDate);
                projectInfoPtr->DaysRemaining = FMath::FloorToDouble((trialEndDate - FDateTime::UtcNow()).GetTotalDays());
            }
            else if (jsonProject->HasField("subscription_end")) {
                projectInfoPtr->IsTrial = false;
                FDateTime subscriptionEndDate;
                FDateTime::Parse(jsonProject->GetStringField("subscription_end"), subscriptionEndDate);
            projectInfoPtr->DaysRemaining = FMath::FloorToDouble(
                (subscriptionEndDate - FDateTime::UtcNow()).GetTotalDays());
            }
            projectMap.Add(projectInfoPtr->Name, projectInfoPtr);
        }

        onGetSessionsDelegate.Execute(projectMap, error);
        return;
    }

    // Error
    if (!error.IsEmpty()) {
        onGetSessionsDelegate.Execute(projectMap, error);
        return;
    }

    // Unexpected error
    KS::Log::Error("GetSessions response did not contain an error message or valid json response.", LOG_CHANNEL);
    onGetSessionsDelegate.Execute(projectMap, "Unexpected get sessions error.");
}

void sfBaseWebService::HandleStartSessionResponse(
    TSharedPtr<FJsonObject> jsonPtr, 
    const FString& error, 
    OnStartSessionDelegate onStartSession)
{
    m_isStartingSession = false;

    // Success
    if (error.IsEmpty() && jsonPtr.IsValid()) {
        for (auto pair : jsonPtr->Values) {
            TSharedPtr<FJsonObject> jsonProject = pair.Value->AsObject();
            for (auto pair : jsonProject->Values) {
                TSharedPtr<sfSessionInfo> sessionInfoPtr = ParseJsonSession(pair.Value->AsObject());
                if (sessionInfoPtr.IsValid()) {
                    sessionInfoPtr->ProjectName = pair.Key;
                    onStartSession.Execute(sessionInfoPtr, error);
                    return;
                }
            }
        }
    }

    // Error
    if (!error.IsEmpty()) {
        onStartSession.Execute(nullptr, error);
        return;
    }

    // Unexpected error
    KS::Log::Error("StartSession response did not contain an error message or valid json response.", LOG_CHANNEL);
    onStartSession.Execute(nullptr, "Unexpected start session error.");
}

void sfBaseWebService::HandleStopSessionResponse(
    uint32 roomId, 
    const FString& error, 
    OnStopSessionDelegate onStopSession)
{
    m_isStoppingSession = false;
    onStopSession.Execute(roomId, error);
}

void sfBaseWebService::HandleSetUserColorResponse(
    const FString& companyProject,
    const FLinearColor& color,
    const FString& error,
    OnSetUserColorDelegate onSetUserColor)
{
    m_isSettingUserColor = false;
    onSetUserColor.Execute(color, error);
}

void sfBaseWebService::HandleRefreshTokenResponse(const FString& error, OnRefreshTokenDelegate onRefreshToken)
{
    m_isRefreshingToken = false;
    onRefreshToken.Execute(error);
}

TSharedPtr<sfSessionInfo> sfBaseWebService::ParseJsonSession(TSharedPtr<FJsonObject> jsonPtr)
{
    if (!jsonPtr.IsValid()) {
        return nullptr;
    }

    TSharedPtr<sfSessionInfo> sessionInfoPtr = MakeShareable(new sfSessionInfo());
    if (jsonPtr->HasField("roomInfo"))
    {
        sessionInfoPtr->RoomInfoPtr = ParseJsonRoom(jsonPtr->GetObjectField("roomInfo"));
        if (sessionInfoPtr->RoomInfoPtr != nullptr &&
            jsonPtr->TryGetStringField("creator", sessionInfoPtr->Creator) &&
            jsonPtr->TryGetStringField("instanceName", sessionInfoPtr->LevelName) &&
            jsonPtr->TryGetStringField("version", sessionInfoPtr->RequiredVersion) &&
            jsonPtr->TryGetBoolField("canDelete", sessionInfoPtr->CanStop))
        {
            return sessionInfoPtr;
        }
    }
    return nullptr;
}

ksRoomInfo::SPtr sfBaseWebService::ParseJsonRoom(TSharedPtr<FJsonObject> jsonPtr)
{
    if (!jsonPtr.IsValid()) {
        return nullptr;
    }
    FString scene, ip, room;
    if (jsonPtr->HasField("id") &&
        jsonPtr->HasField("port") &&
        jsonPtr->TryGetStringField("scene", scene) &&
        jsonPtr->TryGetStringField("ip", ip) &&
        jsonPtr->TryGetStringField("room", room)) 
    {
        ksRoomInfo::SPtr roomInfoPtr = ksRoomInfo::Create();
        roomInfoPtr->Id() = jsonPtr->GetIntegerField("id");
        roomInfoPtr->Port() = jsonPtr->GetIntegerField("port");
        roomInfoPtr->Scene() = TCHAR_TO_UTF8(*scene);
        roomInfoPtr->Host() = TCHAR_TO_UTF8(*ip);
        roomInfoPtr->Type() = TCHAR_TO_UTF8(*room);
        return roomInfoPtr;
    }
    return nullptr;
}

#undef LOG_CHANNEL
