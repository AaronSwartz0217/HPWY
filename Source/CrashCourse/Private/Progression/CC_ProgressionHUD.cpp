#include "Progression/CC_ProgressionHUD.h"
#include "UI/CC_PlayerHealthHUDWidget.h"
#include "UI/CC_FirstPersonScanWidget.h"
#include "Characters/CC_PlayerCharacter.h"
#include "World/CC_LevitationPlatform.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_SkillComponent.h"
#include "Player/CC_PlayerState.h"
#include "Player/CC_PlayerController.h"
#include "Characters/CC_BaseCharacter.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "Progression/CC_ArtifactComponent.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"

void ACC_ProgressionHUD::BeginPlay()
{
	Super::BeginPlay();
 if(PlayerOwner&&PlayerOwner->IsLocalController()){PlayerHealthHUD=CreateWidget<UCC_PlayerHealthHUDWidget>(PlayerOwner,UCC_PlayerHealthHUDWidget::StaticClass());if(PlayerHealthHUD)PlayerHealthHUD->AddToViewport(5);}
 if(PlayerOwner&&PlayerOwner->IsLocalController()){FirstPersonScanHUD=CreateWidget<UCC_FirstPersonScanWidget>(PlayerOwner,UCC_FirstPersonScanWidget::StaticClass());if(FirstPersonScanHUD)FirstPersonScanHUD->AddToViewport(6);}
 LoadedPickupPrompt=PickupPromptTexture.LoadSynchronous();
	// Explicit command-line smoke capture only; ordinary gameplay has no automatic exit.
	if (FParse::Param(FCommandLine::Get(), TEXT("ArenaSmokeCapture")))
	{
		FTimerHandle CaptureTimer;
		GetWorldTimerManager().SetTimer(CaptureTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/FlatArena.png"), true, false);
		}), 15.f, false);
		FTimerHandle ExitTimer;
		GetWorldTimerManager().SetTimer(ExitTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (GEngine) GEngine->Exec(GetWorld(), TEXT("QUIT"));
		}), 18.f, false);
	}
}

