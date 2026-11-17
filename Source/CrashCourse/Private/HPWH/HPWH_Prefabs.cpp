#include "HPWH/HPWH_Prefabs.h"

#include "HPWH/HPWH_GameMode.h"
#include "HPWH/HPWH_World.h"
#include "Kismet/GameplayStatics.h"

AHPWH_CellPrefabBase::AHPWH_CellPrefabBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AHPWH_CellPrefabBase::PlaceInCell(const FIntPoint& InCell, float InCellSizeCm)
{
	CellCoordinate = InCell;
	CellSizeCm = InCellSizeCm;
	OnPlacedInCell();
}

TSubclassOf<AHPWH_CellPrefabBase> UHPWH_TerrainPrefabSet::PickPrefab(EHPWH_TerrainType Terrain, int32 VariantSeed) const
{
	TArray<const FHPWH_TerrainPrefabEntry*> Candidates;
	float TotalWeight = 0.f;
	for (const FHPWH_TerrainPrefabEntry& Entry : Entries)
	{
		if (Entry.Terrain == Terrain && !Entry.Prefab.IsNull())
		{
			Candidates.Add(&Entry);
			TotalWeight += FMath::Max(0.01f, Entry.Weight);
		}
	}
	if (Candidates.IsEmpty() || TotalWeight <= 0.f)
	{
		return nullptr;
	}

	float Roll = HPWHHashUnit(static_cast<uint32>(VariantSeed)) * TotalWeight;
	for (const FHPWH_TerrainPrefabEntry* Entry : Candidates)
	{
		Roll -= FMath::Max(0.01f, Entry->Weight);
		if (Roll <= 0.f)
		{
			return Entry->Prefab.LoadSynchronous();
		}
	}
	return Candidates.Last()->Prefab.LoadSynchronous();
}

float UHPWH_GridLibrary::GetCellSize(const UObject* WorldContextObject)
{
	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(WorldContextObject)))
	{
		if (const auto* Director = GM->GetDirector())
		{
			return Director->GetCellSize();
		}
	}
	return 800.f;
}

FIntPoint UHPWH_GridLibrary::WorldToCell(const UObject* WorldContextObject, FVector WorldLocation)
{
	return HPWH_WorldToCell(WorldLocation, GetCellSize(WorldContextObject));
}

FVector UHPWH_GridLibrary::CellCenter(const UObject* WorldContextObject, const FIntPoint& Cell)
{
	return HPWH_CellCenter(Cell, GetCellSize(WorldContextObject));
}
