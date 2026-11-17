#include "Backend/CC_ExperienceBackendSubsystem.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/World.h"

void UCC_ExperienceBackendSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	BaseUrl = FPlatformMisc::GetEnvironmentVariable(TEXT("SPIRIT_BACKEND_URL"));
	Token = FPlatformMisc::GetEnvironmentVariable(TEXT("SPIRIT_BACKEND_TOKEN"));
	BaseUrl.RemoveFromEnd(TEXT("/"));
}
void UCC_ExperienceBackendSubsystem::Deinitialize()
{
	if (ActiveRequest) { ActiveRequest->OnProcessRequestComplete().Unbind(); ActiveRequest->CancelRequest(); ActiveRequest.Reset(); }
	Token.Empty(); bRequestInFlight = false;
	Super::Deinitialize();
}
void UCC_ExperienceBackendSubsystem::Fail(const FString& CharacterId, const FString& Error)
{
	OnCompleted.Broadcast(false, CharacterId, 0, 0, Error);
}
bool UCC_ExperienceBackendSubsystem::CanRequest(const FString& CharacterId)
{
	if (!GetWorld() || GetWorld()->GetNetMode() == NM_Client) { Fail(CharacterId, TEXT("trusted_server_only")); return false; }
	if (bRequestInFlight) { Fail(CharacterId, TEXT("request_in_flight")); return false; }
	if (Token.Len() < 32 || !(BaseUrl.StartsWith(TEXT("http://")) || BaseUrl.StartsWith(TEXT("https://")))) { Fail(CharacterId, TEXT("backend_not_configured")); return false; }
	if (CharacterId.IsEmpty() || CharacterId.Len() > 64) { Fail(CharacterId, TEXT("invalid_character_id")); return false; }
	for (TCHAR C : CharacterId)
		if (!((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9') || C == '-' || C == '_')) { Fail(CharacterId, TEXT("invalid_character_id")); return false; }
	return true;
}
void UCC_ExperienceBackendSubsystem::LoadCharacterExperience(const FString& CharacterId)
{
	if (!CanRequest(CharacterId)) return;
	if (bHasPendingSave) { Fail(CharacterId, TEXT("retry_pending_save_first")); return; }
	Send(CharacterId, false, FString());
}
void UCC_ExperienceBackendSubsystem::SaveCharacterExperience(const FString& CharacterId, int32 Experience)
{
	if (!CanRequest(CharacterId)) return;
	if (bHasPendingSave) { Fail(CharacterId, TEXT("retry_pending_save_first")); return; }
	const int64* Version = Versions.Find(CharacterId);
	if (!Version) { Fail(CharacterId, TEXT("load_before_save")); return; }
	if (Experience < 0) { Fail(CharacterId, TEXT("invalid_experience")); return; }
	TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetNumberField(TEXT("experience"), Experience);
	Json->SetNumberField(TEXT("expectedVersion"), static_cast<double>(*Version));
	Json->SetStringField(TEXT("requestId"), FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens));
	PendingCharacterId = CharacterId;
	PendingBody.Empty();
	FJsonSerializer::Serialize(Json, TJsonWriterFactory<>::Create(&PendingBody));
	bHasPendingSave = true;
	Send(CharacterId, true, PendingBody);
}
void UCC_ExperienceBackendSubsystem::RetryPendingSave()
{
	if (!CanRequest(PendingCharacterId)) return;
	if (!bHasPendingSave) { Fail(PendingCharacterId, TEXT("no_pending_save")); return; }
	Send(PendingCharacterId, true, PendingBody);
}
void UCC_ExperienceBackendSubsystem::Send(const FString& CharacterId, bool bSave, const FString& Body)
{
	bRequestInFlight = true;
	ActiveRequest = FHttpModule::Get().CreateRequest();
	ActiveRequest->SetURL(BaseUrl + TEXT("/v1/characters/") + CharacterId + TEXT("/experience"));
	ActiveRequest->SetVerb(bSave ? TEXT("PUT") : TEXT("GET"));
	ActiveRequest->SetHeader(TEXT("Authorization"), TEXT("Bearer ") + Token);
	ActiveRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	ActiveRequest->SetTimeout(10.f);
	if (bSave) ActiveRequest->SetContentAsString(Body);
	ActiveRequest->OnProcessRequestComplete().BindWeakLambda(this, [this, CharacterId, bSave](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
	{
		bRequestInFlight = false; ActiveRequest.Reset();
		if (!bConnected || !Response.IsValid()) { Fail(CharacterId, TEXT("network_error_retry_same_save")); return; }
		const int32 Code = Response->GetResponseCode();
		if (!bSave && Code == 404)
		{
			TSharedPtr<FJsonObject> ErrorJson;
			FString ErrorCode;
			if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Response->GetContentAsString()), ErrorJson) && ErrorJson.IsValid()
				&& ErrorJson->TryGetStringField(TEXT("error"), ErrorCode) && ErrorCode == TEXT("character_not_found"))
			{ Versions.Add(CharacterId, 0); OnCompleted.Broadcast(true, CharacterId, 0, 0, FString()); return; }
		}
		if (Code != 200)
		{
			// Definitive rejection: reload before attempting a new save. 5xx remains retryable.
			if (bSave && Code >= 400 && Code < 500) { bHasPendingSave = false; PendingBody.Empty(); Versions.Remove(CharacterId); }
			Fail(CharacterId, FString::Printf(TEXT("http_%d"), Code)); return;
		}
		TSharedPtr<FJsonObject> Json;
		double Experience = -1, Version = -1;
		FString ReturnedId;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Response->GetContentAsString()), Json) || !Json.IsValid()
			|| !Json->TryGetStringField(TEXT("characterId"), ReturnedId) || ReturnedId != CharacterId
			|| !Json->TryGetNumberField(TEXT("experience"), Experience) || !FMath::IsFinite(Experience) || Experience < 0 || Experience > MAX_int32 || Experience != FMath::FloorToDouble(Experience)
			|| !Json->TryGetNumberField(TEXT("version"), Version) || !FMath::IsFinite(Version) || Version < 1 || Version > 9007199254740991.0 || Version != FMath::FloorToDouble(Version))
		{ Fail(CharacterId, TEXT("invalid_backend_response")); return; }
		Versions.Add(CharacterId, static_cast<int64>(Version));
		if (bSave) { bHasPendingSave = false; PendingBody.Empty(); PendingCharacterId.Empty(); }
		OnCompleted.Broadcast(true, CharacterId, static_cast<int32>(Experience), static_cast<int64>(Version), FString());
	});
	if (!ActiveRequest->ProcessRequest()) { bRequestInFlight = false; ActiveRequest.Reset(); Fail(CharacterId, TEXT("request_not_started")); }
}
