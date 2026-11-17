#if WITH_DEV_AUTOMATION_TESTS
// Dedicated game-process smoke probe. No progress/key save APIs are invoked.
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_EnemyCharacter.h"
#include "Characters/CC_PatrolEnemyCharacter.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_ArtifactComponent.h"
#include "Progression/CC_SkillComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "UI/CC_MuralMenuWidget.h"
#include "UI/CC_OptionsWidget.h"
#include "UI/CC_MenuArtData.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/UnrealType.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

namespace
{
template<class T> T* MuralObject(UObject* Owner, const TCHAR* Name)
{
    if (!Owner) return nullptr;
    const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Owner->GetClass(), Name);
    return Property ? Cast<T>(Property->GetObjectPropertyValue_InContainer(Owner)) : nullptr;
}

int32 MuralFormOption(UObject* Owner, const TCHAR* Name, const UCC_FormDefinition* Form)
{
    const FArrayProperty* Property = Owner ? FindFProperty<FArrayProperty>(Owner->GetClass(), Name) : nullptr;
    const FObjectPropertyBase* Inner = Property ? CastField<FObjectPropertyBase>(Property->Inner) : nullptr;
    if (!Inner) return INDEX_NONE;
    FScriptArrayHelper Values(Property, Property->ContainerPtrToValuePtr<void>(Owner));
    for (int32 Index=0; Index<Values.Num(); ++Index)
        if (Inner->GetObjectPropertyValue(Values.GetRawPtr(Index)) == Form) return Index;
    return INDEX_NONE;
}

struct FMuralMenuProbe : TSharedFromThis<FMuralMenuProbe>
{
    TWeakObjectPtr<UWorld> World;
    TWeakObjectPtr<ACC_PlayerController> Controller;
    TWeakObjectPtr<ACC_PlayerCharacter> Player;
    TWeakObjectPtr<UCC_ProgressionComponent> Progression;
    TWeakObjectPtr<UCC_MuralMenuWidget> Menu;
    TWeakObjectPtr<UCC_FormDefinition> Ink, Nature, Bronze, Ceramic;
    FTimerHandle Timer;
    int32 Phase=0;
    bool Passed=true;
    TSharedPtr<FJsonObject> Checks=MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Images;
    FString CaptureDirectory;
    TArray<FString> CaptureFiles;
    TArray<FKey> OriginalKeys;
    struct FSaveSnapshot { FString Path; bool Existed=false; TArray<uint8> Bytes; };
    TArray<FSaveSnapshot> SaveSnapshots;

    void Check(bool Value, const FString& Label)
    {
        Passed &= Value;
        Checks->SetBoolField(Label, Value);
        UE_LOG(LogTemp, Display, TEXT("MURAL_%s: %d"), *Label, Value);
    }

    bool Invoke(UObject* Owner, const TCHAR* Function)
    {
        UFunction* Event = Owner ? Owner->FindFunction(Function) : nullptr;
        if (!Event) { Check(false, FString(TEXT("function_available_"))+Function); return false; }
        Owner->ProcessEvent(Event, nullptr);
        return true;
    }

    bool OpenMenu()
    {
        auto* PC=Controller.Get();
        if (!PC) return false;
        PC->OpenProgressionMenu();
        Menu=Cast<UCC_MuralMenuWidget>(PC->GetProgressionMenuWidget());
        Check(Menu.IsValid() && Menu->IsInViewport(), TEXT("mural_widget_open"));
        Check(PC->IsProgressionMenuOpen() && PC->bShowMouseCursor && PC->IsMoveInputIgnored() && PC->IsLookInputIgnored(), TEXT("menu_locks_gameplay_input"));
        return Menu.IsValid();
    }

