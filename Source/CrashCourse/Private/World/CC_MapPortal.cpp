#include "World/CC_MapPortal.h"
#include "World/CC_AdventureGameInstance.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

ACC_MapPortal::ACC_MapPortal()
{
	Trigger=CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));SetRootComponent(Trigger);
	Trigger->SetBoxExtent(FVector(90,90,130));Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionObjectType(ECC_WorldDynamic);Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);Trigger->SetGenerateOverlapEvents(true);Trigger->SetCanEverAffectNavigation(false);
	Marker=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));Marker->SetupAttachment(Trigger);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Marker->SetStaticMesh(Cube.Object);Marker->SetRelativeScale3D(FVector(1.4f,1.4f,.1f));Marker->SetRelativeLocation(FVector(0,0,-100));
	Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);Marker->SetCanEverAffectNavigation(false);
	Label=CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));Label->SetupAttachment(Trigger);
	Label->SetRelativeLocation(FVector(0,0,140));Label->SetHorizontalAlignment(EHTA_Center);Label->SetWorldSize(28);
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OnActorBeginOverlap.AddDynamic(this,&ThisClass::Touch);
}
void ACC_MapPortal::BeginPlay()
{
	Super::BeginPlay();ArmedAt=GetWorld()->GetTimeSeconds()+ArmDelay;Marker->SetVisibility(bShowMarker);
	if (auto* Adventure=GetWorld()->GetGameInstance<UCC_AdventureGameInstance>())
		if (const auto* Region=Adventure->FindRegion(DestinationRegion)) Label->SetText(FText::Format(FText::FromString(TEXT("前往 · {0}")),Region->DisplayName));
}
void ACC_MapPortal::Touch(AActor* OverlappedActor,AActor* OtherActor)
{
	const float Now=GetWorld()->GetTimeSeconds();
	if (Now<ArmedAt || Now<RetryAt || bDeparturePending || GetWorld()->GetNetMode()!=NM_Standalone) return;
	auto* Character=Cast<ACC_PlayerCharacter>(OtherActor);if (!Character || !Character->IsAlive()) return;
	if (auto* Adventure=GetWorld()->GetGameInstance<UCC_AdventureGameInstance>())
    {
        if (Adventure->bTravelling || Character->GetController()!=Adventure->GetFirstLocalPlayerController()) return;
        StartDeparture(Character);
    }
}

void ACC_MapPortal::StartDeparture(ACC_PlayerCharacter* Character)
{
    if (bDeparturePending || !IsValid(Character)) return;
    if(auto* Adventure=GetWorld()->GetGameInstance<UCC_AdventureGameInstance>())
        if(!Adventure->ArmPortalDeparture(this,Character))return;
    bDeparturePending=true;
    PendingCharacter=Character;
    if (DepartureEffect)
    {
        const FTransform EffectWorld=DepartureEffectTransform*GetActorTransform();
        ActiveDepartureEffect=UNiagaraFunctionLibrary::SpawnSystemAtLocation(this,DepartureEffect,
            EffectWorld.GetLocation(),EffectWorld.Rotator(),EffectWorld.GetScale3D(),true,true);
    }
    // Do not pause the world or show loading yet: Niagara must visibly advance first.
    if (TravelDelay>0.f)
        GetWorldTimerManager().SetTimer(DepartureTimer,this,&ThisClass::CompleteDeparture,TravelDelay,false);
    else
        CompleteDeparture();
}

bool ACC_MapPortal::BeginSummonedDeparture(ACC_PlayerCharacter* Character)
{
    StartDeparture(Character);return bDeparturePending;
}

void ACC_MapPortal::CompleteDeparture()
{
    ACC_PlayerCharacter* Character=PendingCharacter.Get();
    CancelDeparture();
    RetryAt=GetWorld()->GetTimeSeconds()+1.f;
    // The authority rechecks overlap, life, skills, destination and saving at completion.
    if (IsValid(Character))
        if (auto* Adventure=GetWorld()->GetGameInstance<UCC_AdventureGameInstance>())
            Adventure->RequestPortalTravel(this,Character);
    if(bTemporaryPortal)Destroy();
}

void ACC_MapPortal::CancelDeparture()
{
    GetWorldTimerManager().ClearTimer(DepartureTimer);
    PendingCharacter.Reset();
    bDeparturePending=false;
    if (ActiveDepartureEffect.IsValid()) ActiveDepartureEffect->DeactivateImmediate();
    ActiveDepartureEffect.Reset();
}

void ACC_MapPortal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CancelDeparture();
    Super::EndPlay(EndPlayReason);
}
