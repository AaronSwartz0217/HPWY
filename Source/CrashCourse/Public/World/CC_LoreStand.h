#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "CC_LoreStand.generated.h"

class ACC_PlayerCharacter;
class UBoxComponent;

USTRUCT(BlueprintType)
struct FCC_LorePage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Section;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Title;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine=true)) FText Body;
};

UCLASS(BlueprintType)
class CRASHCOURSE_API UCC_LoreBook : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Title;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine=true)) TArray<FText> OpeningThoughts;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FCC_LorePage> Pages;
};

UCLASS(Blueprintable)
class CRASHCOURSE_API ACC_LoreStand : public AActor
{
    GENERATED_BODY()

public:
    ACC_LoreStand();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Book") TObjectPtr<UCC_LoreBook> Book;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Book") TObjectPtr<UBoxComponent> ReadingZone;

    bool CanRead(const ACC_PlayerCharacter* Player) const;
};
