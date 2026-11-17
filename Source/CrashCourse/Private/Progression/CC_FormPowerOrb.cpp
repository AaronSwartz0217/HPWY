#include "Progression/CC_FormPowerOrb.h"
#include "Progression/CC_ArtifactComponent.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Player/CC_PlayerState.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"

ACC_FormPowerOrb::ACC_FormPowerOrb()
{
	Collision->InitSphereRadius(12.f);
	ExperienceValue=50;
	LaunchVelocity=FVector(0,0,280);
	PowerColor=FLinearColor(.02f,.2f,.9f);
	AccentColor=FLinearColor(1.f,.7f,.04f);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/HPWH/Materials/M_TestTint.M_TestTint"));
	PowerMaterial=Material.Object;
	Core=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Core"));
	Core->SetupAttachment(Collision);
	Core->SetStaticMesh(Sphere.Object);
	Core->SetRelativeScale3D(FVector(.24f));
	Core->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Core->SetCanEverAffectNavigation(false);
	Accent=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Accent"));
	Accent->SetupAttachment(Collision);
	Accent->SetStaticMesh(Sphere.Object);
	Accent->SetRelativeScale3D(FVector(.08f));
	Accent->SetRelativeLocation(FVector(15,0,3));
	Accent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Accent->SetCanEverAffectNavigation(false);
	Glow=CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Collision);
	Glow->SetIntensity(120.f);
	Glow->SetAttenuationRadius(100.f);
	Glow->SetCastShadows(false);
}
void ACC_FormPowerOrb::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass,RewardForm);
	DOREPLIFETIME(ThisClass,PowerColor);
	DOREPLIFETIME(ThisClass,AccentColor);
}
void ACC_FormPowerOrb::BeginPlay()
{
	Super::BeginPlay();
	OnRep_PowerColors();
	// The base disables proxy ticks; these ticks animate visuals only on clients.
	if (GetNetMode()!=NM_DedicatedServer) SetActorTickEnabled(true);
}
void ACC_FormPowerOrb::OnRep_PowerColors()
{
	if (GetNetMode()==NM_DedicatedServer || !PowerMaterial) return;
	if (auto* Material=Core->CreateDynamicMaterialInstance(0,PowerMaterial)) Material->SetVectorParameterValue(TEXT("Tint"),PowerColor);
	if (auto* Material=Accent->CreateDynamicMaterialInstance(0,PowerMaterial)) Material->SetVectorParameterValue(TEXT("Tint"),AccentColor);
	Glow->SetLightColor(PowerColor);
}
void ACC_FormPowerOrb::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsActorBeingDestroyed() || GetNetMode()==NM_DedicatedServer) return;
	VisualAge+=DeltaSeconds;
	const float Angle=VisualAge*4.f;
	Accent->SetRelativeLocation(FVector(15.f*FMath::Cos(Angle),15.f*FMath::Sin(Angle),3.f+4.f*FMath::Sin(Angle*2.f)));
}
bool ACC_FormPowerOrb::GrantReward()
{
	return Recipient && Recipient->Progression && Recipient->Progression->CollectArtifactPower(RewardForm,ExperienceValue);
}
void ACC_FormPowerOrb::EndPlay(const EEndPlayReason::Type Reason)
{
	if (HasAuthority() && Reason==EEndPlayReason::Destroyed && SourceArtifact.IsValid())
		SourceArtifact->HandlePowerOrbFinished(this,Phase==ECC_ExperienceOrbPhase::Collected);
	Super::EndPlay(Reason);
}
