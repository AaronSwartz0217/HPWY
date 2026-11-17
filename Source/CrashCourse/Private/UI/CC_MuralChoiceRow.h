#pragma once

#include "Blueprint/UserWidget.h"
#include "CC_MuralChoiceRow.generated.h"

class UMaterialInterface;
class UTexture2D;

/** Slate retains the UUserWidget while a combo box displays the row. */
UCLASS()
class UCC_MuralChoiceRow : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetContent(const FString& Label, bool bShowIcon = false,
        UTexture2D* Texture = nullptr, UMaterialInterface* Material = nullptr);
};
