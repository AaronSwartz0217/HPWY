#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CC_ExperienceBackendSubsystem.generated.h"

class IHttpRequest;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FCC_ExperienceBackendResult, bool, bSuccess, const FString&, CharacterId, int32, Experience, int64, Version, const FString&, Error);

// Explicit snapshot API for a trusted game server (or local development).
// No secrets in assets/config; reads SPIRIT_BACKEND_URL and SPIRIT_BACKEND_TOKEN from the process environment.
UCLASS()
class CRASHCOURSE_API UCC_ExperienceBackendSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	UPROPERTY(BlueprintAssignable, Category="Experience Backend") FCC_ExperienceBackendResult OnCompleted;
	UPROPERTY(BlueprintReadOnly, Category="Experience Backend") bool bRequestInFlight = false;
	UPROPERTY(BlueprintReadOnly, Category="Experience Backend") bool bHasPendingSave = false;
	UFUNCTION(BlueprintCallable, Category="Experience Backend") void LoadCharacterExperience(const FString& CharacterId);
	UFUNCTION(BlueprintCallable, Category="Experience Backend") void SaveCharacterExperience(const FString& CharacterId, int32 Experience);
	// Retry the same payload/request ID after a timeout or uncertain server error.
	UFUNCTION(BlueprintCallable, Category="Experience Backend") void RetryPendingSave();
private:
	FString BaseUrl;
	FString Token;
	FString PendingCharacterId;
	FString PendingBody;
	TMap<FString, int64> Versions;
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ActiveRequest;
	bool CanRequest(const FString& CharacterId);
	void Send(const FString& CharacterId, bool bSave, const FString& Body);
	void Fail(const FString& CharacterId, const FString& Error);
};
