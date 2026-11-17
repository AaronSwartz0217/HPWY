#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_ExperienceOrb.generated.h"

class ACC_PlayerState;
class ACC_PlayerCharacter;
class USphereComponent;
class UNiagaraComponent;
class UNiagaraSystem;

UENUM(BlueprintType)
enum class ECC_ExperienceOrbPhase : uint8 { Scattering, Seeking, Collected };

// The server owns movement/collection. Niagara renders one orb per actor.
UCLASS(Blueprintable)
class CRASHCOURSE_API ACC_ExperienceOrb : public AActor
{
	GENERATED_BODY()
public:
	ACC_ExperienceOrb();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UNiagaraComponent> Particles;
	UPROPERTY(BlueprintReadOnly, Replicated) TObjectPtr<ACC_PlayerState> Recipient;
	UPROPERTY(BlueprintReadOnly, Replicated) int32 ExperienceValue = 1;
	UPROPERTY(BlueprintReadOnly, Replicated) ECC_ExperienceOrbPhase Phase = ECC_ExperienceOrbPhase::Scattering;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experience", meta=(ClampMin="0")) float ScatterDuration = 0.45f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experience", meta=(ClampMin="1")) float InitialSeekSpeed = 350.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experience", meta=(ClampMin="1")) float MaximumSeekSpeed = 1800.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experience", meta=(ClampMin="1")) float SeekAcceleration = 1600.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experience", meta=(ClampMin="1")) float MaximumLifetime = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ParticleSystem, Category="Experience") TObjectPtr<UNiagaraSystem> ParticleSystem;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Experience") void BeginSeeking();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Experience") bool TryCollect(ACC_PlayerCharacter* Character);
	// Returns spawned particle actors. Amounts above 128 share value across particles.
	UFUNCTION(BlueprintCallable, Category="Experience", meta=(WorldContext="WorldContextObject"))
	static int32 SpawnBurst(UObject* WorldContextObject, ACC_PlayerState* InRecipient, FVector Origin, int32 ExperienceAmount, UNiagaraSystem* InParticleSystem = nullptr);
protected:
	// Called with collection already reserved, so reward callbacks cannot collect twice.
	virtual bool GrantReward();
	FVector LaunchVelocity = FVector::ZeroVector;
private:
	UFUNCTION() void OnRep_ParticleSystem();
	float Age = 0.f;
	float SeekSpeed = 350.f;
};
