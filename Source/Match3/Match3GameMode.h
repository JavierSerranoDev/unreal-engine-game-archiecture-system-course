// Match3GameMode.h
// The "game admin": the only class that changes the score, the moves or the inventory.
// It spawns the board, applies the rules from the level config, and decides win or lose.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Match3Types.h"
#include "Match3GameMode.generated.h"

class AMatch3Board;
class AMatch3GameState;
class AMatch3PlayerState;
class UMatch3LevelConfig;

UCLASS()
class MATCH3_API AMatch3GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMatch3GameMode();

	/** Builds the match (GameState values, board) before any actor gets BeginPlay. */
	virtual void StartPlay() override;

	/** Gives each new player's inventory the capacity from the level config. */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	// --- Read-only access for Blueprints ---

	UFUNCTION(BlueprintPure, Category = "Match3")
	UMatch3LevelConfig* GetLevelConfig() const { return LevelConfig; }

	UFUNCTION(BlueprintPure, Category = "Match3")
	AMatch3Board* GetBoard() const { return Board; }

	// --- Request from the player controller (Blueprint) ---

	/**
	 * Starts targeting with the ready power-up, if there is one and the board is Idle.
	 * Spends nothing: the power-up is consumed only when the target is confirmed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Match3")
	bool TryActivateReadyPowerUp(AMatch3PlayerState* Player);

	// --- Reports from AMatch3Board. Plain C++, so Blueprints cannot change the rules ---

	/** Spends one move. Returns false if there were none left. */
	bool ConsumeMove();

	/** Scores one match at ComboLevel and returns the points awarded. */
	int32 AwardMatch(const FMatchResult& Match, int32 ComboLevel);

	/** Scores a power-up blast (Base x CellCount) and returns the points awarded. */
	int32 AwardBlast(int32 CellCount);

	/** Adds a collected power-up to the player's inventory. */
	void GrantPowerUp(EPowerUpType Type);

	/** Consumes the ready power-up and scores the blast. */
	void HandlePowerUpConfirmed(AMatch3PlayerState* Player, int32 CellCount);

	/** Called once the board settles: resets the combo, then checks win (first) and lose. */
	void EvaluateEndOfTurn();

	/** Score = Base x Count x (Count - 2) x Combo. 3 pieces at combo 1 with Base 10 = 30 points. */
	UFUNCTION(BlueprintPure, Category = "Match3")
	static int32 CalculateMatchScore(int32 Count, int32 Combo, int32 Base);

protected:
	/** The level's rules and assets (DA_Level01). Set in BP_Match3GameMode. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match3")
	TObjectPtr<UMatch3LevelConfig> LevelConfig;

	/** The board Blueprint to spawn (BP_Match3Board). Set in BP_Match3GameMode. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match3")
	TSubclassOf<AMatch3Board> BoardClass;

private:
	/** Locks the board and tells the GameState, which broadcasts OnGameOver. */
	void EndGame(bool bWon);

	/** The GameState cast to our type, so the rules can call its C++-only setters. */
	AMatch3GameState* GetMatch3GameState() const;

	UPROPERTY()
	TObjectPtr<AMatch3Board> Board;
};
