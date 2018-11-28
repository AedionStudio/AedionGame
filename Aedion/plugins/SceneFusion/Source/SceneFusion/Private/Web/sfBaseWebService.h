#pragma once

#include "../sfProjectInfo.h"

#include <Json.h>
#include <JsonUtilities.h>

/**
 * Scene Fusion web api handler.  Provides abstract interface declaractions to be impementated in 
 * sfWebService and sfMockWebService. Contains information required to parse and forward web responses 
 * to the Scene Fusion UI.
 */
class sfBaseWebService
{
public:
    /**
     * Delegate for logout responses.
     */
    DECLARE_DELEGATE(OnLogoutDelegate);

    /**
     * Delegate for login responses.
     *
     * @param   const FString& - error message
     */
    DECLARE_DELEGATE_OneParam(OnLoginDelegate, const FString&);

    /**
     * Delegate for get session responses.
     *
     * @param   ProjectMap& - project information
     * @param   const FString& - error message
     */
    DECLARE_DELEGATE_TwoParams(OnGetSessionsDelegate, ProjectMap&, const FString&);

    /**
     * Delegate for start settsion responses.
     *
     * @param   TSharedPtr<sfSessionInfo> - session information
     * @param   const FString& - error message
     */
    DECLARE_DELEGATE_TwoParams(OnStartSessionDelegate, TSharedPtr<sfSessionInfo>, const FString&);

    /**
     * Delegate for stop session responses.
     *
     * @param   uint32 - id of the room stopped
     * @param   const FString& - error message
     */
    DECLARE_DELEGATE_TwoParams(OnStopSessionDelegate, uint32, const FString&);

    /**
     * Delegate for setting a user UI color responses.
     *
     * @param   FLinearColor - User UI color
     * @param   const FString& - error message
     */
    DECLARE_DELEGATE_TwoParams(OnSetUserColorDelegate, FLinearColor, const FString&);

    /**
     * Delegate for refresh token responses.
     *
     * @param   const FString& - error message
     */
    DECLARE_DELEGATE_OneParam(OnRefreshTokenDelegate, const FString&);

    /**
     * Constructor
     */
    sfBaseWebService() :
            m_isLoggedIn{ false },
            m_isLoggingIn{ false },
            m_isFetchingSessions{ false },
            m_isStartingSession{ false },
            m_isStoppingSession{ false },
            m_isSettingUserColor{ false },
            m_isRefreshingToken{ false }
    {
    }
    virtual ~sfBaseWebService() {}

    /**
     * Logout
     *
     * @param   OnLogoutDelegate - response handler
     */
    virtual void Logout(OnLogoutDelegate onLogout) = 0;

    /**
     * Email/Password Login
     *
     * @param   const FString& - email address
     * @param   const FString& - password
     * @param   OnLoginDelegate - response handler
     */
    virtual void Login(const FString& email, const FString& pass, OnLoginDelegate onLogin) = 0;

    /**
     * @param   OnLoginDelegate - response handler
     */
    virtual void Authenticate(OnLoginDelegate onLogin) = 0;

    /**
     * Get Sessions
     *
     * @param   OnGetSessionsDelegate - response handler
     */
    virtual void GetSessions(OnGetSessionsDelegate onGetSessions) = 0;

    /**
     * Start Session
     *
     * @param   const FString& - SF Company and project written as "company/project"
     * @param   const FString& - UE project name
     * @param   OnStartSessionDelegate - response handler
     */
    virtual void StartSession(const FString& company, const FString& project, OnStartSessionDelegate onStartSession) = 0;

    /**
     * Stop Session
     *
     * @param   uint32 - room ID
     * @param   OnStopSessionDelegate - response handler
     */
    virtual void StopSession(uint32 roomId, OnStopSessionDelegate onStartSession) = 0;