bool ACC_ProgressionHUD::IsPickupPromptVisible() const
{
 const auto* PC=Cast<ACC_PlayerController>(PlayerOwner);
 const auto* Character=PC?Cast<ACC_PlayerCharacter>(PC->GetPawn()):nullptr;
 return Character&&Character->IsFirstPerson()&&PC->FindInteractionTarget()!=nullptr;
}
void ACC_ProgressionHUD::DrawHUD()
{
	Super::DrawHUD();
 const auto* LocalCharacter=PlayerOwner?Cast<ACC_PlayerCharacter>(PlayerOwner->GetPawn()):nullptr;
 const bool bFirstPerson=LocalCharacter&&LocalCharacter->IsFirstPerson();
 const float PromptLift=PlayerHealthHUD&&PlayerHealthHUD->IsExpanded()?430.f:0.f;
 const auto* InteractionPC=Cast<ACC_PlayerController>(PlayerOwner);
 const bool LegacyPrompt=bFirstPerson&&!FirstPersonScanHUD;
 const auto* Platform=LegacyPrompt&&InteractionPC ? Cast<ACC_LevitationPlatform>(InteractionPC->FindInteractionTarget()) : nullptr;
 if(Canvas&&LegacyPrompt&&Platform)
 {
  const float S=FMath::Min(Canvas->SizeX/1920.f,Canvas->SizeY/1080.f);
  DrawRect(FLinearColor(0,0,0,.78f),32*S,Canvas->SizeY-(252.f+PromptLift)*S,720*S,82*S);
  DrawRect(FLinearColor(.82f,.72f,.51f,.5f),32*S,Canvas->SizeY-(252.f+PromptLift)*S,720*S,S);
  UFont* PromptFont=GEngine ? GEngine->GetMediumFont() : nullptr;
  const float TextScale=PromptFont ? 22.f*S/FMath::Max(1.f,float(PromptFont->GetMaxCharHeight())) : S;
  DrawText(FString::Printf(TEXT("[%s] 重组浮台"),*InteractionPC->GetCombatKey(5).GetDisplayName().ToString()),FColor(228,208,165),50*S,Canvas->SizeY-(244.f+PromptLift)*S,PromptFont,TextScale);
  DrawText(TEXT("站上平台升起 · 离开平台下落 · T切换视角"),FColor(215,209,193),50*S,Canvas->SizeY-(206.f+PromptLift)*S,PromptFont,.82f*TextScale);
 }
 else if (Canvas && LegacyPrompt && IsPickupPromptVisible()) {
  if (auto* Texture=LoadedPickupPrompt.Get()) {
   const float Scale=FMath::Min(Canvas->SizeX/1920.f,Canvas->SizeY/1080.f);
   // UV window skips the PNG's transparent margins without changing its pixels.
   DrawTexture(Texture,40.f*Scale,Canvas->SizeY-(290.f+PromptLift)*Scale,520.f*Scale,125.f*Scale,.14f,.23f,.72f,.52f,FLinearColor::White,BLEND_Translucent);
   const auto* PC=Cast<ACC_PlayerController>(PlayerOwner);
   if (PC && PC->GetCombatKey(5)!=EKeys::E) DrawText(PC->GetCombatKey(5).GetDisplayName().ToString(),FColor(230,210,170),48.f*Scale,Canvas->SizeY-(310.f+PromptLift)*Scale);
  }
 }
	if (!bShowDebugPanel) return;
	ACC_PlayerState* PS = PlayerOwner ? PlayerOwner->GetPlayerState<ACC_PlayerState>() : nullptr;
	if (!PS) return;
	UCC_ProgressionComponent* P = PS->Progression;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.65f), 10.f, 20.f, 530.f, 620.f);
	float Y = 32.f;
	auto Line = [this, &Y](const FString& Text) { DrawText(Text, FColor::White, 20.f, Y, GEngine ? GEngine->GetMediumFont() : nullptr, 0.8f); Y += 22.f; };
	Line(TEXT("FLAT TEST ARENA | Bronze / Ceramic / Nature"));
	Line(FString::Printf(TEXT("XP: %d | %s | Tier: %d/3"), P->Experience, P->CurrentForm ? *P->CurrentForm->DisplayName.ToString() : TEXT("Native"), P->CurrentTier + 1));
	if (const ACC_BaseCharacter* C = Cast<ACC_BaseCharacter>(PlayerOwner->GetPawn()))
		if (const UCC_AttributeSet* AS = Cast<UCC_AttributeSet>(C->GetAttributeSet()))
		{
			Line(FString::Printf(TEXT("HP: %.0f / %.0f | Shield: %.0f"), AS->GetHealth(), AS->GetMaxHealth(), AS->GetShield()));
			Line(FString::Printf(TEXT("Mana: %.0f / %.0f"), AS->GetMana(), AS->GetMaxMana()));
			Line(FString::Printf(TEXT("Attack: %.2f | Defense: %.0f | Speed: %.0f"), AS->GetAttack(), AS->GetDefense(), AS->GetMoveSpeed()));
		}
 	if (const auto* PC=Cast<ACC_PlayerController>(PlayerOwner)) Line(FString::Printf(TEXT("LMB / %s: attack | %s: interact | RMB: combat HUD | Tab: menu"), *PC->GetCombatKey(4).GetDisplayName().ToString(), *PC->GetCombatKey(5).GetDisplayName().ToString()));
	Line(TEXT("WASD: move | Mouse: look | Space: jump"));
	Line(TEXT("Tab: menu | F1: evolve | F2: nature"));
	Line(FString::Printf(TEXT("Next evolution: %d XP (-1 = unavailable)"), P->GetEvolutionCost()));
	Line(TEXT("Green: Bronze | Yellow: Ceramic | Native: Nature"));
	for (int32 I = 0; I < P->ArtifactForms.Num() && I < 4; ++I)
	{
		const UCC_FormDefinition* Form = P->ArtifactForms[I];
		if (!Form || !Form->bAvailable) continue;
		const FCC_FormProgress* Progress = P->Forms.FindByPredicate([Form](const FCC_FormProgress& Item) { return Item.Form == Form; });
		Line(FString::Printf(TEXT("F%d: %s | %s"), I + 3, Form ? *Form->DisplayName.ToString() : TEXT("Missing"), Progress ? TEXT("Unlocked") : TEXT("Interact to unlock")));
	}
	for (int32 I = 0; I < P->UpgradeCatalog.Num() && I < 6; ++I)
	{
		UCC_UpgradeDefinition* Upgrade = P->UpgradeCatalog[I];
		if (!Upgrade) continue;
		const int32 Level = P->GetUpgradeLevel(Upgrade);
		Line(FString::Printf(TEXT("F%d: %s | Lv %d | Cost %d"), I + 7, *Upgrade->DisplayName.ToString(), Level, Upgrade->Prices.IsValidIndex(Level) ? Upgrade->Prices[Level] : -1));
	}
	if (const ACC_BaseCharacter* Character = Cast<ACC_BaseCharacter>(PlayerOwner->GetPawn()); Character && Character->Skills->CurrentLoadout)
		for (int32 I = 0; I < 4; ++I)
		{
			const FCC_SkillSlot& Slot = Character->Skills->CurrentSlots[I];
			if (Slot.Skill) Line(FString::Printf(TEXT("%s: %s | Lv %d | CD %.1f"), Cast<ACC_PlayerController>(PlayerOwner) ? *Cast<ACC_PlayerController>(PlayerOwner)->GetCombatKey(I).GetDisplayName().ToString() : TEXT("?"), *Slot.Skill->DisplayName.ToString(), Slot.Level + P->GetSkillBonus(Slot.Skill->SkillId), Character->Skills->GetCooldownRemaining(I)));
		}
	FVector Eye; FRotator View;
	PlayerOwner->GetPlayerViewPoint(Eye, View);
	FHitResult Hit; FCollisionQueryParams Params; Params.AddIgnoredActor(PlayerOwner->GetPawn());
	GetWorld()->LineTraceSingleByChannel(Hit, Eye, Eye + View.Vector() * 1500.f, ECC_Visibility, Params);
	if (const AActor* Focused = Hit.GetActor())
	{
		if (const UCC_ArtifactComponent* Artifact = Focused->FindComponentByClass<UCC_ArtifactComponent>(); Artifact && Artifact->Form && Artifact->Form->bAvailable)
			Line(FString::Printf(TEXT("[E] %s | %s"), *Artifact->Form->DisplayName.ToString(), Artifact->CanInteract(Cast<ACC_BaseCharacter>(PlayerOwner->GetPawn())) ? TEXT("Ready") : TEXT("Move closer")));
		if (const ACC_BaseCharacter* Target = Cast<ACC_BaseCharacter>(Focused))
			if (const UCC_AttributeSet* AS = Cast<UCC_AttributeSet>(Target->GetAttributeSet())) Line(FString::Printf(TEXT("Target HP: %.0f / %.0f | ATK %.2f"), AS->GetHealth(), AS->GetMaxHealth(), AS->GetAttack()));
	}
	if(bFirstPerson&&!FirstPersonScanHUD)DrawText(TEXT("+"), FColor::White, Canvas->SizeX * 0.5f - 4.f, Canvas->SizeY * 0.5f - 8.f);
}

void ACC_ProgressionHUD::EndPlay(const EEndPlayReason::Type Reason)
{
 if(FirstPersonScanHUD){FirstPersonScanHUD->RemoveFromParent();FirstPersonScanHUD=nullptr;}
 if(PlayerHealthHUD){PlayerHealthHUD->RemoveFromParent();PlayerHealthHUD=nullptr;}
 Super::EndPlay(Reason);
}
