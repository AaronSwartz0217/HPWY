#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "CC_ArtifactComponent.generated.h"

class UCC_FormDefinition;
class ACC_BaseCharacter;
class UStaticMeshComponent;
class ACC_FormPowerOrb;
class UNiagaraSystem;
class UMaterialInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCC_ArtifactInteracted, ACC_BaseCharacter*, Character);

// Add to any replicated scene Actor blueprint. No reparenting or string-tag mapping required.
UCLASS(ClassGroup=(Crash), meta=(BlueprintSpawnableComponent))
class CRASHCOURSE_API UCC_ArtifactComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UCC_ArtifactComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Artifact") TObjectPtr<UCC_FormDefinition> Form;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Artifact", meta=(ClampMin="1")) float InteractionDistance = 250.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Artifact") bool bEnabled = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Artifact", meta=(ClampMin="0")) int32 ExperienceReward = 50;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Artifact|Presentation") bool bUseFormPalette = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Artifact|Presentation") FLinearColor PowerColor = FLinearColor(.02f,.2f,.9f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Artifact|Presentation") FLinearColor AccentColor = FLinearColor(1.f,.7f,.04f);
	// Optional Niagara decoration; the native colored sphere needs no new asset import.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Artifact|Presentation") TObjectPtr<UNiagaraSystem> PowerOrbSystem;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Consumed, Category="Artifact") bool bConsumed = false;
	UPROPERTY(BlueprintAssignable) FCC_ArtifactInteracted OnInteracted;
	UFUNCTION(BlueprintPure, Category="Artifact") bool CanInteract(ACC_BaseCharacter* Character) const;
	UFUNCTION(BlueprintPure, Category="Artifact|Presentation") void GetPowerColors(FLinearColor& OutPrimary, FLinearColor& OutAccent) const;
	// Called by the validated progression interaction, never an unchecked client RPC.
	bool ReleasePower(class ACC_PlayerCharacter* Character);
	void HandlePowerOrbFinished(ACC_FormPowerOrb* Orb, bool bCollected);
private:
	UPROPERTY() TWeakObjectPtr<ACC_FormPowerOrb> ActivePowerOrb;
	UFUNCTION() void OnRep_Consumed();
};

UCLASS()
class CRASHCOURSE_API ACC_FormArtifact : public AActor
{
	GENERATED_BODY()
public:
	ACC_FormArtifact();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> ColorBand;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Artifact|Presentation") TObjectPtr<UMaterialInterface> TintMaterial;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UCC_ArtifactComponent> Artifact;
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Artifact|Presentation") void RefreshPowerPresentation();
};
