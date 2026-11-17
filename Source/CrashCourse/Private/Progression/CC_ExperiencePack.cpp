#include "Progression/CC_ExperiencePack.h"
#include "Progression/CC_ExperienceOrb.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Player/CC_PlayerState.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

ACC_ExperiencePack::ACC_ExperiencePack()
{
	bReplicates = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Mesh);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 110.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(24.f);
	Label->SetText(FText::FromString(TEXT("50 XP  [E]")));
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void ACC_ExperiencePack::BeginPlay(){Super::BeginPlay();if(Label)Label->SetVisibility(false);}
void ACC_ExperiencePack::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACC_ExperiencePack, bConsumed);
	DOREPLIFETIME(ACC_ExperiencePack, NextUseTime);
}
bool ACC_ExperiencePack::CanInteract(ACC_PlayerCharacter* Character) const
{
	if (bConsumed || !IsValid(Character) || !Character->IsAlive() || !Character->GetPlayerState<ACC_PlayerState>() || GetWorld()->GetTimeSeconds() < NextUseTime) return false;
 const FBox Bounds=GetComponentsBoundingBox(false);
 const FVector ReachPoint=Bounds.IsValid ? Bounds.GetClosestPointTo(Character->GetActorLocation()) : GetActorLocation();
 if (FVector::DistSquared(Character->GetActorLocation(),ReachPoint)>FMath::Square(InteractionDistance)) return false;
 const FVector SightPoint=Bounds.IsValid ? Bounds.GetCenter() : GetActorLocation();
	FHitResult Hit;
	FCollisionQueryParams Params; Params.AddIgnoredActor(Character);
	return !GetWorld()->LineTraceSingleByChannel(Hit, Character->GetPawnViewLocation(), SightPoint, ECC_Visibility, Params) || Hit.GetActor() == this;
}
bool ACC_ExperiencePack::Interact(ACC_PlayerCharacter* Character)
{
	if (!HasAuthority() || !CanInteract(Character) || ExperienceAmount <= 0) return false;
	bConsumed = !bRepeatable;
	NextUseTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.1f, ReuseDelay);
	const FVector Origin = Mesh->Bounds.Origin + FVector(0.f, 0.f, Mesh->Bounds.BoxExtent.Z + 15.f);
	if (ACC_ExperienceOrb::SpawnBurst(this, Character->GetPlayerState<ACC_PlayerState>(), Origin, ExperienceAmount, ExperienceParticleSystem) == 0)
	{
		bConsumed = false;
		NextUseTime = 0.f;
		return false;
	}
	OnRep_Consumed();
	ForceNetUpdate();
	return true;
}
void ACC_ExperiencePack::OnRep_Consumed()
{
	SetActorHiddenInGame(bConsumed);
	Mesh->SetCollisionEnabled(bConsumed ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
}
