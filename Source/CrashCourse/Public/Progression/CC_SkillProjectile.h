#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_SkillProjectile.generated.h"
class USphereComponent;
class UProjectileMovementComponent;
class UCC_SkillDefinition;
class UNiagaraComponent;
class UStaticMeshComponent;

UCLASS()
class CRASHCOURSE_API ACC_SkillProjectile : public AActor
{
	GENERATED_BODY()
public:
	ACC_SkillProjectile();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UProjectileMovementComponent> Movement;
	void Initialize(UCC_SkillDefinition* Skill, float Damage, AActor* Target);
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;
private:
 UPROPERTY() TObjectPtr<UStaticMeshComponent> VisibleCore;
	UPROPERTY(ReplicatedUsing=OnRep_Presentation) TObjectPtr<UCC_SkillDefinition> Definition;
	UPROPERTY() TObjectPtr<UNiagaraComponent> Trail;
	UFUNCTION() void OnRep_Presentation();
	UPROPERTY() TSet<TObjectPtr<AActor>> HitActors;
	float Power = 0.f;
	int32 RemainingHits = 1;
};