    void Capture(const TCHAR* Name)
    {
        if (!Menu.IsValid()) return;
        const FString File=CaptureDirectory/FString(Name)+TEXT(".png");
        CaptureFiles.Add(File);
        FScreenshotRequest::RequestScreenshot(File, true, false);
        auto Image=MakeShared<FJsonObject>();
        Image->SetStringField(TEXT("name"), Name);
        Image->SetStringField(TEXT("file"), File);
        Image->SetStringField(TEXT("form"), Menu->GetDisplayedFormId().ToString());
        Image->SetNumberField(TEXT("tier"), Menu->GetDisplayedTier());
        Image->SetNumberField(TEXT("page"), static_cast<int32>(Menu->GetMenuPage()));
        Images.Add(MakeShared<FJsonValueObject>(Image));
    }

    void CheckArtwork(UCC_FormDefinition* Form, int32 Tier, const TCHAR* Portrait, const TCHAR* Title, const FString& Label)
    {
        auto* M=Menu.Get(); auto* P=Progression.Get();
        UTexture2D* ActualPortrait=M ? M->GetDisplayedPortraitTexture() : nullptr;
        UTexture2D* ActualTitle=M ? M->GetDisplayedTitleTexture() : nullptr;
        Check(P && P->GetActiveForm()==Form && P->CurrentTier==Tier, Label+TEXT("_active_form"));
        Check(M && Form && M->GetDisplayedFormId()==Form->FormId && M->GetDisplayedTier()==Tier, Label+TEXT("_displayed_form"));
        Check(ActualPortrait && ActualPortrait->GetName()==Portrait, Label+TEXT("_portrait"));
        Check(ActualTitle && ActualTitle->GetName()==Title, Label+TEXT("_title"));
        if (Form && Form->Stages.IsValidIndex(Tier))
        {
            const FCC_FormStage& S=Form->Stages[Tier];
            Check(ActualPortrait && S.MenuPortrait.Get()==ActualPortrait && ActualTitle && S.MenuTitle.Get()==ActualTitle, Label+TEXT("_reads_stage_refs"));
        }
        auto* TitleImage=MuralObject<UImage>(M, TEXT("TitleImage"));
        UCanvasPanelSlot* TitleSlot=nullptr;
        for (UWidget* W=TitleImage; W && !TitleSlot; W=W->GetParent()) TitleSlot=Cast<UCanvasPanelSlot>(W->Slot);
        Check(TitleSlot && TitleSlot->GetPosition().X>=1700 && TitleSlot->GetPosition().Y<=100, Label+TEXT("_title_upper_right"));
    }

