#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_ExperiencePack.generated.h"

class ACC_PlayerCharacter;
class UStaticMeshComponent;
class UNiagaraSystem;
class UTextRenderComponent;

UCLASS(Blueprintable)
class CRASHCOURSE_API ACC_ExperiencePack : public AActor
{
	GENERATED_BODY()
public:
	ACC_ExperiencePack();
 virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Experience", meta=(ClampMin="1")) int32 ExperienceAmount = 50;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Experience") TObjectPtr<UNiagaraSystem> ExperienceParticleSystem;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Experience", meta=(ClampMin="1")) float InteractionDistance = 250.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Experience") bool bRepeatable = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Experience", meta=(ClampMin="0.1")) float ReuseDelay = 1.f;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Consumed) bool bConsumed = false;
	UFUNCTION(BlueprintPure, Category="Experience") bool CanInteract(ACC_PlayerCharacter* Character) const;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Experience") bool Interact(ACC_PlayerCharacter* Character);
private:
	UPROPERTY(Replicated) float NextUseTime = 0.f;
	UFUNCTION() void OnRep_Consumed();
};
