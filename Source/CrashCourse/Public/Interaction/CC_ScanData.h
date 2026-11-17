#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Components/ActorComponent.h"
#include "CC_ScanData.generated.h"
UCLASS(BlueprintType)
class CRASHCOURSE_API UCC_ScanDescription:public UDataAsset
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Scan") FText DisplayName;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Scan",meta=(MultiLine="true")) FText Description;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Scan") FText ActionText;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Scan",meta=(ClampMin="1")) float MaxDistance=600.f;
};
// Add to a scene object or its Blueprint and select an information asset.
UCLASS(ClassGroup=(Crash),meta=(BlueprintSpawnableComponent,DisplayName="扫描目标 / Scan Target"))
class CRASHCOURSE_API UCC_ScanTargetComponent:public UActorComponent
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Scan") TObjectPtr<UCC_ScanDescription> Info;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Scan") bool bEnabled=true;
};
USTRUCT(BlueprintType)
struct FCC_ScanRule
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TSubclassOf<AActor> ActorClass;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TSubclassOf<UActorComponent> ComponentClass;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TObjectPtr<UCC_ScanDescription> Info;
};
UCLASS(BlueprintType)
class CRASHCOURSE_API UCC_ScanCatalog:public UDataAsset
{
 GENERATED_BODY()
public:
 // First matching rule wins. A ScanTarget component overrides these defaults.
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FCC_ScanRule> Rules;
};
