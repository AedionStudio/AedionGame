#pragma once

#include "sfBaseWebService.h"

/**
 * Scene Fusion web api handler.
 */
class sfWebService : public sfBaseWebService
{
public:
    sfWebService();

    /**
     * Logout
     *
     * @param   OnLogoutDelegate - response handler
     */
    virtual void Logout(OnLogoutDelegate onLogout);

    /**
     * sBaseWebSerive Interface. Email/Password Login
     *
     * @param   const FString& - email address
     * @param   const FString& - password
     * @param   OnLoginDelegate - response handler
     */
    virtual void Login(const FString& email, const FString& pass, OnLoginDelegate onLogin);

    /**
     * sBaseWebSerive Interface. Email/Token Login
     *
     * @param   OnLoginDelegate - response handler
     */
    virtual void Authenticate(OnLoginDelegate onLogin);

    /**
     * sBaseWebSerive Interface. Get Sessions
     *
     * @param   OnGetSessionsDelegate - response handler
     */
    virtual void GetSessions(OnGetSessionsDelegate onGetSessions);

    /**
     * Start Session
     *
     * @param   const FString& - SF Company and project written as "company/project"
     * @param   const FString& - UE project name
     * @param   OnStartSessionDelegate - response handler
     */
    virtual void StartSession(const FString& company, const FString& project, OnStartSessionDelegate onStartSession);

    /**
     * sBaseWebSerive Interface. Stop Session
     *
     * @param   uint32 - room ID
     * @param   OnStopSessionDelegate - response handler
     */
    virtual void StopSession(uint32 roomId, OnStopSessionDelegate onStartSession);

    /**
     * sBaseWebSerive Interface. Set User Color
     *
     * @param   const FString& - SF Company and project written as "company/project"
     * @param   const FLinearColor& - color
     * @param   OnSetUserColorDelegate - response handler
     */
    virtual void SetUserColor(const FString& company, const FLinearColor& color, OnSetUserColorDelegate onSetUserColor);

    /**
     * sBaseWebSerive Interface. Refresh Token
     *
     * @param   OnRefreshTokenDelegate - response handler
     */
    virtual void RefreshToken(OnRefreshTokenDelegate onRefreshToken);
private:
    /**
     * Handle HTTP Responses
     */
    DECLARE_DELEGATE_TwoParams(OnResponseDelegate, TSharedPtr<FJsonObject>, const FString&);

    /**
     * Send an HTTP request to the SF Web API and parse the response.
     *
     * @param   const FString& - url
     * @param   const FString& - verb (GET/POST)
     * @param   TSharedPtr<FJsonObject> jsonDataPtr - json request data  (Ignored if verb != POST)
     * @param   OnResponseDelegate - HTTP response handle.
     */
    void Send(
        const FString& url, 
        const FString& verb, 
        TSharedPtr<FJsonObject> jsonDataPtr, 
        OnResponseDelegate OnComplete);
    
    /**
     * Send a request for a SceneFusion session token.
     *
     * @param   const FString& - email address
     * @param   const FString& - web authentication token
     * @param   OnLoginDelegate - response handler
     */
    void FetchSFToken(const FString& email, const FString& token, OnLoginDelegate onLogin);

    /**
     * Validate and log json objects
     *
     * TSharedPtr<FJsonObject>
     */
    void LogJSON(TSharedPtr<FJsonObject> jsonPtr);
};