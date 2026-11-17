#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_PatrolEnemyCharacter.h"
#include "Player/CC_PlayerState.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_CoinPickup.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"
#include "UnrealClient.h"
#include "SkeletalRenderPublic.h"
#include "NavigationSystem.h"
namespace {
struct FCoinPatrolProbe {
 TWeakObjectPtr<ACC_CoinPickup> Coin;
 TArray<TWeakObjectPtr<ACC_CoinPickup>> MapCoins;
 TArray<FVector> MapStarts;
 TArray<bool> MapMoved;
 FVector Start;
 bool Moved=false,Idle=false,Run=false,Grounded=true,FeetNearGround=true,Pass=true;
 float Distance=0,MinMeshBottom=MAX_flt,MaxMeshBottom=-MAX_flt;
 int32 Samples=0;
 FTimerHandle SampleTimer;
 TSharedPtr<FJsonObject> Checks=MakeShared<FJsonObject>();
 void Check(bool B,const TCHAR* N){Pass&=B;Checks->SetBoolField(N,B);UE_LOG(LogTemp,Display,TEXT("COIN_PATROL_%s: %d"),N,B);}
};
void RunCoinPatrolSmoke(UWorld* W) {
 if(!W || !W->IsGameWorld() || W->GetNetMode()!=NM_Standalone) return;
 auto Q=MakeShared<FCoinPatrolProbe>();
 auto Later=[W](float Delay,TFunction<void()> Fn){FTimerHandle T;W->GetTimerManager().SetTimer(T,MoveTemp(Fn),Delay,false);};
 Later(3.f,[W,Q,Later](){
  auto* Player=Cast<ACC_PlayerCharacter>(W->GetFirstPlayerController()->GetPawn());
  if(!Player) {GEngine->Exec(W,TEXT("QUIT"));return;}
  Player->SetActorLocation(FVector(-2400,-2400,120));
  for(TActorIterator<ACC_PatrolEnemyCharacter> It(W);It;++It) It->StopPatrol();
  int32 MapCoins=0; bool Config=true;
  for(TActorIterator<ACC_CoinPickup> It(W);It;++It) {
   ++MapCoins;Config&=It->GetRootComponent()==It->GetCapsuleComponent() && It->CoinMesh->GetAnimInstance()!=nullptr && It->GetController<AAIController>()!=nullptr;
   Q->MapCoins.Add(*It);Q->MapStarts.Add(It->GetActorLocation());Q->MapMoved.Add(false);
  }
  Q->Check(MapCoins==5 && Config,TEXT("five_map_coins_use_capsule_AI_and_animation_blueprint"));
  UClass* Class=LoadClass<ACC_CoinPickup>(nullptr,TEXT("/Game/HPWH/Coins/BP_CopperCoin.BP_CopperCoin_C"));
  FActorSpawnParameters P;P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  Q->Coin=W->SpawnActor<ACC_CoinPickup>(Class,FVector(600,-1000,220),FRotator::ZeroRotator,P);
  if(!Q->Coin.IsValid()){Q->Check(false,TEXT("spawn"));GEngine->Exec(W,TEXT("QUIT"));return;}
  Q->Start=Q->Coin->GetActorLocation();
  Q->Check(!Q->Coin->TryCollect(Player),TEXT("remote_collection_rejected"));
  W->GetTimerManager().SetTimer(Q->SampleTimer,[W,Q](){
   auto* C=Q->Coin.Get();if(!C)return;
   for(int32 I=0;I<Q->MapCoins.Num();++I) if(auto* MapCoin=Q->MapCoins[I].Get()) Q->MapMoved[I]=Q->MapMoved[I] || FVector::Dist2D(Q->MapStarts[I],MapCoin->GetActorLocation())>40;
   ++Q->Samples;Q->Distance=FMath::Max(Q->Distance,static_cast<float>(FVector::Dist2D(C->GetActorLocation(),Q->Start)));
   auto* Anim=C->CoinMesh->GetAnimInstance();const float Speed=C->GetVelocity().Size2D();
   double AnimSpeed=-1;
   if(Anim) if(auto* Prop=FindFProperty<FNumericProperty>(Anim->GetClass(),TEXT("speed"))) AnimSpeed=Prop->GetFloatingPointPropertyValue(Prop->ContainerPtrToValuePtr<void>(Anim));
   Q->Moved|=Speed>50.f;
   Q->Run|=Anim && !Anim->GetCurrentStateName(0).IsNone() && Speed>50 && AnimSpeed>50;
   Q->Idle|=Speed<1 && AnimSpeed>=0 && AnimSpeed<1;
   // Allow the initially elevated spawn to fall before checking grounded motion.
   if(Q->Samples>10) {
    Q->Grounded&=C->GetCharacterMovement()->IsMovingOnGround();
    // Animated physics bounds include padding and aren't the rendered soles.
    // Sample deformed geometry rather than using those bounds as foot height.
    if(Q->Samples%10!=0) return;
    TArray<FFinalSkinVertex> Vertices;C->CoinMesh->GetCPUSkinnedVertices(Vertices,0);
    float Bottom=MAX_flt;
    for(const auto& Vertex:Vertices) Bottom=FMath::Min(Bottom,static_cast<float>(C->CoinMesh->GetComponentTransform().TransformPosition(FVector(Vertex.Position)).Z));
    const float Floor=C->GetActorLocation().Z-C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-C->GetCharacterMovement()->CurrentFloor.FloorDist;
    const float Offset=Bottom-Floor;
    Q->MinMeshBottom=FMath::Min(Q->MinMeshBottom,Offset);Q->MaxMeshBottom=FMath::Max(Q->MaxMeshBottom,Offset);
    Q->FeetNearGround&=Vertices.Num()>0 && FMath::Abs(Offset)<6.f;
   }
  },.1f,true);
  Later(15.f,[W,Q,Player,Later](){
   W->GetTimerManager().ClearTimer(Q->SampleTimer);
   auto* C=Q->Coin.Get();
   bool AllMoved=Q->MapCoins.Num()==5;
   for(int32 I=0;I<Q->MapCoins.Num();++I) {
    auto* MapCoin=Q->MapCoins[I].Get();
    AllMoved&=MapCoin && Q->MapMoved[I];
    if(MapCoin)MapCoin->StopPatrol();
   }
   Q->Check(AllMoved,TEXT("all_five_placed_coins_really_patrol"));
   auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(W);FNavLocation N;
   Q->Check(C && Nav && Nav->ProjectPointToNavigation(C->GetActorLocation(),N,FVector(100,100,1000)),TEXT("saved_arena_has_usable_navmesh"));
   Q->Check(Q->Moved && Q->Distance>80,TEXT("actual_navigation_patrol_moves"));
   Q->Check(Q->Idle && Q->Run,TEXT("original_state_machine_speed_drives_idle_and_walk"));
   Q->Check(Q->Grounded,TEXT("gravity_lands_and_patrol_stays_walking_on_floor"));
   Q->Check(Q->FeetNearGround,TEXT("animated_mesh_bottom_stays_near_floor"));
   if(C) {
    C->StopPatrol();Q->Check(C->GetVelocity().IsNearlyZero(),TEXT("stop_patrol_stops_motion"));
    Player->SetActorLocation(C->GetActorLocation()+FVector(-220,0,0));
    Player->GetController()->SetControlRotation(FRotator(0,0,0));Player->SetActorRotation(FRotator(0,0,0));
    Q->Check(C->CoinMesh->GetAnimInstance()->GetClass()->GetName().Contains(TEXT("ABP_WuzhuCoin")),TEXT("uses_original_imported_ABP"));
   }
   Later(.8f,[W](){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/CoinPatrol/GroundedCoin.png"),false,false);});
   Later(1.f,[W,Q,Player,Later](){
    auto* C=Q->Coin.Get();auto* Progress=Player->GetPlayerState<ACC_PlayerState>()->Progression.Get();
    const int32 Coins=Progress->CopperCoins,XP=Progress->Experience;
    if(C) {Player->SetActorLocation(C->GetActorLocation());C->TryCollect(Player);C->TryCollect(Player);}
    Q->Check(Progress->CopperCoins==Coins+1 && Progress->Experience==XP+5,TEXT("touch_awards_once_one_copper_five_XP"));
    Later(.2f,[W,Q](){
     Q->Check(!Q->Coin.IsValid(),TEXT("collected_coin_and_patrol_removed"));
     auto Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("pass"),Q->Pass);Result->SetObjectField(TEXT("checks"),Q->Checks);
     Result->SetNumberField(TEXT("maxDistanceCm"),Q->Distance);Result->SetNumberField(TEXT("minMeshFloorOffsetCm"),Q->MinMeshBottom);Result->SetNumberField(TEXT("maxMeshFloorOffsetCm"),Q->MaxMeshBottom);
     FString Json;auto Writer=TJsonWriterFactory<>::Create(&Json);FJsonSerializer::Serialize(Result,Writer);
     FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("CoinPatrolSmoke.json")));
     GEngine->Exec(W,TEXT("QUIT"));
    });
   });
  });
 });
}
FAutoConsoleCommandWithWorld CoinPatrolSmokeCommand(TEXT("CC.CoinPatrolSmokeTest"),TEXT("Verify original coin animation, grounded navigation and pickup reward; writes Saved/CoinPatrolSmoke.json then exits."),FConsoleCommandWithWorldDelegate::CreateStatic(&RunCoinPatrolSmoke));
}
#endif
