#include "World/CC_LoreStand.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

ACC_LoreStand::ACC_LoreStand()
{
    ReadingZone = CreateDefaultSubobject<UBoxComponent>(TEXT("ReadingZone"));
    SetRootComponent(ReadingZone);
    ReadingZone->InitBoxExtent(FVector(195, 195, 125));
    ReadingZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ReadingZone->SetCanEverAffectNavigation(false);
}

bool ACC_LoreStand::CanRead(const ACC_PlayerCharacter* Player) const
{
    if (!Book || Book->Pages.IsEmpty() || IsHidden() || !IsValid(Player) ||
        !Player->IsAlive() || Player->GetWorld() != GetWorld())
        return false;

    const FVector Local = ReadingZone->GetComponentTransform().InverseTransformPosition(Player->GetActorLocation());
    const FVector Extent = ReadingZone->GetUnscaledBoxExtent();
    if (FMath::Abs(Local.X) > Extent.X || FMath::Abs(Local.Y) > Extent.Y || FMath::Abs(Local.Z) > Extent.Z)
        return false;

    FCollisionQueryParams Query(SCENE_QUERY_STAT(LoreStandReach), false, Player);
    Query.AddIgnoredActor(this);
    const FVector EyeOffset(0, 0, 35);
    return !GetWorld()->LineTraceTestByChannel(Player->GetActorLocation() + EyeOffset,
        GetActorLocation() + EyeOffset, ECC_Visibility, Query);
}
