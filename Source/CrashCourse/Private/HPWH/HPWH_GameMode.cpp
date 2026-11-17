#include "HPWH/HPWH_GameMode.h"

#include "HPWH/HPWH_World.h"
#include "HPWH/HPWH_Player.h"
#include "HPWH/HPWH_PlayerController.h"
#include "HPWH/HPWH_Crawler.h"
#include "HPWH/HPWH_Companion.h"
#include "HPWH/HPWH_Enemies.h"
#include "HPWH/HPWH_POI.h"
#include "HPWH/HPWH_ExperienceOrb.h"
#include "HPWH/HPWH_SaveGame.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "TimerManager.h"
#include "UnrealClient.h"

AHPWH_GameMode::AHPWH_GameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AHPWH_PlayerController::StaticClass();
	HUDClass = nullptr;
	EscapeStream.Initialize(20261117);
}

namespace
{
	/** 升级所需经验：10、15、20，之后 40、60、80、100、120…… */
	int32 HPWH_ExperienceRequiredForLevel(int32 CurrentLevel)
	{
		if (CurrentLevel <= 1)
		{
			return 10;
		}
		if (CurrentLevel == 2)
		{
			return 15;
		}
		if (CurrentLevel == 3)
		{
			return 20;
		}
		return 40 + (CurrentLevel - 4) * 20;
	}
}

