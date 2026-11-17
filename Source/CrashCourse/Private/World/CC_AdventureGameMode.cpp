#include "World/CC_AdventureGameMode.h"
#include "World/CC_AdventureGameInstance.h"
#include "World/CC_HeavenSceneRebuild.h"
#include "World/CC_MapPortal.h"
#include "World/CC_CoinSpawnDirector.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Progression/CC_ProgressionHUD.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

ACC_AdventureGameMode::ACC_AdventureGameMode()
{
	static ConstructorHelpers::FClassFinder<APawn> Player(TEXT("/Game/HPWH/Blueprints/BP_ProgressionCharacter"));
	static ConstructorHelpers::FClassFinder<APlayerController> Controller(TEXT("/Game/HPWH/Blueprints/BP_ProgressionController"));
	static ConstructorHelpers::FClassFinder<APlayerState> State(TEXT("/Game/HPWH/Blueprints/BP_ProgressionPlayerState"));
	static ConstructorHelpers::FClassFinder<AHUD> HUD(TEXT("/Game/UI/MuralMenu/BP_MuralHUD"));
	DefaultPawnClass=Player.Class.Get() ? Player.Class.Get() : ACC_PlayerCharacter::StaticClass();
	PlayerControllerClass=Controller.Class.Get() ? Controller.Class.Get() : ACC_PlayerController::StaticClass();
	PlayerStateClass=State.Class.Get() ? State.Class.Get() : ACC_PlayerState::StaticClass();
	HUDClass=HUD.Class.Get() ? HUD.Class.Get() : ACC_ProgressionHUD::StaticClass();
}
void ACC_AdventureGameMode::InitGame(const FString& MapName,const FString& Options,FString& ErrorMessage)
{
	Super::InitGame(MapName,Options,ErrorMessage);
	for (TActorIterator<APawn> It(GetWorld());It;++It)
		if (!It->IsA<ACC_PlayerCharacter>() && It->AutoPossessPlayer!=EAutoReceiveInput::Disabled)
		{It->Tags.AddUnique(TEXT("CC_LegacyPlayer"));It->AutoPossessPlayer=EAutoReceiveInput::Disabled;It->SetActorHiddenInGame(true);It->SetActorEnableCollision(false);It->SetActorTickEnabled(false);}
}
void ACC_AdventureGameMode::StartPlay()
{
	ConfigureLegacyActors();
	Super::StartPlay();
	for (TActorIterator<APawn> It(GetWorld());It;++It) if (It->ActorHasTag(TEXT("CC_LegacyPlayer"))) {It->SetActorHiddenInGame(true);It->SetActorEnableCollision(false);It->SetActorTickEnabled(false);}
	for (FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
		if (auto* PC=Cast<ACC_PlayerController>(It->Get()))
		{
			if (!Cast<ACC_PlayerCharacter>(PC->GetPawn())) {PC->UnPossess();RestartPlayer(PC);}
			if (PC->GetPawn()) PC->SetViewTarget(PC->GetPawn());
		}
	if (CCHeavenScene::IsHeaven(GetWorld()) && !CCHeavenScene::Enter(GetWorld()))
		UE_LOG(LogTemp, Error, TEXT("Could not generate the Heaven entrance maze."));
	if (auto* Adventure=GetWorld()->GetGameInstance<UCC_AdventureGameInstance>())
 {
  Adventure->HandleRegionEntered(GetWorld());
  const FName Region=Adventure->RegionForMap(UGameplayStatics::GetCurrentLevelName(this,true));
  if(Region==TEXT("Town")||Region==TEXT("Arena"))
  {
   bool Existing=false;for(TActorIterator<ACC_CoinSpawnDirector> It(GetWorld());It;++It){Existing=true;break;}
   if(!Existing)if(auto* Director=GetWorld()->SpawnActor<ACC_CoinSpawnDirector>())Director->PopulationCap=Region==TEXT("Town")?4:6;
  }
 }
}
void ACC_AdventureGameMode::ConfigureLegacyActors()
{
	auto* Adventure=GetWorld()->GetGameInstance<UCC_AdventureGameInstance>();if (!Adventure) return;
	const FName Region=Adventure->RegionForMap(UGameplayStatics::GetCurrentLevelName(this,true));
	TArray<AActor*> OriginalPortals;
	// Use the exact migrated class. Keep its meshes/effects but remove its old overlap route.
	UClass* LegacyPortal=nullptr;
	if (Region==TEXT("Rest") || Region==TEXT("Town")) LegacyPortal=LoadClass<AActor>(nullptr,TEXT("/Game/LanTuRebuild/Blueprints/运河传送门/BP_传送门判断.BP_传送门判断_C"));
	if (LegacyPortal) UGameplayStatics::GetAllActorsOfClass(this,LegacyPortal,OriginalPortals);
	for (AActor* Source:OriginalPortals)
	{
		FTransform Transform=Source->GetActorTransform();FVector Extent(90,90,130);
		TInlineComponentArray<UBoxComponent*> Boxes(Source);
		for (auto* Box:Boxes) if (Box->GetGenerateOverlapEvents()) {Transform=Box->GetComponentTransform();Extent=Box->GetScaledBoxExtent();break;}
		TInlineComponentArray<UPrimitiveComponent*> Components(Source);
		for (auto* Component:Components) {Component->SetGenerateOverlapEvents(false);Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
		// Town returns through the map UI only. Disable any old portal without editing the art map.
		if (Region==TEXT("Town")) {Source->SetActorHiddenInGame(true);Source->SetActorTickEnabled(false);continue;}
		Transform.SetScale3D(FVector::OneVector);
		auto* Portal=GetWorld()->SpawnActorDeferred<ACC_MapPortal>(ACC_MapPortal::StaticClass(),Transform,Source,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (Portal)
        {
            Portal->DestinationRegion=TEXT("Town");Portal->bShowMarker=false;
            Portal->Trigger->SetBoxExtent(Extent.ComponentMax(FVector(40,40,90)));
            // Preserve the migrated blueprint's 8-second Delay and AddNiagaraComponent template.
            Portal->TravelDelay=8.f;
            FTransform EffectRelative=FTransform::Identity;
            if (const auto* Generated=Cast<UBlueprintGeneratedClass>(Source->GetClass()))
                for (UActorComponent* Template:Generated->ComponentTemplates)
                    if (const auto* Niagara=Cast<UNiagaraComponent>(Template))
                        if (Niagara->GetAsset())
                        {
                            Portal->DepartureEffect=Niagara->GetAsset();
                            EffectRelative=Niagara->GetRelativeTransform();
                            break;
                        }
            if (!Portal->DepartureEffect)
                Portal->DepartureEffect=LoadObject<UNiagaraSystem>(nullptr,TEXT("/Game/LanTuRebuild/Effects/运河传送门/粒子传送门.粒子传送门"));
            Portal->DepartureEffectTransform=(EffectRelative*Source->GetActorTransform()).GetRelativeTransform(Transform);
            Portal->FinishSpawning(Transform);
        }
	}
	// Stop an old placed template pawn from stealing Player0 or duplicating the hero.
	for (TActorIterator<APawn> It(GetWorld());It;++It)
		if (!It->IsA<ACC_PlayerCharacter>() && It->AutoPossessPlayer!=EAutoReceiveInput::Disabled)
		{It->AutoPossessPlayer=EAutoReceiveInput::Disabled;It->SetActorHiddenInGame(true);It->SetActorEnableCollision(false);It->SetActorTickEnabled(false);}
}