    /**
     * Set User Color
     *
     * @param   const FString& - SF Company and project written as "company/project"
     * @param   const FLinearColor& - color
     * @param   OnSetUserColorDelegate - response handler
     */
    virtual void SetUserColor(const FString& company, const FLinearColor& color, OnSetUserColorDelegate onSetUserColor) = 0;

    /**
     * Refresh Token
     *
     * @param   OnRefreshTokenDelegate - response handler
     */
    virtual void RefreshToken(OnRefreshTokenDelegate onRefreshToken) = 0;

    /**
     * Login Status
     *
     * @return  bool
     */
    bool IsLoggedIn();
    
protected:
    static FString URL;
    bool m_isLoggedIn;

    // Transaction state
    bool m_isLoggingIn;
    bool m_isFetchingSessions;
    bool m_isStartingSession;
    bool m_isStoppingSession;
    bool m_isSettingUserColor;
    bool m_isRefreshingToken;

    /**
     * Handle HTTP login request data and errors.
     *
     * @param   const FString& - response error
     * @param   OnLogoutDelegate - response handler
     */
    void HandleLogoutResponse(const FString& error, OnLogoutDelegate onLogout);

    /**
     * Handle login/authenticate responses
     *
     * @param   const FString& - email
     * @param   const FString& - login token 
     * @param   TSharedPtr<FJsonObject> - json response data
     * @param   const FString& - response error
     * @param   OnLoginDelegate - response handler
     */
    void HandleLoginResponse(
        const FString& email, 
        const FString& loginToken, 
        TSharedPtr<FJsonObject> jsonPtr, 
        const FString& error, 
        OnLoginDelegate onLogin);

    /**
     * Handle HTTP get sessions request data and errors.
     *
     * @param   TSharedPtr<FJsonObject> - json response data
     * @param   const FString& - response error
     * @param   OnGetSessionsDelegate - response handler
     */
    void HandleGetSessionsResponse(
        TSharedPtr<FJsonObject> jsonPtr, 
        const FString& error, 
        OnGetSessionsDelegate onGetSessions);

    /**
     * Handle HTTP start session request data and errors.
     *
     * @param   TSharedPtr<FJsonObject> - json response data
     * @param   const FString& - response error
     * @param   OnStartSessionDelegate - response handler
     */
    void HandleStartSessionResponse(
        TSharedPtr<FJsonObject> jsonPtr, 
        const FString& error, 
        OnStartSessionDelegate onStartSession);

    /**
     * Handle HTTP stop sessions request data and errors.
     *
     * @param   uint32 - room ID
     * @param   const FString& - response error
     * @param   OnStopSessionDelegate - response handler
     */
    void HandleStopSessionResponse(uint32 roomId, const FString& error, OnStopSessionDelegate onStopSession);

    /**
     * Handle HTTP set user color request data and errors.
     *
     * @param   const FString& - company/project
     * @param   const FColor& - color
     * @param   const FString& - response error
     * @param   OnSetUserColorDelegate - response handler
     */
    void HandleSetUserColorResponse(
        const FString& companyProject,
        const FLinearColor& color,
        const FString& error, 
        OnSetUserColorDelegate onSetUserColor);

    /**
     * Handle HTTP refresh token request data and errors.
     *
     * @param   const FString& - response error
     * @param   OnLoginDelegate - response handler
     */
    void HandleRefreshTokenResponse(const FString& error, OnRefreshTokenDelegate onRefreshToken);

    /**
     * Parse session information from a json object.
     *
     * @param   TSharedPtr<FJsonObject> - json response data
     * @return  TSharedPtr<sfSessionInfo> - null if the json data could not be parsed
     */
    TSharedPtr<sfSessionInfo> ParseJsonSession(TSharedPtr<FJsonObject> jsonPtr);

    /**
     * Parse room information from a json object.
     *
     * @param   TSharedPtr<FJsonObject> - json response data
     * @return  ksRoomInfo::SPtr - null if the json data could not be parsed
     */
    ksRoomInfo::SPtr ParseJsonRoom(TSharedPtr<FJsonObject> jsonPtr);
};
