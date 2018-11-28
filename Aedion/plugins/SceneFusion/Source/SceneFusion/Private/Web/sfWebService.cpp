#include "sfWebService.h"
#include "../SceneFusion.h"
#include "../sfConfig.h"

#include <Runtime/Online/HTTP/Public/Http.h>
#include <Runtime/Launch/Resources/Version.h>

#define LOG_CHANNEL "sfWebService"

sfWebService::sfWebService() : sfBaseWebService{}
{
}

void sfWebService::Logout(OnLogoutDelegate onLogout)
{
    if (!m_isLoggedIn)
    {
        return;
    }

    // Create JSON request data
    sfConfig& settings = sfConfig::Get();
    TSharedPtr<FJsonObject> jsonObjectPtr = MakeShareable(new FJsonObject);
    jsonObjectPtr->SetStringField("email", settings.Email);
    
    Send(settings.ServiceURL / "v2/logout", "POST", jsonObjectPtr,
        OnResponseDelegate::CreateLambda(
            [this, onLogout](TSharedPtr<FJsonObject> resultPtr, const FString& error) {
                HandleLogoutResponse(error, onLogout);
            }
        )
    );
}

void sfWebService::Login(const FString& email, const FString& pass, OnLoginDelegate onLogin)
{
    if (m_isLoggedIn || m_isLoggingIn)
    {
        return;
    }
    
    // Create JSON request data
    sfConfig& settings = sfConfig::Get();
    TSharedPtr<FJsonObject> jsonObjectPtr = MakeShareable(new FJsonObject);
    jsonObjectPtr->SetStringField("email", email);
    jsonObjectPtr->SetStringField("password", pass);
    m_isLoggingIn = true;

    Send(settings.ServiceURL / "v2/login", "POST", jsonObjectPtr,
        OnResponseDelegate::CreateLambda(
            [this, email, onLogin](TSharedPtr<FJsonObject> resultPtr, const FString& error) {
                if (!error.IsEmpty()) {
                    HandleLoginResponse(email, "", resultPtr, error, onLogin);
                }
                else {
                    FetchSFToken(email, resultPtr->GetStringField("token"), onLogin);
                }
            }
        )
    );
}

void sfWebService::Authenticate(OnLoginDelegate onLogin)
{
    if (m_isLoggedIn || m_isLoggingIn) {
        return;
    }

    sfConfig& settings = sfConfig::Get();
    m_isLoggingIn = true;

    if (settings.Email.IsEmpty() || settings.Token.IsEmpty()) {
        HandleLoginResponse(settings.Email, settings.Token, nullptr, "Missing email and/or token.", onLogin);
    }
    else {
        FetchSFToken(settings.Email, settings.Token, onLogin);
    }
}

void sfWebService::GetSessions(OnGetSessionsDelegate onGetSessions)
{
    if (!m_isLoggedIn || m_isFetchingSessions) {
        return;
    }

    sfConfig& settings = sfConfig::Get();
    TSharedPtr<FJsonObject> jsonObjectPtr = MakeShareable(new FJsonObject);
    jsonObjectPtr->SetStringField("token", settings.SFToken);
    jsonObjectPtr->SetStringField("version", "2.0.2");
    m_isFetchingSessions = true;

    Send(settings.ServiceURL / "v1/getSessions", "POST", jsonObjectPtr,
        OnResponseDelegate::CreateLambda(
            [this, onGetSessions](TSharedPtr<FJsonObject> resultPtr, const FString& error) {
                //LogJSON(resultPtr);
                HandleGetSessionsResponse(resultPtr, error, onGetSessions);
            }
        )
    );
}


void sfWebService::StartSession(const FString& company, const FString& project, OnStartSessionDelegate onStartSession)
{
    if (!m_isLoggedIn || m_isStartingSession) {
        return;
    }

    sfConfig& settings = sfConfig::Get();
    TSharedPtr<FJsonObject> jsonObjectPtr = MakeShareable(new FJsonObject);
    jsonObjectPtr->SetStringField("token", settings.SFToken);
    jsonObjectPtr->SetStringField("version", "2.0.2");
    jsonObjectPtr->SetStringField("company", company);
    jsonObjectPtr->SetStringField("scene", project);
    m_isStartingSession = true;

    Send(settings.ServiceURL / "v1/startsession", "POST", jsonObjectPtr,
        OnResponseDelegate::CreateLambda(
            [this, onStartSession](TSharedPtr<FJsonObject> resultPtr, const FString& error) {
                //LogJSON(resultPtr);
                HandleStartSessionResponse(resultPtr, error, onStartSession);
            }
        )
    );
}


void sfWebService::StopSession(uint32 roomId, OnStopSessionDelegate onStopSession)
{
    if (!m_isLoggedIn || m_isStoppingSession) {
        return;
    }

    sfConfig& settings = sfConfig::Get();
    TSharedPtr<FJsonObject> jsonObjectPtr = MakeShareable(new FJsonObject);
    jsonObjectPtr->SetStringField("token", settings.SFToken);
    jsonObjectPtr->SetNumberField("session", roomId);
    m_isStoppingSession = true;

    Send(settings.ServiceURL / "v1/stopSession", "POST", jsonObjectPtr,
        OnResponseDelegate::CreateLambda(
            [this, roomId, onStopSession](TSharedPtr<FJsonObject> resultPtr, const FString& error) {
                //LogJSON(resultPtr);
                HandleStopSessionResponse(roomId, error, onStopSession);
            }
        )
    );
}


