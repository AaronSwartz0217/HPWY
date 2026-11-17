#include "Progression/CC_ArtifactComponent.h"
#include "Progression/CC_GameplayData.h"
#include "Characters/CC_BaseCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Progression/CC_FormPowerOrb.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Player/CC_PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

UCC_ArtifactComponent::UCC_ArtifactComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}
void UCC_ArtifactComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass,bConsumed);
}

bool UCC_ArtifactComponent::CanInteract(ACC_BaseCharacter* Character) const
{
	const AActor* ArtifactOwner = GetOwner();
	if (!bEnabled || bConsumed || !IsValid(Character) || !Character->IsAlive() || !IsValid(ArtifactOwner) || ArtifactOwner->IsHidden() || !IsValid(Form) || !Form->bAvailable || !Form->IsComplete()) return false;
	const FBox Bounds=ArtifactOwner->GetComponentsBoundingBox(false);
 const FVector ReachPoint=Bounds.IsValid ? Bounds.GetClosestPointTo(Character->GetActorLocation()) : ArtifactOwner->GetActorLocation();
 if (FVector::DistSquared(Character->GetActorLocation(),ReachPoint)>FMath::Square(InteractionDistance)) return false;
 const FVector SightPoint=Bounds.IsValid ? Bounds.GetCenter() : ArtifactOwner->GetActorLocation();
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);
	FHitResult Hit;
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Character->GetPawnViewLocation(), SightPoint, ECC_Visibility, Params);
	return !bBlocked || Hit.GetActor() == ArtifactOwner;
}
void UCC_ArtifactComponent::GetPowerColors(FLinearColor& OutPrimary,FLinearColor& OutAccent) const
{
	OutPrimary=PowerColor; OutAccent=AccentColor;
	if (!bUseFormPalette || !Form) return;
	if (Form->FormId==TEXT("Artifact_01")) { OutPrimary=FLinearColor(.025f,.22f,.9f); OutAccent=FLinearColor(1.f,.7f,.035f); }
	else if (Form->FormId==TEXT("Artifact_02")) { OutPrimary=FLinearColor(.5f,.24f,.055f); OutAccent=FLinearColor(.95f,.63f,.12f); }
	else if (Form->FormId==TEXT("Native")) { OutPrimary=FLinearColor(.025f,.65f,.08f); OutAccent=FLinearColor(.2f,.95f,.15f); }
}
void UCC_ArtifactComponent::OnRep_Consumed()
{
	if (AActor* Owner=GetOwner()) { Owner->SetActorHiddenInGame(bConsumed); Owner->SetActorEnableCollision(!bConsumed); }
}
bool UCC_ArtifactComponent::ReleasePower(ACC_PlayerCharacter* Character)
{
	AActor* Owner=GetOwner();
	auto* PS=Character ? Character->GetPlayerState<ACC_PlayerState>() : nullptr;
	if (!Owner || !Owner->HasAuthority() || !CanInteract(Character) || !PS || !PS->Progression || !PS->Progression->ArtifactForms.Contains(Form) || ActivePowerOrb.IsValid()) return false;
	const FBox Bounds=Owner->GetComponentsBoundingBox(false);
	const FVector Origin=Bounds.IsValid ? Bounds.GetCenter()+FVector(0,0,Bounds.GetExtent().Z+18.f) : Owner->GetActorLocation()+FVector(0,0,60);
	// Reserve before spawning: repeated E/RPCs cannot create a second reward.
	bConsumed=true; OnRep_Consumed(); Owner->ForceNetUpdate();
	const FTransform Transform(FRotator::ZeroRotator,Origin);
	auto* Orb=GetWorld()->SpawnActorDeferred<ACC_FormPowerOrb>(ACC_FormPowerOrb::StaticClass(),Transform,PS,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Orb) { bConsumed=false; OnRep_Consumed(); Owner->ForceNetUpdate(); return false; }
	Orb->Recipient=PS; Orb->RewardForm=Form; Orb->ExperienceValue=FMath::Max(0,ExperienceReward);
	Orb->SourceArtifact=this; Orb->ParticleSystem=PowerOrbSystem;
	GetPowerColors(Orb->PowerColor,Orb->AccentColor);
	ActivePowerOrb=Orb;
	Orb->FinishSpawning(Transform);
	OnInteracted.Broadcast(Character);
	return true;
}
void UCC_ArtifactComponent::HandlePowerOrbFinished(ACC_FormPowerOrb* Orb,bool bCollected)
{
	AActor* Owner=GetOwner();
	if (!Owner || !Owner->HasAuthority() || ActivePowerOrb.Get(true)!=Orb) return;
	ActivePowerOrb.Reset();
	if (!bCollected) { bConsumed=false; OnRep_Consumed(); Owner->ForceNetUpdate(); }
}

ACC_FormArtifact::ACC_FormArtifact()
{
	bReplicates = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	Artifact = CreateDefaultSubobject<UCC_ArtifactComponent>(TEXT("Artifact"));
	ColorBand=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ColorBand"));
	ColorBand->SetupAttachment(Mesh);
	ColorBand->SetRelativeScale3D(FVector(.16f,1.015f,1.015f));
	ColorBand->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ColorBand->SetCanEverAffectNavigation(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/HPWH/Materials/M_TestTint.M_TestTint"));
	ColorBand->SetStaticMesh(Cube.Object); TintMaterial=Material.Object;
	Tags.Add(TEXT("SpecialArtifact"));
}
void ACC_FormArtifact::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform); RefreshPowerPresentation();
}
void ACC_FormArtifact::BeginPlay()
{
	Super::BeginPlay(); RefreshPowerPresentation();
}
void ACC_FormArtifact::RefreshPowerPresentation()
{
	const bool Available=Artifact && Artifact->Form && Artifact->Form->bAvailable;
	ColorBand->SetVisibility(Available);
	if (!Available || !TintMaterial) return;
	FLinearColor Primary,Accent; Artifact->GetPowerColors(Primary,Accent);
	if (auto* Material=Mesh->CreateDynamicMaterialInstance(0,TintMaterial)) Material->SetVectorParameterValue(TEXT("Tint"),Primary);
	if (auto* Material=ColorBand->CreateDynamicMaterialInstance(0,TintMaterial)) Material->SetVectorParameterValue(TEXT("Tint"),Accent);
}
