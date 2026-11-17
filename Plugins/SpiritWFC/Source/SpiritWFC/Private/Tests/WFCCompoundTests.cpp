#include "WFCLayoutPlanner.h"
#include "WFCSolver.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWFCCompoundBuildingsTest,"SpiritWFC.Settlement.CompoundBuildings",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWFCCompoundBuildingsTest::RunTest(const FString&)
{
    TStrongObjectPtr<UWFCTileSet> Set(NewObject<UWFCTileSet>());
    Set->TraversableSockets={TEXT("Ground"),TEXT("Road"),TEXT("Stair"),TEXT("CompoundDoor")};
    auto Add=[&](FName Role,FName Down,FName Up,bool Upper=false)
    {
        FWFCTileDefinition T;T.TileId=Role;T.Tags.Add(Role);T.Sockets.Down=Down;T.Sockets.Up=Up;
        T.Sockets.PositiveX=T.Sockets.NegativeX=T.Sockets.PositiveY=T.Sockets.NegativeY=Upper ? TEXT("Closed") : TEXT("Ground");
        T.AllowedYawQuarterTurns={0,1,2,3};T.MinLayer=Upper?1:0;T.MaxLayer=Upper?2:0;
        return T;
    };
    auto Ground=Add(TEXT("Ground"),TEXT("Ground"),TEXT("Sky"));Ground.AllowedYawQuarterTurns={0};Set->Tiles.Add(Ground);
    auto Air=Add(TEXT("Air"),TEXT("Sky"),TEXT("Sky"),true);Air.MinLayer=0;Air.bWalkable=false;Air.bRenderMesh=false;Air.AllowedYawQuarterTurns={0};Set->Tiles.Add(Air);
    for (int32 Mask=0;Mask<16;++Mask) for (int32 Plaza=0;Plaza<2;++Plaza)
    {
        auto T=Ground;T.TileId=FName(*FString::Printf(TEXT("%s_%02d"),Plaza?TEXT("Plaza"):TEXT("Road"),Mask));T.Tags={T.TileId};
        T.Sockets.PositiveX=Mask&1?TEXT("Road"):TEXT("Ground");T.Sockets.NegativeX=Mask&2?TEXT("Road"):TEXT("Ground");
        T.Sockets.PositiveY=Mask&4?TEXT("Road"):TEXT("Ground");T.Sockets.NegativeY=Mask&8?TEXT("Road"):TEXT("Ground");Set->Tiles.Add(T);
    }
    Set->Tiles.Add(Add(TEXT("House"),TEXT("Ground"),TEXT("Sky")));Set->Tiles.Add(Add(TEXT("Base"),TEXT("Ground"),TEXT("Stair")));
    Set->Tiles.Add(Add(TEXT("Middle"),TEXT("Stair"),TEXT("Stair"),true));Set->Tiles.Add(Add(TEXT("Top"),TEXT("Stair"),TEXT("Sky"),true));
    for (const FName Role:{FName(TEXT("CompoundHouse")),FName(TEXT("CompoundBase")),FName(TEXT("CompoundWing"))})
    {
        auto T=Add(Role,TEXT("Ground"),Role==TEXT("CompoundBase")?TEXT("Stair"):TEXT("Sky"));
        if (Role==TEXT("CompoundWing")) T.Sockets.NegativeX=TEXT("CompoundDoor");else T.Sockets.PositiveX=TEXT("CompoundDoor");
        Set->Tiles.Add(T);
    }
    const FIntPoint Right[4]={{1,0},{0,1},{-1,0},{0,-1}};
    const FIntPoint Front[4]={{0,-1},{1,0},{0,1},{-1,0}};
    for (int32 Layers:{1,3}) for (int32 Seed=0;Seed<16;++Seed)
    {
        FWFCGenerationSettings Settings;Settings.Mode=Layers==1?EWFCMapMode::Planar:EWFCMapMode::Volume;
        Settings.Width=Settings.Depth=14;Settings.Layers=Layers;Settings.LayoutPreset=EWFCLayoutPreset::Settlement;
        Settings.BuildingDensity=1;Settings.MaxBuildingStories=2;Settings.CompoundBuildingChance=1;Settings.Seed=Seed;Settings.bConstrainHorizontalBoundary=false;
        TArray<FWFCCellRule> Rules,Repeat;FString Error;
        if (!TestTrue(TEXT("Compound planning succeeds"),FWFCLayoutPlanner::Build(Settings,Rules,Error))) {AddError(Error);continue;}
        FWFCLayoutPlanner::Build(Settings,Repeat,Error);int32 MainCount=0,WingCount=0;
        for (const auto& Rule:Rules)
        {
            const int32 I=Rule.Coordinate.X+14*Rule.Coordinate.Y+196*Rule.Coordinate.Z;
            TestTrue(TEXT("Compound layout is deterministic"),Rule.RequiredTag==Repeat[I].RequiredTag && Rule.QuarterTurns==Repeat[I].QuarterTurns);
            if (Rule.Coordinate.Z==2) TestEqual(TEXT("Story cap preserves air above two floors"),Rule.RequiredTag,FName(TEXT("Air")));
            WingCount+=Rule.RequiredTag==TEXT("CompoundWing");
            if (Rule.RequiredTag!=TEXT("CompoundHouse") && Rule.RequiredTag!=TEXT("CompoundBase")) continue;
            ++MainCount;const auto Direction=Right[Rule.QuarterTurns];
            const int32 Wing=I+Direction.X+14*Direction.Y;
            TestEqual(TEXT("Main has a reserved connecting wing"),Rules[Wing].RequiredTag,FName(TEXT("CompoundWing")));
            TestEqual(TEXT("Connecting doors share orientation"),Rules[Wing].QuarterTurns,Rule.QuarterTurns);
            const auto Entrance=Front[Rule.QuarterTurns];const FString Street=Rules[I+Entrance.X+14*Entrance.Y].RequiredTag.ToString();
            TestTrue(TEXT("Compound main entrance reaches a street"),Street.StartsWith(TEXT("Road_"))||Street.StartsWith(TEXT("Plaza_")));
            if (Layers>1) {TestEqual(TEXT("Main has an upper floor"),Rules[I+196].RequiredTag,FName(TEXT("Top")));TestEqual(TEXT("Wing stays single-story"),Rules[Wing+196].RequiredTag,FName(TEXT("Air")));}
        }
        TestTrue(TEXT("At least one compound generated"),MainCount>0);TestEqual(TEXT("Every wing belongs to exactly one main"),MainCount,WingCount);
        auto Result=FWFCSolver::Solve(Set.Get(),Settings);
        TestTrue(TEXT("Compound ports, roads and floors solve together"),Result.Status==EWFCResultStatus::Success);
        TestEqual(TEXT("Compounds remain physically zoned into one walkable graph"),Result.WalkableComponents,1);
    }
    return true;
}
#endif