    void CheckPage(ECC_MuralMenuPage Page, const FString& Label)
    {
        auto* M=Menu.Get();
        if (!M) { Check(false,Label); return; }
        M->ShowPage(Page);
        auto* Panel=MuralObject<UBorder>(M, TEXT("ContentPanel"));
        const bool Full=Page!=ECC_MuralMenuPage::Overview && !(Page==ECC_MuralMenuPage::Skills && M->GetSkillGallery());
        Check(M->GetMenuPage()==Page && Panel && Panel->GetVisibility()==(Full ? ESlateVisibility::Visible : ESlateVisibility::Collapsed), Label);
        const FArrayProperty* Array=FindFProperty<FArrayProperty>(M->GetClass(), TEXT("PageBoxes"));
        const FObjectPropertyBase* Inner=Array ? CastField<FObjectPropertyBase>(Array->Inner) : nullptr;
        bool OnePage=Inner!=nullptr;
        if (Inner)
        {
            FScriptArrayHelper Values(Array, Array->ContainerPtrToValuePtr<void>(M));
            OnePage=Values.Num()==4;
            for (int32 I=0; I<Values.Num(); ++I)
            {
                auto* Box=Cast<UVerticalBox>(Inner->GetObjectPropertyValue(Values.GetRawPtr(I)));
                const bool Expected=Page!=ECC_MuralMenuPage::Overview && I==static_cast<int32>(Page);
                OnePage &= Box && Box->GetVisibility()==(Expected ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
            }
        }
        Check(OnePage, Label+TEXT("_single_page_content"));
    }

    bool SelectBag(UCC_FormDefinition* Form, bool Use, const FString& Label)
    {
        auto* M=Menu.Get(); auto* P=Progression.Get();
        if (!M || !P) return false;
        M->ShowPage(ECC_MuralMenuPage::Inventory);
        const int32 Index=MuralFormOption(M,TEXT("BagOptions"),Form);
        Check(Index>=0, Label+TEXT("_stored_in_bag"));
        if (Index<0) return false;
        UCC_FormDefinition* Previous=P->GetActiveForm();
        UTexture2D* PreviousArt=M->GetDisplayedPortraitTexture();
        M->SelectBagForm(Form);
        Check(P->GetActiveForm()==Previous && M->GetDisplayedPortraitTexture()==PreviousArt, Label+TEXT("_browsing_does_not_transform"));
        if (Use) { Invoke(M,TEXT("UseBagPower")); Check(P->GetActiveForm()==Form,Label+TEXT("_explicit_use_transforms")); }
        return true;
    }

    void Acquire(UCC_FormDefinition* Form, const FString& Label)
    {
        auto* W=World.Get(); auto* P=Progression.Get(); auto* C=Player.Get();
        if (!W || !P || !C || !Form) { Check(false,Label+TEXT("_fixture_ready")); return; }
        AActor* Artifact=nullptr;
        UCC_ArtifactComponent* Component=nullptr;
        for (TActorIterator<AActor> It(W); It; ++It)
            if (auto* A=It->FindComponentByClass<UCC_ArtifactComponent>(); A && A->Form==Form && A->bEnabled && !It->IsHidden()) { Artifact=*It; Component=A; break; }
        Check(Artifact && Component,Label+TEXT("_scene_artifact_exists"));
        if (!Artifact || !Component) return;
        // Try unobstructed positions inside the real actor's 250 cm/LOS check.
        bool CanInteract=false;
        for (const FVector& Offset : {FVector(160,0,100),FVector(-160,0,100),FVector(0,160,100),FVector(0,-160,100)})
        {
            C->SetActorLocation(Artifact->GetActorLocation()+Offset,false,nullptr,ETeleportType::TeleportPhysics);
            if (Component->CanInteract(C)) { CanInteract=true; break; }
        }
        Check(CanInteract,Label+TEXT("_distance_and_los_valid"));
        const auto* Before=P->GetActiveForm();
        const int32 TierBefore=P->CurrentTier, XPBefore=P->Experience;
        const auto* AS=Cast<UCC_AttributeSet>(C->GetAttributeSet());
        const float HealthBefore=AS ? AS->GetMaxHealth() : 0.f, AttackBefore=AS ? AS->GetAttack() : 0.f;
        P->Interact(Artifact);
        const bool Owned=P->Forms.ContainsByPredicate([Form](const FCC_FormProgress& Item) { return Item.Form==Form; });
        Check(Owned,Label+TEXT("_interaction_stores_power"));
        Check(P->GetActiveForm()==Before && P->CurrentTier==TierBefore && P->Experience==XPBefore,Label+TEXT("_interaction_does_not_transform_or_spend"));
        Check(AS && AS->GetMaxHealth()==HealthBefore && AS->GetAttack()==AttackBefore,Label+TEXT("_acquisition_preserves_stats"));
        const int32 Stored=P->Forms.Num();
        P->Interact(Artifact);
        Check(P->Forms.Num()==Stored && P->GetActiveForm()==Before,Label+TEXT("_repeat_interaction_deduplicated"));
        C->SetActorLocation(FVector(-3500,-3500,110),false,nullptr,ETeleportType::TeleportPhysics);
    }

    void EvolveTo(int32 Tier, const FString& Label)
    {
        auto* P=Progression.Get(); if (!P) return;
        const int32 ExpectedCost=Tier==1 ? 100 : 1000;
        Check(P->GetEvolutionCost()==ExpectedCost,Label+TEXT("_price"));
        P->AddExperience(ExpectedCost);
        const int32 Before=P->Experience;
        Invoke(Menu.Get(),TEXT("EvolveBagPower"));
        Check(P->CurrentTier==Tier && P->Experience==Before-ExpectedCost,Label+TEXT("_tier_and_payment"));
    }

    void Finish()
    {
        const TSharedRef<FMuralMenuProbe> KeepAlive=AsShared();
        auto* W=World.Get();
        if (W) W->GetTimerManager().ClearTimer(Timer);
        for (int32 I=0; I<SaveSnapshots.Num(); ++I)
        {
            const auto& Before=SaveSnapshots[I]; TArray<uint8> After;
            const bool Exists=FFileHelper::LoadFileToArray(After,*Before.Path);
            Check(Exists==Before.Existed && After==Before.Bytes,FString::Printf(TEXT("user_save_%d_unchanged"),I));
        }
        auto* PC=Controller.Get();
        bool KeysUnchanged=PC && OriginalKeys.Num()==6;
        if (PC) for (int32 I=0; I<OriginalKeys.Num(); ++I) KeysUnchanged &= PC->GetCombatKey(I)==OriginalKeys[I];
        Check(KeysUnchanged,TEXT("user_key_settings_unchanged"));
        for (const FString& File:CaptureFiles) Check(IFileManager::Get().FileSize(*File)>0,FString(TEXT("screenshot_saved_"))+FPaths::GetBaseFilename(File));
        Check(CaptureFiles.Num()>=6,TEXT("six_or_more_separate_frame_captures"));
        auto Root=MakeShared<FJsonObject>(); Root->SetBoolField(TEXT("passed"),Passed);
        Root->SetObjectField(TEXT("checks"),Checks); Root->SetArrayField(TEXT("screenshots"),Images);
        Root->SetStringField(TEXT("captureDirectory"),CaptureDirectory);
        Root->SetBoolField(TEXT("artificialUnlockFixtureUsed"),false);
        Root->SetBoolField(TEXT("progressSaveRequested"),false);
        FString Text; FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Text));
        const FString File=FPaths::ProjectSavedDir()/TEXT("MuralMenuSmoke.json");
        FFileHelper::SaveStringToFile(Text,*File);
        UE_LOG(LogTemp,Display,TEXT("MURAL_MENU_SMOKE_%s: %s"),Passed ? TEXT("PASS") : TEXT("FAIL"),*File);
        if (GEngine && W) GEngine->Exec(W,TEXT("QUIT"));
    }

    void Step()
    {
        auto* W=World.Get(); if (!W) return;
        if (Phase==0)
        {
            auto* PC=Cast<ACC_PlayerController>(W->GetFirstPlayerController());
            auto* C=PC ? Cast<ACC_PlayerCharacter>(PC->GetPawn()) : nullptr;
            auto* PS=PC ? PC->GetPlayerState<ACC_PlayerState>() : nullptr;
            auto* P=PS ? PS->Progression.Get() : nullptr;
            Controller=PC; Player=C; Progression=P;
            Check(PC && C && P,TEXT("player_ready_after_startup"));
            if (!PC || !C || !P) { Finish(); return; }
            CaptureDirectory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Screenshots/MuralMenu")/FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S")));
            IFileManager::Get().MakeDirectory(*CaptureDirectory,true);
            for (const TCHAR* Slot : {TEXT("SpiritTales"),TEXT("SpiritTalesControls")})
            {
                FSaveSnapshot Snapshot; Snapshot.Path=FPaths::ProjectSavedDir()/TEXT("SaveGames")/(FString(Slot)+TEXT(".sav"));
                Snapshot.Existed=FFileHelper::LoadFileToArray(Snapshot.Bytes,*Snapshot.Path); SaveSnapshots.Add(MoveTemp(Snapshot));
            }
            for (int32 I=0; I<6; ++I) OriginalKeys.Add(PC->GetCombatKey(I));
            for (TActorIterator<ACC_PatrolEnemyCharacter> It(W); It; ++It) It->StopPatrol();
            for (TActorIterator<ACC_EnemyCharacter> It(W); It; ++It) if (auto* ASC=It->GetAbilitySystemComponent()) ASC->CancelAllAbilities();
            C->SetActorLocation(FVector(-3500,-3500,110),false,nullptr,ETeleportType::TeleportPhysics);
            C->GetCharacterMovement()->DisableMovement();
            Ink=P->NativeForm.Get();
            Nature=LoadObject<UCC_FormDefinition>(nullptr,TEXT("/Game/HPWH/Forms/DA_Form_Native.DA_Form_Native"));
            Bronze=LoadObject<UCC_FormDefinition>(nullptr,TEXT("/Game/HPWH/Forms/DA_Form_Artifact_01.DA_Form_Artifact_01"));
            Ceramic=LoadObject<UCC_FormDefinition>(nullptr,TEXT("/Game/HPWH/Forms/DA_Form_Artifact_02.DA_Form_Artifact_02"));
            const auto* AS=Cast<UCC_AttributeSet>(C->GetAttributeSet());
            Check(Ink.IsValid() && Ink->FormId==TEXT("InitialInk") && P->GetActiveForm()==Ink.Get() && P->CurrentTier==0,TEXT("initial_ink_active"));
            Check(AS && FMath::IsNearlyEqual(AS->GetMaxHealth(),75.f) && FMath::IsNearlyEqual(AS->GetAttack(),11.f),TEXT("initial_ink_rounded_75_health_11_attack"));
            Check(P->Forms.IsEmpty() && P->Experience==0 && P->ManualSkills.IsEmpty(),TEXT("fresh_runtime_progress"));
            Check(Nature.IsValid() && Bronze.IsValid() && Ceramic.IsValid(),TEXT("three_routes_configured"));
            PC->CloseProgressionMenu();
            if (!OpenMenu()) { Finish(); return; }
            auto* M=Menu.Get();
            Check(M->ArtData && M->ArtData->Background.LoadSynchronous() && M->ArtData->Logo.LoadSynchronous() && M->ArtData->Navigation.Num()==4,TEXT("menu_common_art_configured"));
            bool NavComplete=M->ArtData && M->ArtData->Navigation.Num()==4;
            if (M->ArtData) for (const auto& Nav:M->ArtData->Navigation) NavComplete &= Nav.Normal.LoadSynchronous() && Nav.Active.LoadSynchronous();
            Check(NavComplete,TEXT("all_navigation_states_loaded"));
            Check(M->GetMenuPage()==ECC_MuralMenuPage::Overview,TEXT("opens_on_overview"));
            CheckPage(ECC_MuralMenuPage::Overview,TEXT("overview_layout"));
            CheckArtwork(Ink.Get(),0,TEXT("T_Portrait_Ink_01"),TEXT("T_Title_Ink"),TEXT("ink"));
            // First frame can still compile newly imported texture resources. Capture next step.
        }
        else
        {
            auto* PC=Controller.Get(); auto* P=Progression.Get(); auto* M=Menu.Get(); auto* C=Player.Get();
            if (!PC || !P || !C || !M) { Check(false,TEXT("objects_remain_valid")); Finish(); return; }
            switch (Phase)
            {
            case 1:
                Capture(TEXT("OverviewInk"));
                break;
            case 2:
                PC->CloseProgressionMenu(); Menu.Reset();
                Acquire(Nature.Get(),TEXT("nature_white")); Acquire(Bronze.Get(),TEXT("bronze_green")); Acquire(Ceramic.Get(),TEXT("ceramic_yellow"));
                if (!OpenMenu()) { Finish(); return; }
                CheckPage(ECC_MuralMenuPage::Inventory,TEXT("inventory_page"));
                Check(MuralObject<UObject>(Menu.Get(),TEXT("BagUseButton")) && MuralObject<UObject>(Menu.Get(),TEXT("BagEvolveButton")),TEXT("inventory_switch_and_evolve_controls_present"));
                SelectBag(Nature.Get(),false,TEXT("nature_preview"));
                CheckArtwork(Ink.Get(),0,TEXT("T_Portrait_Ink_01"),TEXT("T_Title_Ink"),TEXT("ink_after_acquisition"));
                Capture(TEXT("Inventory"));
                break;
            case 3:
            {
                CheckPage(ECC_MuralMenuPage::Skills,TEXT("skills_page"));
                auto* Choice=MuralObject<UComboBoxString>(M,TEXT("SkillChoice"));
                Check(P->SkillCatalog.Num()>=22 && Choice && Choice->GetOptionCount()==P->SkillCatalog.Num(),TEXT("all_22_skills_listed"));
                bool Descriptions=true; for (const auto& Skill:P->SkillCatalog) Descriptions &= Skill && !Skill->Description.IsEmpty();
                Check(Descriptions,TEXT("all_skill_descriptions_available"));
                if (Choice) Choice->SetSelectedIndex(0);
                M->SelectSkillSlot(0); Invoke(M,TEXT("Equip"));
                Check(!P->SkillCatalog.IsEmpty() && C->Skills->CurrentSlots.IsValidIndex(0) && C->Skills->CurrentSlots[0].Skill==P->SkillCatalog[0] && P->ManualSkills.Num()==4,TEXT("skill_panel_equips_particle_on_server"));
                auto* Text=MuralObject<UTextBlock>(M,TEXT("Details"));
                Check(Text && Text->GetText().ToString().Contains(TEXT("75.0%")) && Text->GetText().ToString().Contains(TEXT("冷却")),TEXT("skill_panel_numeric_description"));
                auto* Feedback=MuralObject<UTextBlock>(M,TEXT("Feedback"));
                Check(Feedback && Feedback->GetText().ToString().Contains(TEXT("成功")),TEXT("skill_success_result_reaches_panel"));
                Capture(TEXT("Skills"));
                break;
            }
            case 4:
            {
                CheckPage(ECC_MuralMenuPage::Trade,TEXT("trade_page"));
                auto* Choice=MuralObject<UComboBoxString>(M,TEXT("RouteChoice"));
                const int32 Index=MuralFormOption(M,TEXT("RouteOptions"),Bronze.Get());
                if (Choice && Index>=0) Choice->SetSelectedIndex(Index);
                Check(Choice && Index>=0 && P->GetActiveForm()==Ink.Get(),TEXT("trade_route_preview_does_not_transform"));
                CheckArtwork(Ink.Get(),0,TEXT("T_Portrait_Ink_01"),TEXT("T_Title_Ink"),TEXT("ink_trade_preview"));
                Capture(TEXT("Trade"));
                break;
            }
            case 5:
                CheckPage(ECC_MuralMenuPage::Settings,TEXT("settings_page"));
                Check(M->GetOptionsPanel() && M->GetOptionsPanel()->GetKeySelector(0) && M->GetOptionsPanel()->GetKeySelector(5),TEXT("key_configuration_controls_present"));
                Capture(TEXT("Settings"));
                break;
            case 6:
                SelectBag(Nature.Get(),true,TEXT("use_nature"));
                CheckArtwork(Nature.Get(),0,TEXT("T_Portrait_Nature_01"),TEXT("T_Title_Nature"),TEXT("nature_initial"));
                EvolveTo(1,TEXT("nature_mid"));
                CheckArtwork(Nature.Get(),1,TEXT("T_Portrait_Nature_03"),TEXT("T_Title_Nature"),TEXT("nature_mid"));
                EvolveTo(2,TEXT("nature_high"));
                CheckArtwork(Nature.Get(),2,TEXT("T_Portrait_Nature_04"),TEXT("T_Title_Nature"),TEXT("nature_high"));
                Capture(TEXT("NatureHigh"));
                break;
            case 7:
                SelectBag(Bronze.Get(),true,TEXT("use_bronze"));
                CheckArtwork(Bronze.Get(),0,TEXT("T_Portrait_Bronze_01"),TEXT("T_Title_Bronze"),TEXT("bronze_initial"));
                EvolveTo(1,TEXT("bronze_mid"));
                CheckArtwork(Bronze.Get(),1,TEXT("T_Portrait_Bronze_02"),TEXT("T_Title_Bronze"),TEXT("bronze_mid"));
                Capture(TEXT("BronzeMid"));
                break;
            case 8:
                EvolveTo(2,TEXT("bronze_high"));
                CheckArtwork(Bronze.Get(),2,TEXT("T_Portrait_Bronze_03"),TEXT("T_Title_Bronze"),TEXT("bronze_high"));
                SelectBag(Ceramic.Get(),true,TEXT("use_ceramic"));
                CheckArtwork(Ceramic.Get(),0,TEXT("T_Portrait_Ceramic_01"),TEXT("T_Title_Ceramic"),TEXT("ceramic_initial"));
                EvolveTo(1,TEXT("ceramic_mid"));
                CheckArtwork(Ceramic.Get(),1,TEXT("T_Portrait_Ceramic_03"),TEXT("T_Title_Ceramic"),TEXT("ceramic_mid"));
                EvolveTo(2,TEXT("ceramic_high"));
                CheckArtwork(Ceramic.Get(),2,TEXT("T_Portrait_Ceramic_05"),TEXT("T_Title_Ceramic"),TEXT("ceramic_high"));
                Capture(TEXT("CeramicHigh"));
                break;
            case 9:
                SelectBag(Ink.Get(),true,TEXT("return_ink"));
                CheckArtwork(Ink.Get(),0,TEXT("T_Portrait_Ink_01"),TEXT("T_Title_Ink"),TEXT("returned_ink"));
                SelectBag(Nature.Get(),true,TEXT("retained_nature")); Check(P->CurrentTier==2,TEXT("nature_tier_retained"));
                SelectBag(Bronze.Get(),true,TEXT("retained_bronze")); Check(P->CurrentTier==2,TEXT("bronze_tier_retained"));
                SelectBag(Ceramic.Get(),true,TEXT("retained_ceramic")); Check(P->CurrentTier==2,TEXT("ceramic_tier_retained"));
                Check(P->Experience==0 && P->Forms.Num()==3,TEXT("three_inventory_powers_and_exact_evolution_payments"));
                Check(!P->SkillCatalog.IsEmpty() && P->ManualSkills.Num()==4 && C->Skills->CurrentSlots.IsValidIndex(0) && C->Skills->CurrentSlots[0].Skill==P->SkillCatalog[0],TEXT("manual_skill_survives_all_form_changes"));
                Invoke(M,TEXT("Close"));
                Check(!PC->IsProgressionMenuOpen() && !PC->GetProgressionMenuWidget() && !PC->IsMoveInputIgnored() && !PC->IsLookInputIgnored() && !PC->bShowMouseCursor,TEXT("close_restores_input_and_removes_widget"));
                Menu.Reset();
                if (!OpenMenu()) { Finish(); return; }
                Check(Menu->GetMenuPage()==ECC_MuralMenuPage::Overview,TEXT("reopen_resets_to_overview"));
                CheckArtwork(Ceramic.Get(),2,TEXT("T_Portrait_Ceramic_05"),TEXT("T_Title_Ceramic"),TEXT("reopened_current_form"));
                break;
            default:
                Finish(); return;
            }
        }
        ++Phase;
    }
};

void RunMuralMenuSmoke(UWorld* World)
{
    if (!World || !World->IsGameWorld() || World->GetNetMode()!=NM_Standalone)
    {
        UE_LOG(LogTemp,Warning,TEXT("CC.MuralMenuSmokeTest requires a dedicated standalone -game process; no editor worlds or network clients."));
        return;
    }
    auto Probe=MakeShared<FMuralMenuProbe>(); Probe->World=World;
    // One screenshot request per timer step; several rendered frames separate them.
    World->GetTimerManager().SetTimer(Probe->Timer,[Probe]() { Probe->Step(); },.8f,true,3.f);
}
FAutoConsoleCommandWithWorld MuralSmokeCommand(
    TEXT("CC.MuralMenuSmokeTest"),
    TEXT("Dedicated standalone game smoke: acquired powers, mural artwork, four pages, skill equipment and screenshots; no player saves modified. Requires rendering."),
    FConsoleCommandWithWorldDelegate::CreateStatic(&RunMuralMenuSmoke));
}
#endif
