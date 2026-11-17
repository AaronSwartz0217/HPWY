#include "Progression/CC_SummonedSoldier.h"
#include "Progression/CC_GameplayData.h"
#include "Progression/CC_ConfiguredAbility.h"
#include "Progression/CC_SkillComponent.h"
#include "Characters/CC_LocomotionComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ACC_SummonedSoldier::ACC_SummonedSoldier()
{
 PrimaryActorTick.bCanEverTick=true;
 // Decisions run at 5 Hz; direct steering needs input every movement frame.
 PrimaryActorTick.TickInterval=0.f;
 AIControllerClass=AAIController::StaticClass();AutoPossessAI=EAutoPossessAI::PlacedInWorldOrSpawned;
 Tags.AddUnique(CrashTags::Player);ExperienceReward=0;CorpseLifetime=3.f;
 SetDefaultAttributeEffects(UCC_InitialAttributesEffect::StaticClass());
 GetCapsuleComponent()->InitCapsuleSize(42.f,96.f);
 GetMesh()->SetRelativeLocation(FVector(0,0,-96));
 GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
 GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 bUseControllerRotationPitch=bUseControllerRotationYaw=bUseControllerRotationRoll=false;
 GetCharacterMovement()->bOrientRotationToMovement=true;
 GetCharacterMovement()->RotationRate=FRotator(0,540,0);
 GetCharacterMovement()->bRunPhysicsWithNoController=true;
 static ConstructorHelpers::FObjectFinder<UCC_FormDefinition> Form(TEXT("/Game/HPWH/Forms/DA_Form_Artifact_01.DA_Form_Artifact_01"));
 static ConstructorHelpers::FObjectFinder<UCC_LocomotionLibrary> Library(TEXT("/Game/HPWH/Locomotion/DA_NPCLocomotion.DA_NPCLocomotion"));
 static ConstructorHelpers::FObjectFinder<UCC_SkillDefinition> Basic(TEXT("/Game/HPWH/Skills/DA_BasicAttack.DA_BasicAttack"));
 SoldierForm=Form.Object;Locomotion->Library=Library.Object;MeleePresentation=Basic.Object;
}
void ACC_SummonedSoldier::BeginPlay()
{
 Super::BeginPlay();ApplyAppearance();
}
void ACC_SummonedSoldier::ApplyAppearance()
{
 if (SoldierForm && !SoldierForm->Stages.IsEmpty())
 {
  const auto& Stage=SoldierForm->Stages[0];
  GetMesh()->SetSkeletalMesh(Stage.Mesh);GetMesh()->SetRelativeScale3D(Stage.MeshScale);
  if (Stage.AnimClass) GetMesh()->SetAnimInstanceClass(Stage.AnimClass);
 }
 GetMesh()->SetRelativeLocation(FVector(0,0,-GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()+MeshGroundOffset));
 GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
}
void ACC_SummonedSoldier::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
 Super::GetLifetimeReplicatedProps(OutLifetimeProps);
 DOREPLIFETIME(ThisClass,Summoner);DOREPLIFETIME(ThisClass,CurrentTarget);
}
void ACC_SummonedSoldier::OnRep_Summoner()
{
 if (!IsValid(Summoner)) return;
 if (Summoner->ActorHasTag(CrashTags::Player)) Tags.AddUnique(CrashTags::Player);
 else Tags.Remove(CrashTags::Player);
}
void ACC_SummonedSoldier::PossessedBy(AController* NewController)
{
 Super::PossessedBy(NewController);
 // Possession normally assigns the AI controller as Owner. Keep kill credit
 // on the leader even when a replacement controller possesses this pawn later.
 if (IsValid(Summoner)) SetOwner(Summoner);
}
void ACC_SummonedSoldier::InitializeSummon(ACC_BaseCharacter* Source,UCC_SkillDefinition* Skill)
{
 if (!HasAuthority() || !IsValid(Source) || !Skill || !Source->GetAbilitySystemComponent()) return;
 Summoner=Source;OnRep_Summoner();
 if (!GetController()) SpawnDefaultController();
 SetOwner(Source);ExperienceReward=0;SetLifeSpan(FMath::Max(.1f,Skill->Duration));ApplyAppearance();
 auto* ASC=GetAbilitySystemComponent();auto* SourceASC=Source->GetAbilitySystemComponent();
 if(!ASC)return;
 if(ASC->GetAvatarActor()!=this)ASC->InitAbilityActorInfo(this,this);
 if(!ASC->GetSet<UCC_AttributeSet>())ASC->AddAttributeSetSubobject(Cast<UCC_AttributeSet>(GetAttributeSet()));
 const float Health=FMath::Max(1.f,SourceASC->GetNumericAttribute(UCC_AttributeSet::GetMaxHealthAttribute())*HealthFraction);
 ASC->SetNumericAttributeBase(UCC_AttributeSet::GetMaxHealthAttribute(),Health);
 ASC->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(),Health);
 ASC->SetNumericAttributeBase(UCC_AttributeSet::GetMoveSpeedAttribute(),500.f);
 ForceNetUpdate();
}
bool ACC_SummonedSoldier::FindGroundedSpawn(ACC_BaseCharacter* Source,FVector PreferredLocation,FTransform& OutTransform,TSubclassOf<ACC_SummonedSoldier> SoldierClass)
{
 if (!IsValid(Source) || !Source->GetWorld()) return false;
 auto* World=Source->GetWorld();const auto* Defaults=SoldierClass ? SoldierClass->GetDefaultObject<ACC_SummonedSoldier>() : GetDefault<ACC_SummonedSoldier>();
 const auto* Capsule=Defaults->GetCapsuleComponent();
 const float Half=Capsule->GetScaledCapsuleHalfHeight(),Radius=Capsule->GetScaledCapsuleRadius();
 auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
 FNavLocation Projected;
 if (Nav && Nav->ProjectPointToNavigation(PreferredLocation,Projected,FVector(100,100,250))) PreferredLocation=Projected.Location;
 FCollisionQueryParams GroundQuery(SCENE_QUERY_STAT(SummonGround),false,Source);
 FCollisionObjectQueryParams GroundObjects;GroundObjects.AddObjectTypesToQuery(ECC_WorldStatic);GroundObjects.AddObjectTypesToQuery(ECC_WorldDynamic);
 FHitResult Floor;
 if (!World->LineTraceSingleByObjectType(Floor,PreferredLocation+FVector(0,0,300),PreferredLocation-FVector(0,0,800),GroundObjects,GroundQuery)
  || !Defaults->GetCharacterMovement()->IsWalkable(Floor)) return false;
 const FVector Position=Floor.ImpactPoint+FVector(0,0,Half+2.f);
 // Unlike the ground trace, clearance also includes the leader and other pawns.
 if (World->OverlapBlockingTestByChannel(Position,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Radius,Half))) return false;
 FHitResult Obstruction;
 if (World->LineTraceSingleByChannel(Obstruction,Source->GetActorLocation(),Position,ECC_Visibility,GroundQuery)) return false;
 OutTransform=FTransform(FRotator(0,Source->GetActorRotation().Yaw,0),Position);return true;
}
bool ACC_SummonedSoldier::HasClearSight(const ACC_BaseCharacter* Target) const
{
 if (!IsValid(Target)) return false;
 FCollisionQueryParams Query(SCENE_QUERY_STAT(CompanionSight),false,this);
 if (Summoner && Summoner!=Target) Query.AddIgnoredActor(Summoner);
 FHitResult Hit;
 return !GetWorld()->LineTraceSingleByChannel(Hit,GetPawnViewLocation(),Target->GetPawnViewLocation(),ECC_Visibility,Query) || Hit.GetActor()==Target;
}
ACC_BaseCharacter* ACC_SummonedSoldier::FindEnemy() const
{
 if(IsValid(Summoner))if(auto* Focus=Summoner->Skills->GetCombatFocus();Focus&&!Focus->Skills->IsStealthed()&&!UCC_ConfiguredAbility::AreAllies(this,Focus)&&FVector::DistSquared(GetActorLocation(),Focus->GetActorLocation())<=FMath::Square(DetectionRadius)&&HasClearSight(Focus))return Focus;
 ACC_BaseCharacter* Target=nullptr;float Best=FMath::Square(DetectionRadius);
 for (TActorIterator<ACC_BaseCharacter> It(GetWorld());It;++It)
 {
  auto* Candidate=*It;
  if (Candidate==this || !Candidate->IsAlive() || UCC_ConfiguredAbility::AreAllies(this,Candidate) || Candidate->Skills->IsStealthed()) continue;
  const float Distance=FVector::DistSquared(GetActorLocation(),Candidate->GetActorLocation());
  if (Distance<Best && HasClearSight(Candidate)) {Best=Distance;Target=Candidate;}
 }
 return Target;
}
bool ACC_SummonedSoldier::CanHit(const ACC_BaseCharacter* Target) const
{
 return IsValid(Target) && Target->IsAlive() && !Target->Skills->IsStealthed()
  && !UCC_ConfiguredAbility::AreAllies(this,Target)
  && FVector::DistSquared(GetActorLocation(),Target->GetActorLocation())<=FMath::Square(AttackRange)
  && HasClearSight(Target);
}
FAIMoveRequest ACC_SummonedSoldier::MakeMoveRequest(ACC_BaseCharacter* Goal,float Acceptance) const
{
 FAIMoveRequest Request(Goal);Request.SetAcceptanceRadius(FMath::Max(1.f,Acceptance));
 // Both radii otherwise inflate "arrived" beyond the actual melee range.
 Request.SetReachTestIncludesAgentRadius(false);Request.SetReachTestIncludesGoalRadius(false);
 Request.SetAllowPartialPath(false);Request.SetUsePathfinding(true);return Request;
}
void ACC_SummonedSoldier::StopMoving()
{
 DirectMoveGoal.Reset();LastMoveGoal.Reset();
 if (auto* AI=GetController<AAIController>();AI && AI->GetMoveStatus()!=EPathFollowingStatus::Idle) AI->StopMovement();
 // Do not cancel vertical gravity/knockback while stopping horizontal pursuit.
 GetCharacterMovement()->Velocity.X=0.f;GetCharacterMovement()->Velocity.Y=0.f;
}
void ACC_SummonedSoldier::MoveToward(ACC_BaseCharacter* Goal,float Acceptance)
{
 if (Skills->IsRooted()) {StopMoving();return;}
 const float Now=GetWorld()->GetTimeSeconds();auto* AI=GetController<AAIController>();
 if (AI && LastMoveGoal.Get()==Goal && AI->GetMoveStatus()==EPathFollowingStatus::Moving) return;
 if (Now<NextMoveRequest && LastMoveGoal.Get()==Goal) return;
 NextMoveRequest=Now+.5f;LastMoveGoal=Goal;
 if (AI && AI->MoveTo(MakeMoveRequest(Goal,Acceptance)).Code!=EPathFollowingRequestResult::Failed)
 {DirectMoveGoal.Reset();return;}
 // While navigation is missing or rebuilding, steer using normal capsule
 // collision and gravity. This fallback never teleports or moves through walls.
 if (HasClearSight(Goal)) {DirectMoveGoal=Goal;DirectAcceptance=Acceptance;}
 else DirectMoveGoal.Reset();
}
void ACC_SummonedSoldier::UpdateBrain()
{
 if (!IsValid(Summoner) || !Summoner->IsAlive()) {Destroy();return;}
 if (!IsAlive()) return;
 auto* ASC=GetAbilitySystemComponent();auto* SourceASC=Summoner->GetAbilitySystemComponent();
 if (!ASC || !SourceASC) return;
 ASC->SetNumericAttributeBase(UCC_AttributeSet::GetDefenseAttribute(),SourceASC->GetNumericAttribute(UCC_AttributeSet::GetDefenseAttribute()));
 if (Skills->IsStunned() || bIsBeingLaunched || GetCharacterMovement()->IsFalling()) {StopMoving();return;}
 CurrentTarget=FindEnemy();
 if (CurrentTarget)
 {
  if (CanHit(CurrentTarget))
  {
   StopMoving();FRotator Facing=(CurrentTarget->GetActorLocation()-GetActorLocation()).Rotation();Facing.Pitch=Facing.Roll=0;
   SetActorRotation(Facing);
   if (GetWorld()->GetTimeSeconds()>=NextAttack && !GetWorldTimerManager().IsTimerActive(HitTimer))
   {
    NextAttack=GetWorld()->GetTimeSeconds()+FMath::Max(.1f,AttackInterval);PendingTarget=CurrentTarget;
    Locomotion->PresentSkill(MeleePresentation,CurrentTarget->GetActorLocation());
    GetWorldTimerManager().SetTimer(HitTimer,this,&ThisClass::PerformMeleeHit,FMath::Max(.01f,AttackWindup),false);
   }
  }
  else MoveToward(CurrentTarget,FMath::Max(20.f,AttackRange-50.f));
 }
 else if (FVector::DistSquared2D(GetActorLocation(),Summoner->GetActorLocation())>FMath::Square(FollowDistance))
  MoveToward(Summoner,FMath::Max(30.f,FollowDistance-50.f));
 else StopMoving();
}
void ACC_SummonedSoldier::PerformMeleeHit()
{
 auto* Target=PendingTarget.Get();PendingTarget.Reset();
 if (!HasAuthority() || !IsAlive() || Skills->IsStunned() || GetCharacterMovement()->IsFalling()
  || !IsValid(Summoner) || !Summoner->IsAlive() || !CanHit(Target)) return;
 auto* SourceASC=Summoner->GetAbilitySystemComponent();if (!SourceASC) return;
 UCC_ConfiguredAbility::ApplyEffect(this,Target,UCC_DamageEffect::StaticClass(),
  SourceASC->GetNumericAttribute(UCC_AttributeSet::GetAttackAttribute())*AttackFraction,0.f,false);
 Locomotion->PresentImpact(MeleePresentation,Target->GetActorLocation());
}
void ACC_SummonedSoldier::Tick(float Delta)
{
 Super::Tick(Delta);if (!HasAuthority() || !IsAlive()) return;
 BrainElapsed+=Delta;
 if (BrainElapsed>=.2f) {BrainElapsed=0.f;UpdateBrain();}
 if (IsActorBeingDestroyed() || !DirectMoveGoal.IsValid() || Skills->IsRooted() || GetCharacterMovement()->IsFalling()) return;
 const FVector ToGoal=DirectMoveGoal->GetActorLocation()-GetActorLocation();
 if (ToGoal.SizeSquared2D()>FMath::Square(DirectAcceptance) && HasClearSight(DirectMoveGoal.Get())) AddMovementInput(ToGoal.GetSafeNormal2D());
}
void ACC_SummonedSoldier::HandleDeath()
{
 GetWorldTimerManager().ClearTimer(HitTimer);PendingTarget.Reset();CurrentTarget=nullptr;StopMoving();Super::HandleDeath();
}
void ACC_SummonedSoldier::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
 GetWorldTimerManager().ClearTimer(HitTimer);Super::EndPlay(EndPlayReason);
}
