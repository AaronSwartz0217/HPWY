#pragma once

#include "CoreMinimal.h"
#include "Progression/CC_ExperienceOrb.h"
#include "CC_FormPowerOrb.generated.h"

class UCC_FormDefinition;
class UCC_ArtifactComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UPointLightComponent;

// One physical pickup carries both the form token and its experience reward.
// Scatter, homing, recipient validation and capsule contact reuse the XP orb.
UCLASS(Blueprintable)
class CRASHCOURSE_API ACC_FormPowerOrb : public ACC_ExperienceOrb
{
	GENERATED_BODY()
public:
	ACC_FormPowerOrb();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Power") TObjectPtr<UStaticMeshComponent> Core;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Power") TObjectPtr<UStaticMeshComponent> Accent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Power") TObjectPtr<UPointLightComponent> Glow;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Power") TObjectPtr<UMaterialInterface> PowerMaterial;
	UPROPERTY(BlueprintReadOnly, Replicated) TObjectPtr<UCC_FormDefinition> RewardForm;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_PowerColors) FLinearColor PowerColor;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_PowerColors) FLinearColor AccentColor;
	// Server only; restores the token if an uncollected orb is destroyed/expired.
	UPROPERTY() TWeakObjectPtr<UCC_ArtifactComponent> SourceArtifact;
protected:
	virtual bool GrantReward() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	UFUNCTION() void OnRep_PowerColors();
	float VisualAge=0.f;
};
