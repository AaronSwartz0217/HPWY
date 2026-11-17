#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_MapPortal.generated.h"
class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class ACC_PlayerCharacter;

UCLASS(Blueprintable)
class CRASHCOURSE_API ACC_MapPortal : public AActor
{
	GENERATED_BODY()
public:
	ACC_MapPortal();
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UBoxComponent> Trigger;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Marker;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Travel") FName DestinationRegion=TEXT("Town");
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Travel") bool bShowMarker=true;
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Travel",meta=(ClampMin="0")) float ArmDelay=1.f;
// ArmDelay is startup protection; TravelDelay is the visible departure sequence.
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Travel|Departure",meta=(ClampMin="0",Units="s")) float TravelDelay=0.f;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Travel|Departure") TObjectPtr<UNiagaraSystem> DepartureEffect;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Travel|Departure") FTransform DepartureEffectTransform;
    UFUNCTION(BlueprintPure,Category="Travel|Departure") bool IsDeparturePending() const { return bDeparturePending; }
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Travel|Departure") bool bTemporaryPortal=false;
    bool BeginSummonedDeparture(ACC_PlayerCharacter* Character);
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    friend class UCC_AdventureGameInstance;
    friend class FCC_PortalDepartureTimingTest;
    bool bDeparturePending=false;
    FTimerHandle DepartureTimer;
    TWeakObjectPtr<ACC_PlayerCharacter> PendingCharacter;
    TWeakObjectPtr<UNiagaraComponent> ActiveDepartureEffect;
    void StartDeparture(ACC_PlayerCharacter* Character);
    void CompleteDeparture();
    void CancelDeparture();
	float ArmedAt=0.f;
	float RetryAt=0.f;
	UFUNCTION() void Touch(AActor* OverlappedActor,AActor* OtherActor);
};
