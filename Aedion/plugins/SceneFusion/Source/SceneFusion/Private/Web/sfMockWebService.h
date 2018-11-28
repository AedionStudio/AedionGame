#pragma once

#include "sfBaseWebService.h"

/**
 * Scene Fusion mock web api handler.
 */
class sfMockWebService : public sfBaseWebService
{
public:
    /**
     * Constructor
     *
     * @param   const FString& - server address
     * @param   const FString& - server port
     */
    sfMockWebService(const FString& host, const FString& port);

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
    FString m_host;
    uint16 m_port;
};