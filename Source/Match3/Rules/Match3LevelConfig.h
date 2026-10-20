// Match3LevelConfig.h
// Every tunable number of a level lives here, so designers change the game
// in a data asset (DA_Level01) instead of in code.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Core/Match3Types.h"
#include "Match3LevelConfig.generated.h"

class APuzzlePieceBase;

UCLASS(BlueprintType)
class MATCH3_API UMatch3LevelConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- Board ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Board", meta = (ClampMin = "3"))
	int32 Rows = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Board", meta = (ClampMin = "3"))
	int32 Cols = 5;

	/** How many EPieceColor values are in play, counted from Red. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Board", meta = (ClampMin = "3", ClampMax = "4"))
	int32 NumColors = 4;

	/** World distance between two neighboring cells, in Unreal units. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Board", meta = (ClampMin = "1.0"))
	float TileSize = 100.f;

	/** The Blueprint piece class the board spawns (BP_PuzzlePiece). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Board")
	TSubclassOf<APuzzlePieceBase> PieceClass;

	// --- Rules ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules", meta = (ClampMin = "1"))
	int32 MaxMoves = 20;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules", meta = (ClampMin = "1"))
	int32 TargetScore = 1500;

	/** Base in Score = Base x Count x (Count - 2) x Combo, and Blast = Base x CellCount. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules", meta = (ClampMin = "1"))
	int32 BasePoints = 10;

	/** Bug guard only: stops a broken cascade from freezing the editor. Not a gameplay limit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules", meta = (ClampMin = "1"))
	int32 MaxCascadeSafety = 50;

	// --- Power-ups ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PowerUps", meta = (ClampMin = "1"))
	int32 InventoryCapacity = 3;

	/** Chance (0 to 1) that a newly spawned piece carries a power-up. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PowerUps", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PowerUpSpawnChance = 0.08f;

	/** One entry per power-up type: effect class, icon and name. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PowerUps")
	TArray<FPowerUpEntry> PowerUps;

	// --- Timing ---
	// C++ timers wait exactly these durations. Blueprint timelines play at
	// 1 / Duration so the animation always fits the time C++ waits.

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.01"))
	float SwapDuration = 0.20f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.01"))
	float ClearDuration = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.01"))
	float FallDuration = 0.30f;

	// --- Visuals ---

	/** One entry per color: material and tint for BP_PuzzlePiece. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	TArray<FPieceColorVisual> ColorVisuals;

	// --- Lookups ---

	/** The entry for Type. Returns an empty entry (EffectClass is None) when the asset has none. */
	UFUNCTION(BlueprintPure, Category = "PowerUps")
	FPowerUpEntry FindPowerUp(EPowerUpType Type) const;

	/** The visual for Color. Returns a default (white, no material) when the asset has none. */
	UFUNCTION(BlueprintPure, Category = "Visuals")
	FPieceColorVisual FindColorVisual(EPieceColor Color) const;
};