void AHPWH_GameMode::StartPlay()
{
	Super::StartPlay();

	RunSeed = InitialRunSeed;
	EscapeStream.Initialize(RunSeed ^ 0x455343);
	UpgradeStream.Initialize(RunSeed ^ 0x555047);
	ExperienceToNextLevel = HPWH_ExperienceRequiredForLevel(PlayerLevel);
	RecomputeChance();
	EnsureRuntimeLighting();
	GAreScreenMessagesEnabled = false;

	if (AHPWH_WorldDirector* Director = GetDirector())
	{
		Director->InitializeForRun(RunSeed, LayerIndex);
		bDirectorInitialized = true;
	}
	UE_LOG(LogTemp, Display, TEXT("HPWH: GameMode StartPlay run=%d layer=%d chance=%.0f"), RunSeed, LayerIndex, RawChance);

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugPOI")))
	{
		if (AHPWH_WorldDirector* Director = GetDirector())
		{
			if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
			{
				int32 POIIndex = 0;
				FParse::Value(FCommandLine::Get(), TEXT("HPWHDebugPOIIndex="), POIIndex);
				const FIntPoint Cell = Director->GetPOICell(POIIndex);
				Body->TeleportTo(Director->CellCenter(Cell) + FVector(320.f, 0.f, 140.f), FRotator::ZeroRotator);
				UE_LOG(LogTemp, Display, TEXT("HPWH: debug teleport to poi index=%d (%d,%d)"),
					POIIndex, Cell.X, Cell.Y);
			}
		}
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugBiome")))
	{
		if (AHPWH_WorldDirector* Director = GetDirector())
		{
			if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
			{
				int32 BiomeIndex = 1;
				FParse::Value(FCommandLine::Get(), TEXT("HPWHDebugBiome="), BiomeIndex);
				for (const TPair<FIntPoint, FHPWH_CellRecord>& Pair : Director->GetRecords())
				{
					if (static_cast<int32>(Pair.Value.Terrain) == BiomeIndex && !Pair.Value.bHasPOI)
					{
						Body->TeleportTo(Director->CellCenter(Pair.Key) + FVector(0.f, 0.f, 140.f), FRotator::ZeroRotator);
						UE_LOG(LogTemp, Display, TEXT("HPWH: debug teleport to biome=%d cell (%d,%d)"),
							BiomeIndex, Pair.Key.X, Pair.Key.Y);
						break;
					}
				}
			}
		}
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugLine")))
	{
		if (AHPWH_WorldDirector* Director = GetDirector())
		{
			if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
			{
				int32 LineCoord = 5;
				FParse::Value(FCommandLine::Get(), TEXT("HPWHDebugLineCoord="), LineCoord);
				const FIntPoint Cell(LineCoord, 0);
				Body->TeleportTo(Director->CellCenter(Cell) + FVector(0.f, 0.f, 140.f), FRotator::ZeroRotator);
				UE_LOG(LogTemp, Display, TEXT("HPWH: debug teleport to puzzle line (%d,%d)"), Cell.X, Cell.Y);
			}
		}
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugLaptop")))
	{
		if (AHPWH_WorldDirector* Director = GetDirector())
		{
			if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
			{
				const FIntPoint Cell(1, 0);
				Body->TeleportTo(Director->CellCenter(Cell) + FVector(260.f, 0.f, 140.f), FRotator::ZeroRotator);
				FActorSpawnParameters Params;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				if (AHPWH_LaptopPOI* Laptop = GetWorld()->SpawnActor<AHPWH_LaptopPOI>(
					AHPWH_LaptopPOI::StaticClass(), Director->CellCenter(Cell), FRotator::ZeroRotator, Params))
				{
					Laptop->InitializePOI(Director, Cell, false);
					FTimerHandle CatTimer;
					GetWorld()->GetTimerManager().SetTimer(CatTimer, FTimerDelegate::CreateLambda([Laptop, Body]()
					{
						if (IsValid(Laptop) && IsValid(Body))
						{
							Laptop->HPWHInteract(Body);
						}
					}), 1.f, false);
					UE_LOG(LogTemp, Display, TEXT("HPWH: debug laptop spawned at cell (%d,%d)"), Cell.X, Cell.Y);
				}
			}
		}
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugCrawler")))
	{
		if (AHPWH_Crawler* Crawler = GetCrawler())
		{
			Crawler->Activate(false);
			Crawler->DebugPlaceNearPlayer(1200.f);
			FTimerHandle HurtTimer;
			GetWorld()->GetTimerManager().SetTimer(HurtTimer, FTimerDelegate::CreateLambda([this]()
			{
				if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
				{
					Body->TakeHPWHDamage(5.f, false);
					UE_LOG(LogTemp, Display, TEXT("HPWH: debug player hurt for crawler speed test"));
				}
			}), 6.f, false);
		}
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugSoul")))
	{
		FTimerHandle SoulTimer;
		GetWorld()->GetTimerManager().SetTimer(SoulTimer, FTimerDelegate::CreateLambda([this]()
		{
			TriggerSoulOut();
		}), 2.f, false);
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugCompanion")))
	{
		if (AHPWH_Companion* Companion = GetCompanion())
		{
			if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
			{
				Body->TeleportTo(Companion->GetActorLocation() + FVector(260.f, 0.f, 80.f), FRotator::ZeroRotator);
				UE_LOG(LogTemp, Display, TEXT("HPWH: debug teleport to companion"));
			}
		}
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugCompanionJoin")))
	{
		FTimerHandle JoinTimer;
		GetWorld()->GetTimerManager().SetTimer(JoinTimer, FTimerDelegate::CreateLambda([this]()
		{
			AHPWH_Companion* Companion = GetCompanion();
			AHPWH_PlayerCharacter* Body = GetPlayerBody();
			if (Companion && Body)
			{
				Companion->HPWHInteract(Body);
			}
		}), 2.f, false);
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugCompanionDuel")))
	{
		FTimerHandle DuelTimer;
		GetWorld()->GetTimerManager().SetTimer(DuelTimer, FTimerDelegate::CreateLambda([this]()
		{
			AHPWH_Companion* Companion = GetCompanion();
			AHPWH_WorldDirector* Director = GetDirector();
			if (!Companion || !Director)
			{
				return;
			}
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			const FVector Location = Companion->GetActorLocation() + FVector(650.f, 0.f, 20.f);
			if (AHPWH_EnemyBase* Enemy = GetWorld()->SpawnActor<AHPWH_MeleeMonster>(
				AHPWH_MeleeMonster::StaticClass(), Location, FRotator::ZeroRotator, Params))
			{
				Enemy->InitEnemy(Director, FIntPoint(0, 0), false);
				UE_LOG(LogTemp, Display, TEXT("HPWH: debug companion duel enemy spawned"));
			}
		}), 3.f, false);
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugBetray")))
	{
		FTimerHandle BetrayTimer;
		GetWorld()->GetTimerManager().SetTimer(BetrayTimer, FTimerDelegate::CreateLambda([this]()
		{
			if (AHPWH_Companion* Companion = GetCompanion())
			{
				Companion->ForceBetray();
			}
		}), 4.f, false);
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugSoulReturn")))
	{
		FTimerHandle ReturnTimer;
		GetWorld()->GetTimerManager().SetTimer(ReturnTimer, FTimerDelegate::CreateLambda([this]()
		{
			AHPWH_SoulAvatar* Soul = SoulAvatar;
			AHPWH_PlayerCharacter* Body = GetPlayerBody();
			UE_LOG(LogTemp, Display, TEXT("HPWH: debug soul return tick soul=%d body=%d"),
				Soul ? 1 : 0, Body ? 1 : 0);
			if (Soul && Body)
			{
				Soul->SetActorLocation(Body->GetActorLocation() + FVector(0.f, 0.f, 120.f));
				Soul->TryReturnToBody();
				UE_LOG(LogTemp, Display, TEXT("HPWH: debug soul return attempted"));
			}
		}), 3.4f, false);
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugSoulKill")))
	{
		FTimerHandle SoulKillTimer;
		GetWorld()->GetTimerManager().SetTimer(SoulKillTimer, FTimerDelegate::CreateLambda([this]()
		{
			HandleSoulKilled();
		}), 5.f, false);
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugUpgrade")))
	{
		FTimerHandle UpgradeTimer;
		GetWorld()->GetTimerManager().SetTimer(UpgradeTimer, FTimerDelegate::CreateLambda([this]()
		{
			RequestUpgradeChoice();
		}), 1.5f, false);
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugBuffs")))
	{
		// 一次性发放覆盖五类图标的强化，用于验收梦境档案面板。
		struct FGrant
		{
			EHPWH_UpgradeType Type;
			EHPWH_Rarity Rarity;
		};
		static const FGrant Grants[] = {
			{ EHPWH_UpgradeType::AttackDamage, EHPWH_Rarity::Common },
			{ EHPWH_UpgradeType::BulletCooldown, EHPWH_Rarity::Epic },
			{ EHPWH_UpgradeType::MultiShotThree, EHPWH_Rarity::Legendary },
			{ EHPWH_UpgradeType::MaxHealth, EHPWH_Rarity::Epic },
			{ EHPWH_UpgradeType::KillHeal, EHPWH_Rarity::Common },
			{ EHPWH_UpgradeType::AttackRange, EHPWH_Rarity::Common },
			{ EHPWH_UpgradeType::InteractRange, EHPWH_Rarity::Epic },
			{ EHPWH_UpgradeType::MoveSpeed, EHPWH_Rarity::Epic },
			{ EHPWH_UpgradeType::SoulGuard, EHPWH_Rarity::Legendary },
			{ EHPWH_UpgradeType::SoulSpeed, EHPWH_Rarity::Common },
			{ EHPWH_UpgradeType::ReturnRange, EHPWH_Rarity::Common },
			{ EHPWH_UpgradeType::CrawlerSlow, EHPWH_Rarity::Epic },
			{ EHPWH_UpgradeType::PuzzleTime, EHPWH_Rarity::Common },
			{ EHPWH_UpgradeType::SuccessRate, EHPWH_Rarity::Legendary }
		};
		for (const FGrant& Grant : Grants)
		{
			ApplyUpgrade(HPWH_MakeUpgradeOption(Grant.Type, Grant.Rarity));
		}
		UE_LOG(LogTemp, Display, TEXT("HPWH: debug buffs granted=%d"), static_cast<int32>(UE_ARRAY_COUNT(Grants)));
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugPuzzleSolve")))
	{
		FTimerHandle SolveTimer;
		GetWorld()->GetTimerManager().SetTimer(SolveTimer, FTimerDelegate::CreateLambda([this]()
		{
			OnPuzzleSolved(EHPWH_TerrainType::Mirror);
		}), 2.f, false);
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugXP")))
	{
		FTimerHandle XpTimer;
		GetWorld()->GetTimerManager().SetTimer(XpTimer, FTimerDelegate::CreateLambda([this]()
		{
			if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
			{
				AHPWH_ExperienceOrb::SpawnOrb(this, Body->GetActorLocation() + FVector(520.f, 0.f, 120.f), 5);
				AHPWH_ExperienceOrb::SpawnOrb(this, Body->GetActorLocation() + FVector(0.f, 520.f, 120.f), 5);
				UE_LOG(LogTemp, Display, TEXT("HPWH: debug xp orbs spawned"));
			}
		}), 1.5f, false);
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugHurt")))
	{
		FTimerHandle HurtTimer;
		GetWorld()->GetTimerManager().SetTimer(HurtTimer, FTimerDelegate::CreateLambda([this]()
		{
			if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
			{
				Body->TakeHPWHDamage(5.f, false);
				UE_LOG(LogTemp, Display, TEXT("HPWH: debug hurt applied"));
			}
		}), 2.f, false);
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugShot")))
	{
		// 三连发 + 冷却缩减，并在身边放三只测试怪，方便验证射击与卡片效果。
		RunStats.ProjectileCount = 3;
		RunStats.BulletCooldownReduction = 0.12f;
		if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
		{
			for (int32 Index = 0; Index < 3; ++Index)
			{
				const FVector Location = Body->GetActorLocation() +
					FVector(450.f, (Index - 1) * 180.f, 0.f);
				if (AHPWH_EnemyBase* Enemy = GetWorld()->SpawnActor<AHPWH_EnemyBase>(
					AHPWH_MeleeMonster::StaticClass(), Location, FRotator::ZeroRotator))
				{
					Enemy->InitEnemy(GetDirector(), FIntPoint::ZeroValue, false);
				}
			}
			UE_LOG(LogTemp, Display, TEXT("HPWH: debug shot spawned bullets=3 cd=1.88"));
		}
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugEnemies")))
	{
		// 三只测试怪：近身追击、远程拉扯、远处巡逻各一只。
		if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
		{
			const FVector Origin = Body->GetActorLocation();
			auto SpawnEnemy = [&](UClass* Class, const FVector& Offset)
			{
				FActorSpawnParameters Params;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				if (AHPWH_EnemyBase* Enemy = GetWorld()->SpawnActor<AHPWH_EnemyBase>(
					Class, Origin + Offset, FRotator::ZeroRotator, Params))
				{
					Enemy->InitEnemy(GetDirector(), FIntPoint::ZeroValue, false);
					UE_LOG(LogTemp, Display, TEXT("HPWH: debug enemy %s spawned at (%.1f,%.1f)"),
						*Enemy->GetName(), Enemy->GetActorLocation().X / 100.f, Enemy->GetActorLocation().Y / 100.f);
				}
			};
			SpawnEnemy(AHPWH_MeleeMonster::StaticClass(), FVector(700.f, 0.f, 0.f));
			SpawnEnemy(AHPWH_RangedMonster::StaticClass(), FVector(1300.f, 600.f, 0.f));
			SpawnEnemy(AHPWH_MeleeMonster::StaticClass(), FVector(1900.f, 0.f, 0.f));
		}
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugCoin")))
	{
		if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
		{
			if (AHPWH_WorldDirector* Director = GetDirector())
			{
				const FIntPoint Cell = Director->WorldToCell(Body->GetActorLocation());
				const FVector Location = Director->CellCenter(Cell) + FVector(280.f, 0.f, 100.f);
				if (GetWorld()->SpawnActor<AHPWH_CoinMonster>(
					AHPWH_CoinMonster::StaticClass(), Location, FRotator::ZeroRotator))
				{
					UE_LOG(LogTemp, Display, TEXT("HPWH: debug coin spawned at cell (%d,%d)"), Cell.X, Cell.Y);
				}
			}
		}
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHScreenshot")))
	{
		// 用真实时间触发截图，避免三选一的时间减速把截图推迟。
		const double Delay = FParse::Param(FCommandLine::Get(), TEXT("HPWHScreenshotLate")) ? 22.0 : 6.0;
		ScreenshotAtRealTime = FPlatformTime::Seconds() + Delay;
	}

	LoadMusic();
	LoadSfx();
	PlayMusic(BgmCalm);

	// 音效试听：按 EHPWH_Sfx 顺序每 1.5 秒播一个，方便替换资产后快速验收。
	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugSfx")))
	{
		bDebugSfx = true;
		NextDebugSfxTime = FPlatformTime::Seconds() + 1.0;
	}

	bMainMenuActive = true;
	MenuPage = EHPWH_MenuPage::Main;
	if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
	{
		Body->SetRunLocked(true);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetWorldSettings()->SetTimeDilation(0.001f);
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHAutoStart")))
	{
		StartGameFromMenu();
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHContinue")))
	{
		if (LoadProgress())
		{
			StartGameFromMenu();
		}
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHMenuSettings")))
	{
		MenuPage = EHPWH_MenuPage::Settings;
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHInGameMenu")))
	{
		ToggleInGameMenu();
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugSave")))
	{
		SaveProgress();
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHReturnMenu")))
	{
		StartGameFromMenu();
		ReturnToMainMenu();
	}
	static bool bPortalDebugOnce = false;
	if (!bPortalDebugOnce && FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugPortal")))
	{
		bPortalDebugOnce = true;
		SolvedPuzzleTypes.Add(EHPWH_TerrainType::Mirror);
		SolvedPuzzleTypes.Add(EHPWH_TerrainType::Courtyard);
		SolvedPuzzleTypes.Add(EHPWH_TerrainType::Laptop);
		SpawnFloorPortal();
	}
	if (!bPortalDebugOnce &&
		(FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugPortalWin")) ||
			FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugPortalLose"))))
	{
		bPortalDebugOnce = true;
		SolvedPuzzleTypes.Add(EHPWH_TerrainType::Mirror);
		SolvedPuzzleTypes.Add(EHPWH_TerrainType::Courtyard);
		SolvedPuzzleTypes.Add(EHPWH_TerrainType::Laptop);
		SpawnFloorPortal();
		OpenFloorPortalChoice();
		bPortalChoiceSuccess = FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugPortalWin"));
	}
}

void AHPWH_GameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	RefreshMusicState();

	// 三选一使用世界时间减速，调试自动选择则按真实时间推进。
	if (bUpgradeSelectionActive && bDebugAutoReroll && UpgradeShownRealTime > 0.0 &&
		FPlatformTime::Seconds() - UpgradeShownRealTime > 2.0)
	{
		bDebugAutoReroll = false;
		RerollUpgrade();
	}
	if (bUpgradeSelectionActive && bAutoPickDebug && UpgradeShownRealTime > 0.0)
	{
		if (FPlatformTime::Seconds() - UpgradeShownRealTime > 4.0)
		{
			ChooseUpgrade(0);
		}
	}

	if (bPortalChoiceActive && PortalChoiceRealTime > 0.0 &&
		FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugPortalAutoConfirm")) &&
		FPlatformTime::Seconds() - PortalChoiceRealTime > 4.0)
	{
		ConfirmFloorPortalChoice();
	}

	if (ScreenshotAtRealTime > 0.0 && FPlatformTime::Seconds() >= ScreenshotAtRealTime)
	{
		ScreenshotAtRealTime = 0.0;
		FScreenshotRequest::RequestScreenshot(TEXT("HPWH_Prototype"), true, false);
	}

	if (bDebugSfx && FPlatformTime::Seconds() >= NextDebugSfxTime && SfxById.Num() > 0)
	{
		const int32 Index = DebugSfxIndex % SfxById.Num();
		PlaySfx2D(static_cast<EHPWH_Sfx>(Index), 0.8f);
		UE_LOG(LogTemp, Display, TEXT("HPWH: debug sfx %d"), Index);
		DebugSfxIndex = (Index + 1) % SfxById.Num();
		NextDebugSfxTime = FPlatformTime::Seconds() + 1.5;
	}
}

void AHPWH_GameMode::LoadMusic()
{
	auto LoadSong = [](const TCHAR* Path) -> USoundBase*
	{
		USoundBase* Sound = LoadObject<USoundBase>(nullptr, Path);
		if (USoundWave* Wave = Cast<USoundWave>(Sound))
		{
			Wave->bLooping = true;
		}
		return Sound;
	};

	BgmCalm = LoadSong(TEXT("/Game/HPWH/Audio/S_HPWH_Emergency1.S_HPWH_Emergency1"));
	BgmAlert = LoadSong(TEXT("/Game/HPWH/Audio/S_HPWH_Emergency2.S_HPWH_Emergency2"));
	BgmCrawler = LoadSong(TEXT("/Game/HPWH/Audio/S_HPWH_Emergency3.S_HPWH_Emergency3"));
	UE_LOG(LogTemp, Display, TEXT("HPWH: music loaded calm=%d alert=%d crawler=%d"),
		BgmCalm ? 1 : 0, BgmAlert ? 1 : 0, BgmCrawler ? 1 : 0);
}

void AHPWH_GameMode::LoadSfx()
{
	// 顺序与 EHPWH_Sfx 一致；资源缺失时对应音效静默跳过。
	static const TCHAR* Paths[] = {
		TEXT("/Game/HPWH/Audio/SFX/S_HPWH_Attack.S_HPWH_Attack"),
		TEXT("/Game/HPWH/Audio/SFX/S_HPWH_SoulOut.S_HPWH_SoulOut"),
		TEXT("/Game/HPWH/Audio/SFX/S_HPWH_EnemySpawn.S_HPWH_EnemySpawn"),
		TEXT("/Game/HPWH/Audio/SFX/S_HPWH_CrawlerAppear.S_HPWH_CrawlerAppear"),
		TEXT("/Game/HPWH/Audio/SFX/S_HPWH_CardAppear.S_HPWH_CardAppear"),
		TEXT("/Game/HPWH/Audio/SFX/S_HPWH_PuzzleSolve.S_HPWH_PuzzleSolve"),
		TEXT("/Game/HPWH/Audio/SFX/S_HPWH_Footstep.S_HPWH_Footstep"),
		TEXT("/Game/HPWH/Audio/SFX/S_HPWH_Win.S_HPWH_Win"),
		TEXT("/Game/HPWH/Audio/SFX/S_HPWH_TileLaptop.S_HPWH_TileLaptop"),
		TEXT("/Game/HPWH/Audio/SFX/S_HPWH_TileMirror.S_HPWH_TileMirror"),
		TEXT("/Game/HPWH/Audio/SFX/S_HPWH_TileCourtyard.S_HPWH_TileCourtyard")
	};

	SfxById.Reset();
	SfxById.Reserve(UE_ARRAY_COUNT(Paths));
	int32 Loaded = 0;
	for (const TCHAR* Path : Paths)
	{
		USoundBase* Sound = LoadObject<USoundBase>(nullptr, Path);
		SfxById.Add(Sound);
		Loaded += Sound ? 1 : 0;
	}
	UE_LOG(LogTemp, Display, TEXT("HPWH: sfx loaded %d/%d"), Loaded, UE_ARRAY_COUNT(Paths));
}

bool AHPWH_GameMode::HasSfx(EHPWH_Sfx Id) const
{
	const int32 Index = static_cast<int32>(Id);
	return SfxById.IsValidIndex(Index) && SfxById[Index] != nullptr;
}

void AHPWH_GameMode::PlaySfx2D(EHPWH_Sfx Id, float Volume, float Pitch)
{
	const int32 Index = static_cast<int32>(Id);
	if (!SfxById.IsValidIndex(Index) || !SfxById[Index] || !GetWorld())
	{
		return;
	}
	UGameplayStatics::PlaySound2D(this, SfxById[Index], Volume, Pitch);
}

void AHPWH_GameMode::PlaySfxAt(EHPWH_Sfx Id, const FVector& Location, float Volume, float Pitch)
{
	const int32 Index = static_cast<int32>(Id);
	if (!SfxById.IsValidIndex(Index) || !SfxById[Index] || !GetWorld())
	{
		return;
	}
	UGameplayStatics::PlaySoundAtLocation(this, SfxById[Index], Location, FRotator::ZeroRotator, Volume, Pitch);
}

void AHPWH_GameMode::PlayMusic(USoundBase* Sound)
{
	if (!Sound || Sound == CurrentMusic)
	{
		return;
	}
	if (MusicComponent)
	{
		MusicComponent->Stop();
		MusicComponent = nullptr;
	}
	MusicComponent = UGameplayStatics::SpawnSound2D(this, Sound, 0.55f);
	CurrentMusic = Sound;
	UE_LOG(LogTemp, Display, TEXT("HPWH: music -> %s"), *Sound->GetName());
}

void AHPWH_GameMode::RefreshMusicState()
{
	bool bCrawlerVisible = false;
	if (AHPWH_Crawler* Crawler = GetCrawler())
	{
		bCrawlerVisible = Crawler->GetCrawlerState() != EHPWH_CrawlerState::Dormant && !Crawler->IsHidden();
	}

	USoundBase* Desired = bCrawlerVisible
		? BgmCrawler
		: (bAlertMusic ? BgmAlert : BgmCalm);
	PlayMusic(Desired);
}

void AHPWH_GameMode::NotifyPlayerAttacked()
{
	if (bMainMenuActive || bAlertMusic)
	{
		return;
	}
	bAlertMusic = true;
	if (AHPWH_Crawler* Crawler = GetCrawler())
	{
		if (Crawler->GetCrawlerState() == EHPWH_CrawlerState::Dormant)
		{
			Crawler->Activate(false);
		}
	}
	PushMessage(TEXT("我来找你了"), 4.f, true);
	RefreshMusicState();
	UE_LOG(LogTemp, Display, TEXT("HPWH: player attacked, alert music + crawler"));
}

void AHPWH_GameMode::OnSoulReturnedToBody()
{
	bAlertMusic = false;
	RefreshMusicState();
	UE_LOG(LogTemp, Display, TEXT("HPWH: soul returned, music reset"));
}

void AHPWH_GameMode::HandleMenuSelection(int32 Index)
{
	switch (MenuPage)
	{
	case EHPWH_MenuPage::Main:
	{
		const bool bHasSave = HasSaveGame();
		if (bHasSave)
		{
			if (Index == 0)
			{
				if (LoadProgress())
				{
					StartGameFromMenu();
				}
			}
			else if (Index == 1)
			{
				StartGameFromMenu();
			}
			else if (Index == 2)
			{
				MenuPage = EHPWH_MenuPage::About;
			}
			else if (Index == 3)
			{
				MenuPage = EHPWH_MenuPage::Settings;
			}
		}
		else
		{
			if (Index == 0)
			{
				StartGameFromMenu();
			}
			else if (Index == 1)
			{
				MenuPage = EHPWH_MenuPage::About;
			}
			else if (Index == 2)
			{
				MenuPage = EHPWH_MenuPage::Settings;
			}
		}
		break;
	}
	case EHPWH_MenuPage::About:
		MenuPage = EHPWH_MenuPage::Main;
		break;
	case EHPWH_MenuPage::Settings:
		if (Index == 0)
		{
			SetGraphicsMode(EHPWH_GraphicsMode::DLSS);
		}
		else if (Index == 1)
		{
			SetGraphicsMode(EHPWH_GraphicsMode::TSR);
		}
		else if (Index == 2)
		{
			MenuPage = EHPWH_MenuPage::Main;
		}
		break;
	}
}

bool AHPWH_GameMode::HasSaveGame() const
{
	return UGameplayStatics::DoesSaveGameExist(TEXT("HPWH_Run"), 0);
}

bool AHPWH_GameMode::LoadProgress()
{
	if (!HasSaveGame())
	{
		UE_LOG(LogTemp, Warning, TEXT("HPWH: load failed, no save"));
		return false;
	}
	UHPWH_SaveGame* Save = Cast<UHPWH_SaveGame>(
		UGameplayStatics::LoadGameFromSlot(TEXT("HPWH_Run"), 0));
	if (!Save)
	{
		UE_LOG(LogTemp, Warning, TEXT("HPWH: load failed, slot unreadable"));
		return false;
	}

	RunSeed = Save->RunSeed;
	LayerIndex = Save->LayerIndex;
	PlayerLevel = Save->PlayerLevel;
	Experience = Save->Experience;
	ExperienceToNextLevel = Save->ExperienceToNextLevel;
	KillCount = Save->KillCount;
	DeathCount = Save->DeathCount;
	ChancePenalty = Save->ChancePenalty;
	CrawlerSpeedLevel = Save->CrawlerSpeedLevel;
	bCoreCollected = Save->bCoreCollected;
	PendingUpgradeChoices = 0;
	PendingSoulHits = 0;
	FloorState = bCoreCollected ? EHPWH_FloorState::CoreCollected : EHPWH_FloorState::Exploring;

	RunStats = FHPWH_RunStats();
	RunStats.AttackBonus = Save->AttackBonus;
	RunStats.MaxHealthBonus = Save->MaxHealthBonus;
	RunStats.MoveSpeedBonus = Save->MoveSpeedBonus;
	RunStats.AttackRangeBonus = Save->AttackRangeBonus;
	RunStats.BulletCooldownReduction = Save->BulletCooldownReduction;
	RunStats.ProjectileCount = Save->ProjectileCount;
	RunStats.SoulHitsRequired = Save->SoulHitsRequired;
	RunStats.SoulSpeedBonus = Save->SoulSpeedBonus;
	RunStats.ReturnRangeBonus = Save->ReturnRangeBonus;
	RunStats.InvulnBonus = Save->InvulnBonus;
	RunStats.KillHeal = Save->KillHeal;
	RunStats.CrawlerDistanceCells = Save->CrawlerDistanceCells;
	RunStats.CrawlerSlow = Save->CrawlerSlow;
	RunStats.PuzzleTimeBonus = Save->PuzzleTimeBonus;
	RunStats.ChanceBonus = Save->ChanceBonus;
	RunStats.InteractRangeBonus = Save->InteractRangeBonus;

	OwnedUpgrades.Reset();
	const int32 OwnedCount = FMath::Min3(Save->OwnedUpgradeTypes.Num(),
		Save->OwnedUpgradeRarities.Num(), Save->OwnedUpgradeCounts.Num());
	for (int32 Index = 0; Index < OwnedCount; ++Index)
	{
		FHPWH_OwnedUpgrade& Entry = OwnedUpgrades.AddDefaulted_GetRef();
		Entry.Type = static_cast<EHPWH_UpgradeType>(Save->OwnedUpgradeTypes[Index]);
		Entry.Rarity = static_cast<EHPWH_Rarity>(Save->OwnedUpgradeRarities[Index]);
		Entry.Count = Save->OwnedUpgradeCounts[Index];
	}
	ConsolidateOwnedUpgrades();

	SolvedPuzzleTypes.Reset();
	for (const uint8 Type : Save->SolvedPuzzleTypes)
	{
		SolvedPuzzleTypes.Add(static_cast<EHPWH_TerrainType>(Type));
	}

	AHPWH_PlayerCharacter* Body = GetPlayerBody();
	if (AHPWH_WorldDirector* Director = GetDirector())
	{
		Director->InitializeForRun(RunSeed, LayerIndex);
	}
	if (Body)
	{
		Body->ForceReset();
		Body->ResetMaxHealthForLoad();
		Body->SetActorLocation(Save->PlayerLocation);
		Body->AddMaxHealth(Save->MaxHealthBonus);
		Body->SetHealthForLoad(Save->PlayerHealth);
		Body->RefreshMoveSpeed();
	}
	if (AHPWH_WorldDirector* Director = GetDirector())
	{
		Director->RestoreProgress(Save->Records, Save->PuzzleAnchorCells);
	}
	if (SolvedPuzzleTypes.Num() > 0)
	{
		if (AHPWH_Crawler* Crawler = GetCrawler())
		{
			Crawler->Activate(false);
		}
	}
	RecomputeChance();
	SetGameplayPaused(false);
	UE_LOG(LogTemp, Display, TEXT("HPWH: progress loaded layer=%d level=%d xp=%d records=%d anchors=%d"),
		LayerIndex, PlayerLevel, Experience, Save->Records.Num(), Save->PuzzleAnchorCells.Num());
	return true;
}

void AHPWH_GameMode::MenuBack()
{
	if (MenuPage != EHPWH_MenuPage::Main)
	{
		MenuPage = EHPWH_MenuPage::Main;
	}
}

void AHPWH_GameMode::StartGameFromMenu()
{
	if (!bMainMenuActive)
	{
		return;
	}
	bMainMenuActive = false;
	MenuPage = EHPWH_MenuPage::Main;
	if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
	{
		Body->SetRunLocked(false);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetWorldSettings()->SetTimeDilation(1.f);
	}
	bAlertMusic = false;
	PlayMusic(BgmCalm);
	ApplyGraphicsMode();
	UE_LOG(LogTemp, Display, TEXT("HPWH: game started from menu"));
}

void AHPWH_GameMode::SetGraphicsMode(EHPWH_GraphicsMode Mode)
{
	GraphicsMode = Mode;
	ApplyGraphicsMode();
}

void AHPWH_GameMode::ApplyGraphicsMode()
{
	UWorld* World = GetWorld();
	if (!GEngine || !World)
	{
		return;
	}
	if (GraphicsMode == EHPWH_GraphicsMode::DLSS)
	{
		GEngine->Exec(World, TEXT("r.NGX.DLSS.Enable 1"));
		GEngine->Exec(World, TEXT("r.AntiAliasingMethod 0"));
	}
	else
	{
		GEngine->Exec(World, TEXT("r.NGX.DLSS.Enable 0"));
		GEngine->Exec(World, TEXT("r.AntiAliasingMethod 4"));
	}
	UE_LOG(LogTemp, Display, TEXT("HPWH: graphics mode -> %s"),
		GraphicsMode == EHPWH_GraphicsMode::DLSS ? TEXT("DLSS") : TEXT("TSR"));
}

void AHPWH_GameMode::SetGameplayPaused(bool bPaused)
{
	if (UWorld* World = GetWorld())
	{
		World->GetWorldSettings()->SetTimeDilation(bPaused ? 0.001f : 1.f);
	}
	if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
	{
		Body->SetRunLocked(bPaused);
	}
}

void AHPWH_GameMode::ToggleInGameMenu()
{
	if (bMainMenuActive || bUpgradeSelectionActive)
	{
		return;
	}
	if (bInGameMenuOpen)
	{
		CloseInGameMenu();
		return;
	}
	bInGameMenuOpen = true;
	bInGameSettingsOpen = false;
	SetGameplayPaused(true);
}

void AHPWH_GameMode::CloseInGameMenu()
{
	if (!bInGameMenuOpen)
	{
		return;
	}
	bInGameMenuOpen = false;
	bInGameSettingsOpen = false;
	SetGameplayPaused(false);
}

void AHPWH_GameMode::HandleInGameMenuSelection(int32 Index)
{
	if (!bInGameMenuOpen)
	{
		return;
	}
	if (bInGameSettingsOpen)
	{
		if (Index == 0)
		{
			SetGraphicsMode(EHPWH_GraphicsMode::DLSS);
		}
		else if (Index == 1)
		{
			SetGraphicsMode(EHPWH_GraphicsMode::TSR);
		}
		else if (Index == 2)
		{
			bInGameSettingsOpen = false;
		}
		return;
	}

	if (Index == 0)
	{
		ReturnToMainMenu();
	}
	else if (Index == 1)
	{
		bInGameSettingsOpen = true;
	}
	else if (Index == 2)
	{
		SaveProgress();
	}
}

void AHPWH_GameMode::ReturnToMainMenu()
{
	bInGameMenuOpen = false;
	bInGameSettingsOpen = false;
	bMainMenuActive = true;
	MenuPage = EHPWH_MenuPage::Main;
	bAlertMusic = false;
	PlayMusic(BgmCalm);
	CancelUpgradeSelection();
	SetGameplayPaused(true);
	UE_LOG(LogTemp, Display, TEXT("HPWH: returned to main menu"));
}

void AHPWH_GameMode::SaveProgress()
{
	UHPWH_SaveGame* Save = Cast<UHPWH_SaveGame>(
		UGameplayStatics::CreateSaveGameObject(UHPWH_SaveGame::StaticClass()));
	if (!Save)
	{
		return;
	}

	Save->RunSeed = RunSeed;
	Save->LayerIndex = LayerIndex;
	Save->PlayerLevel = PlayerLevel;
	Save->Experience = Experience;
	Save->ExperienceToNextLevel = ExperienceToNextLevel;
	Save->KillCount = KillCount;
	Save->DeathCount = DeathCount;
	Save->ChancePenalty = ChancePenalty;
	Save->CrawlerSpeedLevel = CrawlerSpeedLevel;
	Save->bCoreCollected = bCoreCollected;

	if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
	{
		Save->PlayerLocation = Body->GetActorLocation();
		Save->PlayerHealth = Body->GetHealth();
	}
	if (AHPWH_WorldDirector* Director = GetDirector())
	{
		Save->Records = Director->GetRecords();
		Save->PuzzleAnchorCells = Director->GetPuzzleAnchors().Array();
	}

	for (const FHPWH_OwnedUpgrade& Owned : OwnedUpgrades)
	{
		Save->OwnedUpgradeTypes.Add(static_cast<uint8>(Owned.Type));
		Save->OwnedUpgradeRarities.Add(static_cast<uint8>(Owned.Rarity));
		Save->OwnedUpgradeCounts.Add(Owned.Count);
	}
	for (const EHPWH_TerrainType Type : SolvedPuzzleTypes)
	{
		Save->SolvedPuzzleTypes.Add(static_cast<uint8>(Type));
	}

	Save->AttackBonus = RunStats.AttackBonus;
	Save->MaxHealthBonus = RunStats.MaxHealthBonus;
	Save->MoveSpeedBonus = RunStats.MoveSpeedBonus;
	Save->AttackRangeBonus = RunStats.AttackRangeBonus;
	Save->BulletCooldownReduction = RunStats.BulletCooldownReduction;
	Save->ProjectileCount = RunStats.ProjectileCount;
	Save->SoulHitsRequired = RunStats.SoulHitsRequired;
	Save->SoulSpeedBonus = RunStats.SoulSpeedBonus;
	Save->ReturnRangeBonus = RunStats.ReturnRangeBonus;
	Save->InvulnBonus = RunStats.InvulnBonus;
	Save->KillHeal = RunStats.KillHeal;
	Save->CrawlerDistanceCells = RunStats.CrawlerDistanceCells;
	Save->CrawlerSlow = RunStats.CrawlerSlow;
	Save->PuzzleTimeBonus = RunStats.PuzzleTimeBonus;
	Save->ChanceBonus = RunStats.ChanceBonus;
	Save->InteractRangeBonus = RunStats.InteractRangeBonus;

	const bool bSaved = UGameplayStatics::SaveGameToSlot(Save, TEXT("HPWH_Run"), 0);
	PushMessage(bSaved ? TEXT("已存档") : TEXT("存档失败"), 2.f);
	UE_LOG(LogTemp, Display, TEXT("HPWH: save progress result=%d layer=%d level=%d xp=%d"),
		bSaved ? 1 : 0, LayerIndex, PlayerLevel, Experience);
}

void AHPWH_GameMode::OnEnemyKilled()
{
	if (FloorState == EHPWH_FloorState::RunAwake)
	{
		return;
	}
	++KillCount;
	if (RunStats.KillHeal > 0.f)
	{
		if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
		{
			Body->HealHPWH(RunStats.KillHeal);
		}
	}
}

void AHPWH_GameMode::AddExperience(int32 Amount)
{
	if (Amount <= 0 || FloorState == EHPWH_FloorState::RunAwake)
	{
		return;
	}
	Experience += Amount;
	while (Experience >= ExperienceToNextLevel)
	{
		Experience -= ExperienceToNextLevel;
		++PlayerLevel;
		ExperienceToNextLevel = HPWH_ExperienceRequiredForLevel(PlayerLevel);
		RequestUpgradeChoice();
	}
	UE_LOG(LogTemp, Display, TEXT("HPWH: xp +%d total=%d/%d level=%d"),
		Amount, Experience, ExperienceToNextLevel, PlayerLevel);
}

void AHPWH_GameMode::RequestUpgradeChoice()
{
	++PendingUpgradeChoices;
	if (!bUpgradeSelectionActive)
	{
		BeginUpgradeSelection();
	}
}

void AHPWH_GameMode::GenerateUpgradeOptions()
{
	UpgradeOptions.Reset();
	PlaySfx2D(EHPWH_Sfx::CardAppear, 0.65f);

	static const EHPWH_UpgradeType AllTypes[] = {
		EHPWH_UpgradeType::AttackDamage,
		EHPWH_UpgradeType::MaxHealth,
		EHPWH_UpgradeType::MoveSpeed,
		EHPWH_UpgradeType::AttackRange,
		EHPWH_UpgradeType::BulletCooldown,
		EHPWH_UpgradeType::MultiShotTwo,
		EHPWH_UpgradeType::MultiShotThree,
		EHPWH_UpgradeType::SoulGuard,
		EHPWH_UpgradeType::SoulSpeed,
		EHPWH_UpgradeType::ReturnRange,
		EHPWH_UpgradeType::DamageTolerance,
		EHPWH_UpgradeType::KillHeal,
		EHPWH_UpgradeType::CrawlerDistance,
		EHPWH_UpgradeType::CrawlerSlow,
		EHPWH_UpgradeType::PuzzleTime,
		EHPWH_UpgradeType::SuccessRate,
		EHPWH_UpgradeType::InteractRange
	};

	TArray<EHPWH_UpgradeType> UsedTypes;
	auto IsTypeUsable = [this](EHPWH_UpgradeType Type)
	{
		// 多发卡只在尚未达到对应弹数时出现，避免重复占卡位。
		if (Type == EHPWH_UpgradeType::MultiShotTwo)
		{
			return RunStats.ProjectileCount < 2;
		}
		if (Type == EHPWH_UpgradeType::MultiShotThree)
		{
			return RunStats.ProjectileCount < 3;
		}
		return true;
	};
	for (int32 CardIndex = 0; CardIndex < 3; ++CardIndex)
	{
		// 10% 极品、30% 中级、60% 普通。
		const float RarityRoll = UpgradeStream.FRand();
		const EHPWH_Rarity Rarity = RarityRoll < 0.10f
			? EHPWH_Rarity::Legendary
			: (RarityRoll < 0.40f ? EHPWH_Rarity::Epic : EHPWH_Rarity::Common);

		TArray<EHPWH_UpgradeType> Candidates;
		for (const EHPWH_UpgradeType Type : AllTypes)
		{
			if (!UsedTypes.Contains(Type) && HPWH_IsUpgradeAllowed(Type, Rarity) && IsTypeUsable(Type))
			{
				Candidates.Add(Type);
			}
		}
		if (Candidates.Num() == 0)
		{
			for (const EHPWH_UpgradeType Type : AllTypes)
			{
				if (!UsedTypes.Contains(Type) && HPWH_IsUpgradeAllowed(Type, Rarity) && IsTypeUsable(Type))
				{
					Candidates.Add(Type);
				}
			}
		}
		if (Candidates.Num() == 0)
		{
			break;
		}
		const EHPWH_UpgradeType Picked = Candidates[UpgradeStream.RandRange(0, Candidates.Num() - 1)];
		UsedTypes.Add(Picked);
		UpgradeOptions.Add(HPWH_MakeUpgradeOption(Picked, Rarity));

		const FHPWH_UpgradeOption& Option = UpgradeOptions.Last();
		UE_LOG(LogTemp, Display, TEXT("HPWH: upgrade card %d rarity=%d type=%d value=%.2f"),
			CardIndex, static_cast<int32>(Option.Rarity), static_cast<int32>(Option.Type), Option.Value);
	}
}

void AHPWH_GameMode::BeginUpgradeSelection()
{
	if (bUpgradeSelectionActive)
	{
		return;
	}
	bUpgradeSelectionActive = true;
	bUpgradeRerollUsed = false;
	GenerateUpgradeOptions();

	if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
	{
		Body->SetRunLocked(true);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetWorldSettings()->SetTimeDilation(0.01f);
	}
	UpgradeShownRealTime = FPlatformTime::Seconds();
	bAutoPickDebug = FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugUpgradeAuto"));
	bDebugAutoReroll = FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugReroll"));
	UE_LOG(LogTemp, Display, TEXT("HPWH: upgrade selection opened level=%d xp=%d/%d"),
		PlayerLevel, Experience, ExperienceToNextLevel);
}

void AHPWH_GameMode::RerollUpgrade()
{
	if (!bUpgradeSelectionActive || bUpgradeRerollUsed)
	{
		return;
	}
	bUpgradeRerollUsed = true;
	GenerateUpgradeOptions();
	UE_LOG(LogTemp, Display, TEXT("HPWH: upgrade rerolled level=%d"), PlayerLevel);
}

void AHPWH_GameMode::EndUpgradeSelection()
{
	bUpgradeSelectionActive = false;
	UpgradeOptions.Reset();
	UpgradeShownRealTime = 0.0;
	bAutoPickDebug = false;
	bDebugAutoReroll = false;

	if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
	{
		Body->SetRunLocked(false);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetWorldSettings()->SetTimeDilation(1.f);
	}
}

void AHPWH_GameMode::CancelUpgradeSelection()
{
	PendingUpgradeChoices = 0;
	if (!bUpgradeSelectionActive)
	{
		return;
	}
	EndUpgradeSelection();
}

void AHPWH_GameMode::ChooseUpgrade(int32 Index)
{
	if (!bUpgradeSelectionActive || !UpgradeOptions.IsValidIndex(Index))
	{
		return;
	}
	const FHPWH_UpgradeOption Option = UpgradeOptions[Index];
	ApplyUpgrade(Option);
	PlaySfx2D(EHPWH_Sfx::CardAppear, 0.45f, 1.15f);
	PendingUpgradeChoices = FMath::Max(0, PendingUpgradeChoices - 1);
	EndUpgradeSelection();
	PushMessage(FString::Printf(TEXT("获得强化  %s"), *Option.Title), 3.f);
	UE_LOG(LogTemp, Display, TEXT("HPWH: upgrade chosen %s rarity=%d value=%.2f pending=%d"),
		*Option.Title, static_cast<int32>(Option.Rarity), Option.Value, PendingUpgradeChoices);

	// 解密和升级可能同时累计多次机会，依次弹出。
	if (PendingUpgradeChoices > 0)
	{
		BeginUpgradeSelection();
	}
}

void AHPWH_GameMode::ApplyUpgrade(const FHPWH_UpgradeOption& Option)
{
	AHPWH_PlayerCharacter* Body = GetPlayerBody();
	switch (Option.Type)
	{
	case EHPWH_UpgradeType::AttackDamage:
		RunStats.AttackBonus += Option.Value;
		break;
	case EHPWH_UpgradeType::MaxHealth:
		RunStats.MaxHealthBonus += Option.Value;
		if (Body)
		{
			Body->AddMaxHealth(Option.Value);
		}
		break;
	case EHPWH_UpgradeType::MoveSpeed:
		RunStats.MoveSpeedBonus += Option.Value;
		if (Body)
		{
			Body->RefreshMoveSpeed();
		}
		break;
	case EHPWH_UpgradeType::AttackRange:
		RunStats.AttackRangeBonus += Option.Value;
		break;
	case EHPWH_UpgradeType::BulletCooldown:
		RunStats.BulletCooldownReduction = FMath::Min(1.8f, RunStats.BulletCooldownReduction + Option.Value);
		break;
	case EHPWH_UpgradeType::MultiShotTwo:
		RunStats.ProjectileCount = FMath::Max(RunStats.ProjectileCount, 2);
		break;
	case EHPWH_UpgradeType::MultiShotThree:
		RunStats.ProjectileCount = FMath::Max(RunStats.ProjectileCount, 3);
		break;
	case EHPWH_UpgradeType::SoulGuard:
		RunStats.SoulHitsRequired = FMath::Max(RunStats.SoulHitsRequired, 1 + FMath::RoundToInt(Option.Value));
		break;
	case EHPWH_UpgradeType::SoulSpeed:
		RunStats.SoulSpeedBonus += Option.Value;
		if (SoulAvatar)
		{
			SoulAvatar->RefreshMoveSpeed();
		}
		break;
	case EHPWH_UpgradeType::ReturnRange:
		RunStats.ReturnRangeBonus += Option.Value;
		break;
	case EHPWH_UpgradeType::DamageTolerance:
		RunStats.InvulnBonus += Option.Value;
		break;
	case EHPWH_UpgradeType::KillHeal:
		RunStats.KillHeal += Option.Value;
		break;
	case EHPWH_UpgradeType::CrawlerDistance:
		RunStats.CrawlerDistanceCells += FMath::RoundToInt(Option.Value);
		break;
	case EHPWH_UpgradeType::CrawlerSlow:
		RunStats.CrawlerSlow = FMath::Min(0.85f, RunStats.CrawlerSlow + Option.Value);
		break;
	case EHPWH_UpgradeType::PuzzleTime:
		RunStats.PuzzleTimeBonus += Option.Value;
		break;
	case EHPWH_UpgradeType::SuccessRate:
		RunStats.ChanceBonus += Option.Value;
		RecomputeChance();
		break;
	case EHPWH_UpgradeType::InteractRange:
		RunStats.InteractRangeBonus += Option.Value;
		break;
	default:
		break;
	}

	FHPWH_OwnedUpgrade* Owned = OwnedUpgrades.FindByPredicate(
		[&Option](const FHPWH_OwnedUpgrade& Entry) { return Entry.Type == Option.Type; });
	if (Owned)
	{
		++Owned->Count;
		if (static_cast<uint8>(Option.Rarity) > static_cast<uint8>(Owned->Rarity))
		{
			Owned->Rarity = Option.Rarity;
		}
	}
	else
	{
		FHPWH_OwnedUpgrade& Entry = OwnedUpgrades.AddDefaulted_GetRef();
		Entry.Type = Option.Type;
		Entry.Rarity = Option.Rarity;
		Entry.Count = 1;
	}

	ConsolidateOwnedUpgrades();

	UE_LOG(LogTemp, Display, TEXT("HPWH: stats attack=%.0f hp=%.0f move=%.2f range=%.2f cd=%.2f bullets=%d soul=%.2f chance=%.2f"),
		RunStats.AttackBonus, RunStats.MaxHealthBonus, RunStats.MoveSpeedBonus,
		RunStats.AttackRangeBonus, RunStats.BulletCooldownReduction, RunStats.ProjectileCount,
		RunStats.SoulSpeedBonus, RunStats.ChanceBonus);
}

void AHPWH_GameMode::ConsolidateOwnedUpgrades()
{
	// 旧存档可能按每次选择各存一条，或带有早期版本的类型值；这里统一归并。
	const uint8 LastValidType = static_cast<uint8>(EHPWH_UpgradeType::InteractRange);
	TArray<FHPWH_OwnedUpgrade> Merged;
	Merged.Reserve(OwnedUpgrades.Num());
	for (const FHPWH_OwnedUpgrade& Entry : OwnedUpgrades)
	{
		if (static_cast<uint8>(Entry.Type) > LastValidType)
		{
			continue;
		}
		const int32 Count = FMath::Max(1, Entry.Count);
		FHPWH_OwnedUpgrade* Existing = Merged.FindByPredicate(
			[&Entry](const FHPWH_OwnedUpgrade& Candidate) { return Candidate.Type == Entry.Type; });
		if (Existing)
		{
			Existing->Count += Count;
			if (static_cast<uint8>(Entry.Rarity) > static_cast<uint8>(Existing->Rarity))
			{
				Existing->Rarity = Entry.Rarity;
			}
		}
		else
		{
			FHPWH_OwnedUpgrade& Added = Merged.AddDefaulted_GetRef();
			Added.Type = Entry.Type;
			Added.Rarity = Entry.Rarity;
			Added.Count = Count;
		}
	}
	if (Merged.Num() != OwnedUpgrades.Num())
	{
		UE_LOG(LogTemp, Display, TEXT("HPWH: owned upgrades consolidated %d -> %d"),
			OwnedUpgrades.Num(), Merged.Num());
	}
	OwnedUpgrades = MoveTemp(Merged);
}

void AHPWH_GameMode::RestartPlayer(AController* NewPlayer)
{
	if (!NewPlayer)
	{
		return;
	}
	if (NewPlayer->GetPawn())
	{
		NewPlayer->GetPawn()->TeleportTo(FVector(0.f, 0.f, PlayerSpawnHeight), FRotator::ZeroRotator);
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AHPWH_PlayerCharacter* Character = GetWorld()->SpawnActor<AHPWH_PlayerCharacter>(
		AHPWH_PlayerCharacter::StaticClass(),
		FVector(0.f, 0.f, PlayerSpawnHeight), FRotator::ZeroRotator, Params);
	if (!Character)
	{
		return;
	}
	PlayerBody = Character;
	NewPlayer->Possess(Character);
	UE_LOG(LogTemp, Display, TEXT("HPWH: player spawned at origin"));

	if (AHPWH_WorldDirector* Director = GetDirector())
	{
		Director->InitializeForRun(RunSeed, LayerIndex);
		bDirectorInitialized = true;
	}
}

AHPWH_WorldDirector* AHPWH_GameMode::GetDirector()
{
	if (!WorldDirector && GetWorld())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		WorldDirector = GetWorld()->SpawnActor<AHPWH_WorldDirector>(
			AHPWH_WorldDirector::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	}
	return WorldDirector;
}

AHPWH_PlayerCharacter* AHPWH_GameMode::GetPlayerBody()
{
	if (IsValid(PlayerBody))
	{
		return PlayerBody;
	}
	for (TActorIterator<AHPWH_PlayerCharacter> It(GetWorld()); It; ++It)
	{
		PlayerBody = *It;
		return PlayerBody;
	}
	return nullptr;
}

AHPWH_Crawler* AHPWH_GameMode::GetCrawler()
{
	const AHPWH_WorldDirector* Director = GetDirector();
	return Director ? Director->GetCrawler() : nullptr;
}

AHPWH_Companion* AHPWH_GameMode::GetCompanion()
{
	const AHPWH_WorldDirector* Director = GetDirector();
	return Director ? Director->GetCompanion() : nullptr;
}

void AHPWH_GameMode::NotifyCompanionPlayerHurt()
{
	if (AHPWH_Companion* Companion = GetCompanion())
	{
		Companion->NotifyPlayerHurt();
	}
}

void AHPWH_GameMode::RecomputeChance()
{
	const float Base = FMath::Min(100.f, 5.f * FMath::Pow(2.f, static_cast<float>(FMath::Max(0, LayerIndex - 1))));
	RawChance = Base - DeathCount * 5.f - ChancePenalty + RunStats.ChanceBonus * 100.f;
	RollChance = FMath::Clamp(RawChance, 0.f, 100.f);
	bRunImpossible = RawChance < 0.f;
}

void AHPWH_GameMode::ApplyChancePenalty(float Percent)
{
	ChancePenalty += FMath::Max(0.f, Percent);
	RecomputeChance();
}

void AHPWH_GameMode::PushMessage(const FString& Text, float Duration, bool bPriority)
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (bPriority)
	{
		PriorityMessage = Text;
		PriorityMessageEnd = Now + Duration;
		return;
	}
	NormalMessage = Text;
	NormalMessageEnd = Now + Duration;
}

FString AHPWH_GameMode::GetActiveMessage() const
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (!PriorityMessage.IsEmpty() && Now < PriorityMessageEnd)
	{
		return PriorityMessage;
	}
	if (!NormalMessage.IsEmpty() && Now < NormalMessageEnd)
	{
		return NormalMessage;
	}
	return FString();
}

bool AHPWH_GameMode::IsPriorityMessageActive() const
{
	return GetWorld() && !PriorityMessage.IsEmpty() && GetWorld()->GetTimeSeconds() < PriorityMessageEnd;
}

void AHPWH_GameMode::TriggerSoulOut()
{
	if (SoulState != EHPWH_SoulState::Embodied)
	{
		return;
	}

	// 魂壳：极品强化可让灵魂离体需要多挨一次攻击。
	if (RunStats.SoulHitsRequired > 1)
	{
		++PendingSoulHits;
		if (PendingSoulHits < RunStats.SoulHitsRequired)
		{
			PushMessage(FString::Printf(TEXT("魂壳挡下了这一击  %d/%d"),
				PendingSoulHits, RunStats.SoulHitsRequired), 2.f);
			return;
		}
	}
	PendingSoulHits = 0;

	AHPWH_PlayerCharacter* Body = GetPlayerBody();
	if (!Body)
	{
		return;
	}
	Body->EnterSoulOut();
	SoulState = EHPWH_SoulState::SoulOut;
	MarkState = EHPWH_MarkState::Marked;
	bBodyCaughtPenalized = false;

	if (FParse::Param(FCommandLine::Get(), TEXT("HPWHDebugSoulChaser")) && SoulAvatar)
	{
		// 在灵魂附近放一只近身怪，验证普通怪改为锁定灵魂。
		if (AHPWH_EnemyBase* Enemy = GetWorld()->SpawnActor<AHPWH_EnemyBase>(
			AHPWH_MeleeMonster::StaticClass(),
			SoulAvatar->GetActorLocation() + FVector(320.f, 0.f, 0.f), FRotator::ZeroRotator))
		{
			Enemy->InitEnemy(GetDirector(), FIntPoint::ZeroValue, false);
			UE_LOG(LogTemp, Display, TEXT("HPWH: debug soul chaser spawned at (%.1f,%.1f)"),
				Enemy->GetActorLocation().X / 100.f, Enemy->GetActorLocation().Y / 100.f);
		}
	}

	// 徘徊者只在第一个谜题完成后激活，灵魂离体本身不再提前唤醒。
	PushMessage(TEXT("灵魂离体  找回肉身"), 1.8f, true);
}

void AHPWH_GameMode::HandleCrawlerCaughtBody()
{
	if (SoulState != EHPWH_SoulState::SoulOut || bBodyCaughtPenalized)
	{
		return;
	}
	bBodyCaughtPenalized = true;
	ApplyChancePenalty(5.f);
	PlaySfx2D(EHPWH_Sfx::CrawlerAppear, 0.45f, 0.9f);
	UE_LOG(LogTemp, Display, TEXT("HPWH: crawler caught body, chance penalty=%.0f raw=%.0f"),
		ChancePenalty, RawChance);
	PushMessage(TEXT("身体被触碰  通关率 -5%"), 4.f, true);
}

void AHPWH_GameMode::HandleSoulKilled()
{
	if (SoulState != EHPWH_SoulState::SoulOut)
	{
		return;
	}
	PushMessage(TEXT("灵魂被怪物吞掉了"), 2.f, true);
	HandlePlayerDeath();
	UE_LOG(LogTemp, Display, TEXT("HPWH: soul killed, deaths=%d raw=%.0f"), DeathCount, RawChance);
}

void AHPWH_GameMode::CollectCore()
{
	if (bCoreCollected)
	{
		return;
	}
	bCoreCollected = true;
	FloorState = EHPWH_FloorState::CoreCollected;
	PushMessage(TEXT("带着灯芯去出口"), 4.f);
}

void AHPWH_GameMode::OnPuzzleSolved(EHPWH_TerrainType Type)
{
	FloorState = EHPWH_FloorState::Exploring;
	SolvedPuzzleTypes.Add(Type);
	PlaySfx2D(EHPWH_Sfx::PuzzleSolve, 0.85f);

	// 第一个谜题完成后才唤醒徘徊者。
	bool bCrawlerWoke = false;
	if (AHPWH_Crawler* Crawler = GetCrawler())
	{
		if (Crawler->GetCrawlerState() == EHPWH_CrawlerState::Dormant)
		{
			Crawler->Activate(false);
			bCrawlerWoke = true;
			UE_LOG(LogTemp, Display, TEXT("HPWH: first puzzle solved, crawler activated"));
		}
	}

	const int32 SolvedCount = SolvedPuzzleTypes.Num();
	if (SolvedCount >= 3 && !bFloorPortalSpawned)
	{
		SpawnFloorPortal();
		PushMessage(TEXT("三种解密完成  传送门出现了"), 4.f, true);
	}
	else if (bCrawlerWoke)
	{
		PushMessage(FString::Printf(TEXT("解密完成  还差 %d 种  它醒了"), FMath::Max(0, 3 - SolvedCount)), 4.f, true);
	}
	else
	{
		PushMessage(FString::Printf(TEXT("解密完成  还差 %d 种"), FMath::Max(0, 3 - SolvedCount)), 3.f);
	}

	// 每次解密成功获得一次强化三选一机会。
	RequestUpgradeChoice();
}

void AHPWH_GameMode::DestroyFloorPortal()
{
	if (IsValid(FloorPortal))
	{
		FloorPortal->Destroy();
	}
	FloorPortal = nullptr;
}

void AHPWH_GameMode::SpawnFloorPortal()
{
	if (bFloorPortalSpawned || !GetWorld())
	{
		return;
	}
	AHPWH_PlayerCharacter* Body = GetPlayerBody();
	AHPWH_WorldDirector* Director = GetDirector();
	if (!Body || !Director)
	{
		return;
	}
	DestroyFloorPortal();

	const FIntPoint Center = Director->WorldToCell(Body->GetActorLocation());
	FVector SpawnLocation = Director->CellCenter(Center) + FVector(500.f, 0.f, 0.f);
	bool bFoundOpenSpot = false;
	for (int32 Attempt = 0; Attempt < 24; ++Attempt)
	{
		const int32 DX = FMath::RandRange(-2, 2);
		const int32 DY = FMath::RandRange(-2, 2);
		if (FMath::Max(FMath::Abs(DX), FMath::Abs(DY)) < 1)
		{
			continue;
		}
		const FVector CenterWorld = Director->CellCenter(Center + FIntPoint(DX, DY));
		const FVector Candidate = HPWH_SnapDecimeter(CenterWorld + FVector(
			FMath::FRandRange(-250.f, 250.f), FMath::FRandRange(-250.f, 250.f), 0.f));
		FCollisionQueryParams Params(SCENE_QUERY_STAT(HPWHPortalSpot), false, Body);
		if (GetWorld()->OverlapBlockingTestByChannel(Candidate + FVector(0.f, 0.f, 60.f),
			FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(70.f), Params))
		{
			continue;
		}
		SpawnLocation = Candidate;
		bFoundOpenSpot = true;
		break;
	}
	if (!bFoundOpenSpot)
	{
		UE_LOG(LogTemp, Warning, TEXT("HPWH: portal fallback spawned without open-spot check"));
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FloorPortal = GetWorld()->SpawnActor<AHPWH_NextFloorPortal>(
		AHPWH_NextFloorPortal::StaticClass(), SpawnLocation, FRotator::ZeroRotator, Params);
	if (FloorPortal)
	{
		FloorPortal->InitializePortal(Director);
		bFloorPortalSpawned = true;
		PlaySfx2D(EHPWH_Sfx::CardAppear, 0.6f, 1.05f);
		UE_LOG(LogTemp, Display, TEXT("HPWH: next floor portal spawned at (%.1f, %.1f)m"),
			SpawnLocation.X / 100.f, SpawnLocation.Y / 100.f);
	}
}

void AHPWH_GameMode::OpenFloorPortalChoice()
{
	if (bPortalChoiceActive || bMainMenuActive || bUpgradeSelectionActive || !bFloorPortalSpawned)
	{
		return;
	}
	bPortalChoiceActive = true;
	bPortalChoiceSuccess = (EscapeStream.FRand() * 100.f) < RollChance;
	PortalChoiceRealTime = FPlatformTime::Seconds();
	PlaySfx2D(EHPWH_Sfx::CardAppear, 0.8f);
	if (bPortalChoiceSuccess)
	{
		PlaySfx2D(EHPWH_Sfx::Win, 0.9f);
	}
	SetGameplayPaused(true);
	UE_LOG(LogTemp, Display, TEXT("HPWH: portal choice roll chance=%.0f success=%d"),
		RollChance, bPortalChoiceSuccess ? 1 : 0);
}

void AHPWH_GameMode::ConfirmFloorPortalChoice()
{
	if (!bPortalChoiceActive)
	{
		return;
	}
	const bool bWin = bPortalChoiceSuccess;
	bPortalChoiceActive = false;
	SetGameplayPaused(false);

	if (bWin)
	{
		FloorState = EHPWH_FloorState::RunAwake;
		PushMessage(TEXT("你醒了。"), 8.f, true);
		UE_LOG(LogTemp, Display, TEXT("HPWH: portal clear, run awake"));
		FString MapName = GetWorld() ? GetWorld()->GetMapName() : TEXT("L_HPWH_Prototype");
		if (GetWorld())
		{
			MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
		}
		UGameplayStatics::OpenLevel(this, FName(*MapName));
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("HPWH: portal failed, advancing layer"));
	AdvanceLayer();
}

void AHPWH_GameMode::OnPuzzleFailed()
{
	++PuzzleFailCount;
	if (AHPWH_Crawler* Crawler = GetCrawler())
	{
		if (Crawler->GetCrawlerState() != EHPWH_CrawlerState::Dormant)
		{
			++CrawlerSpeedLevel;
			PushMessage(TEXT("谜题失败  它更快了"), 3.f);
			return;
		}
	}
	PushMessage(TEXT("谜题失败  重新来过"), 3.f);
}

void AHPWH_GameMode::HandlePlayerDeath()
{
	if (FloorState == EHPWH_FloorState::RunAwake)
	{
		return;
	}

	CancelUpgradeSelection();
	bPortalChoiceActive = false;
	SolvedPuzzleTypes.Reset();
	bFloorPortalSpawned = false;
	DestroyFloorPortal();
	SetGameplayPaused(false);
	PendingSoulHits = 0;
	++DeathCount;
	RecomputeChance();

	if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
	{
		Body->ForceReturnToBody();
		Body->ForceReset();
	}
	SoulState = EHPWH_SoulState::Embodied;
	MarkState = EHPWH_MarkState::Unmarked;
	bCoreCollected = false;
	FloorState = EHPWH_FloorState::Exploring;

	if (AHPWH_WorldDirector* Director = GetDirector())
	{
		Director->ResetForRespawn();
	}
	bAlertMusic = false;
	RefreshMusicState();
	PlaySfx2D(EHPWH_Sfx::CrawlerAppear, 0.7f, 0.8f);
	PushMessage(TEXT("你死了  概率 -5%"), 4.f, true);
}

void AHPWH_GameMode::TryExit(APawn* Interactor)
{
	if (FloorState == EHPWH_FloorState::RunAwake)
	{
		return;
	}
	if (!bCoreCollected)
	{
		PushMessage(TEXT("出口没有回应"), 2.5f);
		return;
	}
	if (SoulState != EHPWH_SoulState::Embodied)
	{
		PushMessage(TEXT("灵魂无法离开"), 2.5f);
		return;
	}
	if (RawChance < 0.f)
	{
		bRunImpossible = true;
		PushMessage(TEXT("门没有开。"), 3.f);
		return;
	}

	FloorState = EHPWH_FloorState::ExitRolling;
	const float Roll = EscapeStream.FRand() * 100.f;
	if (Roll < RollChance)
	{
		FloorState = EHPWH_FloorState::RunAwake;
		if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
		{
			Body->SetRunLocked(true);
		}
		PushMessage(TEXT("你醒了。"), 8.f, true);
		return;
	}
	AdvanceLayer();
}

void AHPWH_GameMode::AdvanceLayer()
{
	FloorState = EHPWH_FloorState::FloorTransition;
	bPortalChoiceActive = false;
	SolvedPuzzleTypes.Reset();
	bFloorPortalSpawned = false;
	DestroyFloorPortal();
	SetGameplayPaused(false);
	++LayerIndex;
	++CrawlerSpeedLevel;
	RecomputeChance();

	if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
	{
		Body->ForceReset();
	}
	SoulState = EHPWH_SoulState::Embodied;
	MarkState = EHPWH_MarkState::Unmarked;
	bCoreCollected = false;

	if (AHPWH_WorldDirector* Director = GetDirector())
	{
		Director->SetLayer(LayerIndex);
	}
	bAlertMusic = false;
	RefreshMusicState();

	FloorState = EHPWH_FloorState::Exploring;
	PushMessage(FString::Printf(TEXT("第 %d 层"), LayerIndex), 4.f);
}

void AHPWH_GameMode::PlacePlayerAtOrigin()
{
	if (AHPWH_PlayerCharacter* Body = GetPlayerBody())
	{
		Body->TeleportTo(FVector(0.f, 0.f, PlayerSpawnHeight), FRotator::ZeroRotator);
	}
}

void AHPWH_GameMode::EnsureRuntimeLighting()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	bool bHasSun = false;
	bool bHasSky = false;
	bool bHasFog = false;
	bool bHasAtmosphere = false;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->IsA<ADirectionalLight>())
		{
			bHasSun = true;
		}
		else if (It->IsA<ASkyLight>())
		{
			bHasSky = true;
		}
		else if (It->IsA<AExponentialHeightFog>())
		{
			bHasFog = true;
		}
		else if (It->IsA<ASkyAtmosphere>())
		{
			bHasAtmosphere = true;
		}
	}

	if (!bHasAtmosphere)
	{
		World->SpawnActor<ASkyAtmosphere>(ASkyAtmosphere::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	}

	if (!bHasSun)
	{
		if (ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(
			ADirectionalLight::StaticClass(), FVector(0.f, 0.f, 4000.f), FRotator(-55.f, -35.f, 0.f)))
		{
			if (UDirectionalLightComponent* Light = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
			{
				Light->SetMobility(EComponentMobility::Movable);
				Light->SetIntensity(5.f);
				Light->SetLightColor(FLinearColor(0.52f, 0.60f, 0.82f));
			}
		}
	}
	if (!bHasSky)
	{
		if (ASkyLight* Sky = World->SpawnActor<ASkyLight>(
			ASkyLight::StaticClass(), FVector(0.f, 0.f, 800.f), FRotator::ZeroRotator))
		{
			if (USkyLightComponent* Light = Sky->GetLightComponent())
			{
				Light->SetMobility(EComponentMobility::Movable);
				Light->SetIntensity(1.2f);
				Light->SetLightColor(FLinearColor(0.30f, 0.36f, 0.50f));
				Light->SetRealTimeCapture(true);
			}
		}
	}
	if (!bHasFog)
	{
		if (AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>(
			AExponentialHeightFog::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator))
		{
			if (UExponentialHeightFogComponent* FogComponent = Fog->GetComponent())
			{
				FogComponent->SetFogDensity(0.02f);
				FogComponent->SetFogHeightFalloff(0.10f);
			}
		}
	}

	if (APostProcessVolume* PostProcess = World->SpawnActor<APostProcessVolume>())
	{
		PostProcess->bUnbound = true;
		PostProcess->Settings.bOverride_AutoExposureBias = true;
		PostProcess->Settings.AutoExposureBias = -0.3f;
		PostProcess->Settings.bOverride_BloomIntensity = true;
		PostProcess->Settings.BloomIntensity = 0.15f;
		PostProcess->Settings.bOverride_VignetteIntensity = true;
		PostProcess->Settings.VignetteIntensity = 0.45f;
	}
}
