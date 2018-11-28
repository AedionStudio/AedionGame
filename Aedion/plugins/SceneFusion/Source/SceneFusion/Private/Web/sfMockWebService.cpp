#include "sfMockWebService.h"
#include "../SceneFusion.h"
#include "../sfConfig.h"

sfMockWebService::sfMockWebService(const FString& host, const FString& port) : sfBaseWebService{}
{
    m_host = host;
    m_port = FCString::Atoi(*port);
}

void sfMockWebService::Logout(OnLogoutDelegate onLogout)
{
    // Simulate a 1 second request delay
    FTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateLambda([this, onLogout](float time)->bool {
            HandleLogoutResponse("", onLogout);
            return false; // returning true will reschedule this delegate.
        }),
        1.0f // delay period
    );
}

void sfMockWebService::Login(const FString& email, const FString& pass, OnLoginDelegate onLogin)
{
    if (m_isLoggedIn || m_isLoggingIn){
        return;
    }

    if (email.IsEmpty() || pass.IsEmpty()) {
        HandleLoginResponse(email, pass, nullptr, "Missing email and/or password", onLogin);
        return;
    }
    m_isLoggingIn = true;

    // Create JSON request data
    TSharedPtr<FJsonObject> jsonPtr = MakeShareable(new FJsonObject);
    jsonPtr->SetStringField("token", pass);
    jsonPtr->SetStringField("name", email);

    // Simulate a 1 second request delay
    FTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateLambda([this, email, pass, jsonPtr, onLogin](float time)->bool {
            HandleLoginResponse(email, pass, jsonPtr, "", onLogin);
            return false; // returning true will reschedule this delegate.
        }),
        1.0f // delay period
    );
}

void sfMockWebService::Authenticate(OnLoginDelegate onLogin)
{
    if (m_isLoggedIn || m_isLoggingIn) {
        return;
    }

    sfConfig& settings = sfConfig::Get();
    if (settings.Email.IsEmpty() || settings.Token.IsEmpty()) {
        HandleLoginResponse(settings.Email, settings.Token, nullptr, "Missing email and/or token.", onLogin);
    }
    else {
        Login(settings.Email, settings.Token, onLogin);
    }
}

void sfMockWebService::GetSessions(OnGetSessionsDelegate onGetSessions)
{
    if (!m_isLoggedIn || m_isFetchingSessions) {
        return;
    }
    m_isFetchingSessions = true;

    // Create JSON useage data
    TSharedPtr<FJsonObject> jsonUsagePtr = MakeShareable(new FJsonObject);
    jsonUsagePtr->SetNumberField("object limit", -1);
    jsonUsagePtr->SetNumberField("session limit", -1);
    jsonUsagePtr->SetNumberField("user limit", -1);
    jsonUsagePtr->SetNumberField("sessions", 0);
    jsonUsagePtr->SetNumberField("users", 0);

    // Create JSON room info
    TSharedPtr<FJsonObject> jsonRoomInfoPtr = MakeShareable(new FJsonObject);
    jsonRoomInfoPtr->SetNumberField("id", 1);
    jsonRoomInfoPtr->SetStringField("scene", "Test");
    jsonRoomInfoPtr->SetStringField("room", "SceneFusion");
    jsonRoomInfoPtr->SetStringField("ip", m_host);
    jsonRoomInfoPtr->SetNumberField("port", m_port);

    // Create JSON session data
    TSharedPtr<FJsonObject> jsonSessionPtr = MakeShareable(new FJsonObject);
    jsonSessionPtr->SetStringField("creator", "You");
    jsonSessionPtr->SetStringField("instanceName", "Test Level");
    jsonSessionPtr->SetBoolField("canDelete", false);
    jsonSessionPtr->SetStringField("version", "2.0");
    jsonSessionPtr->SetObjectField("roomInfo", jsonRoomInfoPtr);
    
    TSharedPtr<FJsonObject> jsonSessionListPtr = MakeShareable(new FJsonObject);
    jsonSessionListPtr->SetObjectField("1", jsonSessionPtr);

    // Create JSON project data
    TSharedPtr<FJsonObject> jsonProjectPtr = MakeShareable(new FJsonObject);
    jsonProjectPtr->SetObjectField("usagelimits", jsonUsagePtr);
    jsonProjectPtr->SetObjectField("sessions", jsonSessionListPtr);

    // Create JSON data
    TSharedPtr<FJsonObject> jsonPtr = MakeShareable(new FJsonObject);
    jsonPtr->SetObjectField("Test Project", jsonProjectPtr);

    // Simulate a 1 second request delay
    FTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateLambda([this, jsonPtr, onGetSessions](float time)->bool {
            HandleGetSessionsResponse(jsonPtr, "", onGetSessions);
            return false; // returning true will reschedule this delegate.
        }),
        1.0f // delay period
    );
}

