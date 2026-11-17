#include "World/CC_LevitationPlatform.h"
#include "World/CC_HeavenSceneRebuild.h"
#include "WFCMapGenerator.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ACC_LevitationPlatform::ACC_LevitationPlatform()
{
    bReplicates=true;SetReplicateMovement(true);PrimaryActorTick.bCanEverTick=true;SetNetUpdateFrequency(30.f);
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("PlatformAnchor")));
    LiftRoot=CreateDefaultSubobject<USceneComponent>(TEXT("LiftRoot"));LiftRoot->SetupAttachment(RootComponent);LiftRoot->SetMobility(EComponentMobility::Movable);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Deck=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PhysicalDeck"));Deck->SetupAttachment(LiftRoot);Deck->SetCanEverAffectNavigation(false);Deck->SetStaticMesh(Cube.Object);Deck->SetRelativeLocation(FVector(0,0,-15));Deck->SetRelativeScale3D(FVector(12,12,.3));Deck->SetMobility(EComponentMobility::Movable);Deck->SetCollisionProfileName(TEXT("BlockAll"));Deck->CanCharacterStepUpOn=ECB_Yes;
    Tiles=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ReassemblyTiles"));Tiles->SetupAttachment(LiftRoot);Tiles->SetCanEverAffectNavigation(false);Tiles->SetStaticMesh(Cube.Object);Tiles->SetMobility(EComponentMobility::Movable);Tiles->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Console=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ReassemblyConsole"));Console->SetupAttachment(LiftRoot);Console->SetStaticMesh(Cube.Object);Console->SetRelativeLocation(FVector(0,-500,55));Console->SetRelativeScale3D(FVector(.5,.5,1.1));Console->SetMobility(EComponentMobility::Movable);Console->SetCollisionProfileName(TEXT("BlockAll"));
    Field=CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionRegion"));Field->SetupAttachment(LiftRoot);Field->SetRelativeLocation(FVector(0,0,150));Field->SetBoxExtent(FVector(610,610,250));Field->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Field->SetCollisionObjectType(ECC_WorldDynamic);Field->SetCollisionResponseToAllChannels(ECR_Ignore);
    Edges=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("DeckInlay"));Edges->SetupAttachment(LiftRoot);Edges->SetCanEverAffectNavigation(false);Edges->SetStaticMesh(Cube.Object);Edges->SetMobility(EComponentMobility::Movable);Edges->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void ACC_LevitationPlatform::OnConstruction(const FTransform& T)
{
    Super::OnConstruction(T);LiftRoot->SetRelativeScale3D(FVector(FMath::Clamp(FootprintScale,.2f,2.f),FMath::Clamp(FootprintScale,.2f,2.f),1));LiftRoot->SetRelativeLocation(FVector(0,0,CurrentLift));BuildLayout(false);Edges->ClearInstances();
    for(int32 Side:{-1,1})
    {
        Edges->AddInstance(FTransform(FRotator::ZeroRotator,FVector(Side*595,0,1),FVector(.035,12,.04)));
        Edges->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,Side*595,1),FVector(12,.035,.04)));
        for(int32 Other:{-1,1})Edges->AddInstance(FTransform(FRotator::ZeroRotator,FVector(Side*580,Other*580,2),FVector(.25,.25,.06)));
    }
}
void ACC_LevitationPlatform::BeginPlay(){Super::BeginPlay();BuildLayout(false);OnRep_Lift();}
void ACC_LevitationPlatform::BuildLayout(bool Animate)
{
    Previous.Reset();for(int32 I=0;I<Tiles->GetInstanceCount();++I){FTransform T;Tiles->GetInstanceTransform(I,T,false);Previous.Add(T);}
    Targets.Reset();FRandomStream R(LayoutSeed);const int32 Kind=0;
    for(int32 I=0;I<9;++I)
    {
        FVector P;
        if(Kind==0)P=FVector((I%3-1)*300,(I/3-1)*300,-11.5);
        else if(I==0)P=FVector(0,0,-11.5);
        else if(Kind==1){const float A=(I-1)*PI/4;P=FVector(FMath::Cos(A)*420,FMath::Sin(A)*420,-11.5);}
        else{const int32 Arm=(I-1)/2;const float Distance=(I%2 ? 225.f : 450.f);P=FVector(Arm==0 ? Distance : Arm==1 ? -Distance : 0,Arm==2 ? Distance : Arm==3 ? -Distance : 0,-11.5);}
        Targets.Add(FTransform(FRotator(0,R.RandRange(0,3)*90,0),P,FVector(2.8,2.8,.25)));
    }
    bAnimating=Animate&&Previous.Num()==Targets.Num();
    if(bAnimating)RebuildStarted=GetWorld()->GetTimeSeconds();
    else{Tiles->ClearInstances();for(const auto& T:Targets)Tiles->AddInstance(T);}
    RebuildElapsed=0;
}
void ACC_LevitationPlatform::OnRep_Layout(){}
void ACC_LevitationPlatform::OnRep_Lift(){if(HasAuthority()||!HasActorBegunPlay())LiftRoot->SetRelativeLocation(FVector(0,0,CurrentLift));}
bool ACC_LevitationPlatform::IsCarrying(const ACC_PlayerCharacter* C) const
{
    return IsValid(C)&&C->IsAlive()&&(C->GetMovementBase()==Deck||C->GetMovementBase()==Console)&&C->GetCharacterMovement()->IsMovingOnGround();
}
bool ACC_LevitationPlatform::CanInteract(const ACC_PlayerCharacter* C) const
{
    if(!IsValid(C)||!C->IsAlive()||IsActorBeingDestroyed())return false;
    if(auto* Generator=CCHeavenScene::FindGenerator(GetWorld()))return Generator->CanRegenerateRandomScene();
    return false;
}
bool ACC_LevitationPlatform::Reassemble(ACC_PlayerCharacter* C)
{
    if(!HasAuthority()||!CanInteract(C))return false;
    return CCHeavenScene::Rebuild(GetWorld());
}
void ACC_LevitationPlatform::Tick(float Dt)
{
    Super::Tick(Dt);
    if(HasAuthority())
    {
        bool Occupied=false;
        for(TActorIterator<ACC_PlayerCharacter> It(GetWorld());It;++It)if(IsCarrying(*It)){Occupied=true;It->GetCharacterMovement()->AddTickPrerequisiteActor(this);}
        EmptyTime=Occupied ? 0.f : EmptyTime+Dt;
        const float Target=(Occupied||EmptyTime<FMath::Max(0.f,EmptyReturnDelay)) ? FMath::Max(0.f,LiftHeight) : 0.f;
        CurrentLift=FMath::FInterpConstantTo(CurrentLift,Target,Dt,FMath::Max(1.f,LiftSpeed));OnRep_Lift();
    }
    else
    {
        const float Z=FMath::FInterpConstantTo(LiftRoot->GetRelativeLocation().Z,CurrentLift,Dt,FMath::Max(1.f,LiftSpeed)*1.2f);
        LiftRoot->SetRelativeLocation(FVector(0,0,Z));
    }
    // Cosmetic stones animate; the physical deck remains continuous under passengers.
    if(bAnimating)
    {
        RebuildElapsed+=Dt;const float T=FMath::Clamp(RebuildElapsed/FMath::Max(.1f,RebuildDuration),0.f,1.f);const float Ease=T*T*(3-2*T);
        for(int32 I=0;I<Targets.Num();++I){FTransform Pose;Pose.Blend(Previous[I],Targets[I],Ease);Pose.AddToTranslation(FVector(0,0,FMath::Sin(PI*T)*(75+I*4)));Tiles->UpdateInstanceTransform(I,Pose,false,false,true);}
        Tiles->MarkRenderStateDirty();if(T>=1.f)bAnimating=false;
    }
}
void ACC_LevitationPlatform::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ThisClass,LayoutSeed);DOREPLIFETIME(ThisClass,CurrentLift);}