void sfWebService::SetUserColor(const FString& company, const FLinearColor& color, OnSetUserColorDelegate onSetUserColor)
{
    if (!m_isLoggedIn && m_isSettingUserColor) {
        return;
    }

    sfConfig& settings = sfConfig::Get();
    TSharedPtr<FJsonObject> jsonObjectPtr = MakeShareable(new FJsonObject);
    jsonObjectPtr->SetStringField("token", settings.SFToken);
    jsonObjectPtr->SetStringField("company", company);
    jsonObjectPtr->SetStringField("newColor", color.ToFColor(true).ToHex());   
    m_isSettingUserColor = true;

    Send(settings.ServiceURL / "v1/setColor", "POST", jsonObjectPtr,
        OnResponseDelegate::CreateLambda(
            [this, company, color, onSetUserColor](TSharedPtr<FJsonObject> resultPtr, const FString& error) {
                //LogJSON(resultPtr);
                HandleSetUserColorResponse(company, color, error, onSetUserColor);
            }
        )
    );
}

void sfWebService::RefreshToken(OnRefreshTokenDelegate onRefreshToken)
{
    if (!m_isLoggedIn && m_isRefreshingToken) {
        return;
    }

    sfConfig& settings = sfConfig::Get();
    TSharedPtr<FJsonObject> jsonObjectPtr = MakeShareable(new FJsonObject);
    jsonObjectPtr->SetStringField("token", settings.SFToken);

    m_isRefreshingToken = true;
    Send(settings.ServiceURL / "v1/refreshToken", "POST", jsonObjectPtr,
        OnResponseDelegate::CreateLambda(
            [this, onRefreshToken](TSharedPtr<FJsonObject> resultPtr, const FString& error) {
                //LogJSON(resultPtr);
                HandleRefreshTokenResponse(error, onRefreshToken);
            }
        )
    );
}

void sfWebService::FetchSFToken(const FString& email, const FString& token, OnLoginDelegate onLogin)
{
    sfConfig& settings = sfConfig::Get();
    TSharedPtr<FJsonObject> jsonObjectPtr = MakeShareable(new FJsonObject);
    jsonObjectPtr->SetStringField("email", email);
    jsonObjectPtr->SetStringField("token", token);

    Send(settings.ServiceURL / "v2/SceneFusionLogin", "POST", jsonObjectPtr,
        OnResponseDelegate::CreateLambda(
            [this, email, token, onLogin](TSharedPtr<FJsonObject> resultPtr, const FString& error) {
                //LogJSON(resultPtr);
                HandleLoginResponse(email, token, resultPtr, error, onLogin);
            }
        )
    );
}

void sfWebService::Send(
    const FString& url, 
    const FString& verb, 
    TSharedPtr<FJsonObject> jsonDataPtr, 
    OnResponseDelegate onComplete)
{
    jsonDataPtr->SetStringField("application", FString::Printf(TEXT("Unreal Editor %d.%d.%d"),
        ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION, ENGINE_PATCH_VERSION));
    FString jsonString;
    TSharedRef<TJsonWriter<>> jsonWriter = TJsonWriterFactory<>::Create(&jsonString);
    FJsonSerializer::Serialize(jsonDataPtr.ToSharedRef(), jsonWriter);
    //UE_LOG(LogSceneFusion, Log, TEXT("URL: %s\n%s"), *url, *jsonString);

    // HTTPS Post Request
    TSharedRef<IHttpRequest> request = FHttpModule::Get().CreateRequest();
    request->SetURL(url);
    request->SetVerb(verb);
    request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
    request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    request->SetHeader(TEXT("Accepts"), TEXT("application/json"));
    request->SetContentAsString(jsonString);
    request->OnProcessRequestComplete().BindLambda(
        [onComplete](FHttpRequestPtr requestPtr, FHttpResponsePtr responsePtr, bool wasSuccessful) {
            FString error = "";
            TSharedPtr<FJsonObject> resultPtr = nullptr;

            if (!wasSuccessful || !responsePtr.IsValid()) {
                error = "HTTP Request failed";
            }
            else if (!EHttpResponseCodes::IsOk(responsePtr->GetResponseCode())) {
                error =  "HTTP Request failed: " + responsePtr->GetResponseCode();
            }
            else {
                TSharedPtr<FJsonObject> jsonObjectPtr = MakeShareable(new FJsonObject);
                TSharedRef<TJsonReader<>> jsonReader = 
                    TJsonReaderFactory<>::Create(responsePtr->GetContentAsString());
                FJsonSerializer::Deserialize(jsonReader, jsonObjectPtr);

                if (jsonObjectPtr->HasField("err")) {
                    error = jsonObjectPtr->GetStringField("err");
                }
                else if (jsonObjectPtr->HasField("msg")) {
                    resultPtr = jsonObjectPtr->GetObjectField("msg");
                }
                else {
                    KS::Log::Error("Response missing 'msg' or 'err' field:\n "
                        + std::string(TCHAR_TO_UTF8(*responsePtr->GetContentAsString())), LOG_CHANNEL);
                    error = "Unexpected response format, check output logs.";
                }
            }

            onComplete.Execute(resultPtr, error);
        }
    );
    request->ProcessRequest();
}

void sfWebService::LogJSON(TSharedPtr<FJsonObject> jsonPtr)
{
    if (jsonPtr.IsValid()) {
        FString jsonString;
        TSharedRef<TJsonWriter<>> jsonWriter = TJsonWriterFactory<>::Create(&jsonString);
        FJsonSerializer::Serialize(jsonPtr.ToSharedRef(), jsonWriter);
        KS::Log::Debug("  " + std::string(TCHAR_TO_UTF8(*jsonString)), LOG_CHANNEL);
    }
}

#undef LOG_CHANNEL