void sfMockWebService::StartSession(const FString& company, const FString& project, OnStartSessionDelegate onStartSession)
{
    if (!m_isLoggedIn || m_isStartingSession) {
        return;
    }
    m_isStartingSession = true;

    // Create JSON room info
    TSharedPtr<FJsonObject> jsonRoomInfoPtr = MakeShareable(new FJsonObject);
    jsonRoomInfoPtr->SetNumberField("id", 1);
    jsonRoomInfoPtr->SetStringField("scene", "SceneFusion Test");
    jsonRoomInfoPtr->SetStringField("room", "SceneFusion");
    jsonRoomInfoPtr->SetStringField("ip", m_host);
    jsonRoomInfoPtr->SetNumberField("port", m_port);

    // Create JSON session data
    TSharedPtr<FJsonObject> jsonSessionPtr = MakeShareable(new FJsonObject);
    jsonSessionPtr->SetStringField("creator", "You");
    jsonSessionPtr->SetStringField("instanceName", "Test Level");
    jsonSessionPtr->SetBoolField("canDelete", false);
    jsonSessionPtr->SetStringField("version", "2.0");
    jsonSessionPtr->SetObjectField("roomInfo", jsonRoomInfoPtr);

    // Create JSON project session list
    TSharedPtr<FJsonObject> jsonProjectPtr = MakeShareable(new FJsonObject);
    jsonProjectPtr->SetObjectField("1", jsonSessionPtr);

    // Create JSON data
    TSharedPtr<FJsonObject> jsonPtr = MakeShareable(new FJsonObject);
    jsonPtr->SetObjectField("Test Project", jsonProjectPtr);

    // Simulate a 1 second request delay
    FTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateLambda([this, jsonPtr, onStartSession](float time)->bool {
            HandleStartSessionResponse(jsonPtr, "", onStartSession);
            return false; // returning true will reschedule this delegate.
        }),
        1.0f // delay period
    );
}


void sfMockWebService::StopSession(uint32_t roomId, OnStopSessionDelegate onStopSession)
{
    if (!m_isLoggedIn || m_isStoppingSession) {
        return;
    }
    m_isStoppingSession = true;

    // Simulate a 1 second request delay
    FTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateLambda([this, roomId, onStopSession](float time)->bool {
            HandleStopSessionResponse(roomId, "", onStopSession);
            return false; // returning true will reschedule this delegate.
        }),
        1.0f // delay period
    );
}


void sfMockWebService::SetUserColor(const FString& company, const FLinearColor& color, OnSetUserColorDelegate onSetUserColor)
{
    if (!m_isLoggedIn && m_isSettingUserColor) {
        return;
    }
    m_isSettingUserColor = true;

    // Simulate a 1 second request delay
    FTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateLambda([this, company, color, onSetUserColor](float time)->bool {
            HandleSetUserColorResponse(company, color, "", onSetUserColor);
            return false; // returning true will reschedule this delegate.
        }),
        1.0f // delay period
    );
}

void sfMockWebService::RefreshToken(OnRefreshTokenDelegate onRefreshToken)
{
    if (!m_isLoggedIn && m_isRefreshingToken) {
        return;
    }
    m_isRefreshingToken = true;

    // Simulate a 1 second request delay
    FTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateLambda([this, onRefreshToken](float time)->bool {
            HandleRefreshTokenResponse("", onRefreshToken);
            return false; // returning true will reschedule this delegate.
        }),
        1.0f // delay period
     );
}