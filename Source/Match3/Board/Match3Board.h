// Match3Board.h
// The board owns the grid as data and runs the turn state machine
// (see "8. Board state machine"). It is the only class that moves,
// spawns or destroys pieces. Timers set the rhythm; Blueprints only
// fill that time with animation.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/Match3Types.h"
#include "Match3Board.generated.h"

class APuzzlePieceBase;
class UMatch3LevelConfig;
class UPowerUpEffect;
class AMatch3PlayerState;

UCLASS(Blueprintable)
class MATCH3_API AMatch3Board : public AActor
{
	GENERATED_BODY()

public:
	AMatch3Board();

	// --- Events ---

	UPROPERTY(BlueprintAssignable, Category = "Match3|Events")
	FOnBoardStateChanged OnBoardStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Match3|Events")
	FOnTargetingStarted OnTargetingStarted;

	UPROPERTY(BlueprintAssignable, Category = "Match3|Events")
	FOnTargetingEnded OnTargetingEnded;

	// --- Commands from AMatch3GameMode (plain C++) ---

	/** Stores the config and fills all cells with no starting matches. Ends in Idle. */
	void InitializeBoard(UMatch3LevelConfig* InConfig);

	/** Enters Targeting with Effect. The power-up stays in the inventory until ConfirmTarget. */
	bool BeginTargeting(EPowerUpType Type, UPowerUpEffect* Effect, AMatch3PlayerState* InInstigator);

	/** Stops all timers and enters GameOver, so no more input is accepted. */
	void LockForGameOver();

	// --- Requests from the player controller (Blueprint) ---

	/**
	 * Swaps the piece at From with its neighbor in Dir. Only works in Idle.
	 * Off the board: the piece shakes and no move is spent. Returns true if the swap happened.
	 */
	UFUNCTION(BlueprintCallable, Category = "Match3|Board")
	bool RequestSwap(FGridCoord From, ESwipeDirection Dir);

	/** Highlights the cells the active power-up would hit at Coord. Only works in Targeting. */
	UFUNCTION(BlueprintCallable, Category = "Match3|Board")
	void PreviewTarget(FGridCoord Coord);

	/** Fires the active power-up at Coord. Only works in Targeting. Returns true if it fired. */
	UFUNCTION(BlueprintCallable, Category = "Match3|Board")
	bool ConfirmTarget(FGridCoord Coord);

	/** Leaves Targeting and keeps the power-up in the inventory. */
	UFUNCTION(BlueprintCallable, Category = "Match3|Board")
	void CancelTargeting();

	// --- Queries ---

	/** The piece at Coord, or None if the cell is empty or off the board. */
	UFUNCTION(BlueprintPure, Category = "Match3|Board")
	APuzzlePieceBase* GetPieceAt(FGridCoord Coord) const;

	UFUNCTION(BlueprintPure, Category = "Match3|Board")
	bool IsValidCoord(FGridCoord Coord) const;

	/** World location of a cell's center. Row 0 is the top row; Row may be negative for pieces spawning above. */
	UFUNCTION(BlueprintPure, Category = "Match3|Board")
	FVector GridToWorld(FGridCoord Coord) const;

	UFUNCTION(BlueprintPure, Category = "Match3|Board")
	EBoardState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Match3|Board")
	int32 GetRows() const;

	UFUNCTION(BlueprintPure, Category = "Match3|Board")
	int32 GetCols() const;

	/** Every run of 3 or more same-colored pieces in a row or a column. Reads the grid, changes nothing. */
	TArray<FMatchResult> FindMatches() const;

protected:
	// --- Filling ---

	void FillInitialBoard();
	APuzzlePieceBase* SpawnPiece(FGridCoord Coord, EPieceColor Color, EPowerUpType PowerUp, bool bFromAbove);
	EPieceColor PickColorWithoutMatch(FGridCoord Coord) const;
	EPowerUpType RollPowerUp() const;

	// --- Turn flow (timer driven) ---

	void SwapInGrid(FGridCoord A, FGridCoord B);
	void ResolveStep();
	void ClearCells(const TArray<FGridCoord>& Cells);
	void CollapseAndRefill();
	void FinishTurn();

	// --- Helpers ---

	void ClearPreview();
	void SetState(EBoardState NewState);
	int32 ToIndex(FGridCoord Coord) const;

private:
	UPROPERTY()
	TObjectPtr<UMatch3LevelConfig> Config;

	/** Rows x Cols slots, index = Row x Cols + Col. A null slot is an empty cell. */
	UPROPERTY(VisibleInstanceOnly, Category = "Match3|Board")
	TArray<TObjectPtr<APuzzlePieceBase>> Grid;

	UPROPERTY(VisibleInstanceOnly, Category = "Match3|Board")
	EBoardState State = EBoardState::Initializing;

	/** Goes up by 1 each cascade step and resets every turn. */
	UPROPERTY(VisibleInstanceOnly, Category = "Match3|Board")
	int32 ComboLevel = 0;

	/** Cascade steps this turn, compared against MaxCascadeSafety. */
	int32 CascadeSteps = 0;

	// --- Targeting ---

	EPowerUpType PendingPowerUp = EPowerUpType::None;

	UPROPERTY()
	TObjectPtr<UPowerUpEffect> ActiveEffect;

	UPROPERTY()
	TObjectPtr<AMatch3PlayerState> TargetingInstigator;

	TArray<FGridCoord> PreviewCells;

	/** One timer drives every timed phase: swap, clear and fall. */
	FTimerHandle StepTimer;
};