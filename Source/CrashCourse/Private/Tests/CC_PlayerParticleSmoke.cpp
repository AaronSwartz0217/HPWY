#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_SkillComponent.h"
#include "Progression/CC_SkillProjectile.h"
#include "AbilitySystemComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Components/StaticMeshComponent.h"
namespace {
struct FParticleProbe:public TSharedFromThis<FParticleProbe> {
 UWorld* W=nullptr;TWeakObjectPtr<ACC_PlayerController> PC;TWeakObjectPtr<ACC_PlayerCharacter> Player;UCC_SkillDefinition* Skill=nullptr;
 int32 Phase=0;float Start=0;int32 Samples=0;float MaxDistance=0;bool SawFX=false,Pass=true;FVector Origin;FTimerHandle Timer;
 TArray<TSharedPtr<FJsonValue>> Results;
 void BeginPhase(){
  if(Phase>=5){auto Root=MakeShared<FJsonObject>();Root->SetBoolField(TEXT("passed"),Pass);Root->SetArrayField(TEXT("phases"),Results);FString Json;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Json));FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("PlayerParticleSmoke.json")));UE_LOG(LogTemp,Display,TEXT("PLAYER_PARTICLE_REPORT %s"),*Json);W->GetTimerManager().ClearTimer(Timer);GEngine->Exec(W,TEXT("QUIT"));return;}
  for(TActorIterator<ACC_SkillProjectile> It(W);It;++It)if(It->GetOwner()==Player.Get())It->Destroy();
  Player->Skills->CancelActiveSkills();Player->Skills->ClearDesignStates();Player->GetAbilitySystemComponent()->RemoveActiveEffects(FGameplayEffectQuery());
  if(Phase==4)Player->SetFirstPerson(true);
  if(Phase>0)PC->SetControlRotation(FRotator(Phase==1||Phase==4?0:Phase==2?-25:-55,0,0));
  Origin=Player->GetActorLocation();Start=W->GetTimeSeconds();Samples=0;MaxDistance=0;SawFX=false;
  const bool Accepted=Player->Skills->ActivateSlot(0);Player->Skills->ReleaseSlot(0);
  UE_LOG(LogTemp,Display,TEXT("PLAYER_PARTICLE_START phase=%d accepted=%d pos=%s aim=%s view=%s speed=%.1f range=%.1f fx=%s scale=%.3f"),Phase,Accepted,*Origin.ToString(),*PC->GetControlRotation().ToString(),*Player->GetPawnViewLocation().ToString(),Skill->ProjectileSpeed,Skill->Range,*GetNameSafe(Skill->ProjectileNiagara),Skill->EffectScale);
 }
 void Tick(){
  for(TActorIterator<ACC_SkillProjectile> It(W);It;++It)if(It->GetOwner()==Player.Get()){
   ++Samples;MaxDistance=FMath::Max(MaxDistance,FVector::Dist(It->GetActorLocation(),Origin));
   TArray<UStaticMeshComponent*> Cores;It->GetComponents(Cores);for(auto* C:Cores)SawFX|=C->IsVisible()&&C->GetMaterial(0)==Skill->ProjectileCoreMaterial&&Skill->ProjectileCoreMaterial!=nullptr;
   TArray<UNiagaraComponent*> Components;It->GetComponents(Components);for(auto* C:Components)SawFX|=C->IsActive();
   if(Samples==8)FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/PlayerParticle/Phase%d.png"),Phase),true,false);
   if(Samples==1)UE_LOG(LogTemp,Display,TEXT("PLAYER_PARTICLE_ALIVE phase=%d pos=%s velocity=%s"),Phase,*It->GetActorLocation().ToString(),*It->Movement->Velocity.ToString());
  }
  if(W->GetTimeSeconds()-Start<.65f)return;
  Pass&=Samples>0&&SawFX&&MaxDistance>60.f;
  auto Result=MakeShared<FJsonObject>();Result->SetNumberField(TEXT("phase"),Phase);Result->SetNumberField(TEXT("samples"),Samples);Result->SetNumberField(TEXT("maxDistance"),MaxDistance);Result->SetBoolField(TEXT("visibleEffect"),SawFX);Results.Add(MakeShared<FJsonValueObject>(Result));++Phase;BeginPhase();
 }
};
void RunParticleProbe(UWorld* W){
 if(!W||!W->IsGameWorld()||W->GetNetMode()!=NM_Standalone)return;
 FTimerHandle Wait;W->GetTimerManager().SetTimer(Wait,[W](){
  auto Q=MakeShared<FParticleProbe>();Q->W=W;Q->PC=Cast<ACC_PlayerController>(W->GetFirstPlayerController());Q->Player=Q->PC.IsValid()?Cast<ACC_PlayerCharacter>(Q->PC->GetPawn()):nullptr;
  if(!Q->Player.IsValid()){GEngine->Exec(W,TEXT("QUIT"));return;}
  auto* P=Q->PC->GetPlayerState<ACC_PlayerState>()->Progression.Get();P->ApplyDeveloperPreset(false);
  Q->Skill=LoadObject<UCC_SkillDefinition>(nullptr,TEXT("/Game/HPWH/Skills/DA_Skill_01.DA_Skill_01"));P->EquipSkill(0,Q->Skill);
  if(Q->Skill&&Q->Skill->ProjectileNiagara){TArray<FNiagaraVariable> Vars;Q->Skill->ProjectileNiagara->GetExposedParameters().GetUserParameters(Vars);for(const auto& V:Vars)UE_LOG(LogTemp,Display,TEXT("PLAYER_PARTICLE_FX_PARAM %s %s"),*V.GetName().ToString(),*V.GetType().GetName());}
  Q->BeginPhase();W->GetTimerManager().SetTimer(Q->Timer,[Q](){Q->Tick();},.01f,true);
 },3.f,false);
}
FAutoConsoleCommandWithWorld PlayerParticleProbe(TEXT("CC.PlayerParticleSmokeTest"),TEXT("Probe the actual player particle skill at normal and downward aim; writes report then exits isolated process."),FConsoleCommandWithWorldDelegate::CreateStatic(&RunParticleProbe));
}
#endif
