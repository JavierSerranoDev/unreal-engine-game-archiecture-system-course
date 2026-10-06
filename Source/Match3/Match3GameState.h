// Match3GameState.h
// Holds the match data everyone can see: score, moves, target and combo.
// Blueprints can read it and listen to it, but only AMatch3GameMode can change it.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Match3Types.h"
#include "Match3GameState.generated.h"

UCLASS()
class MATCH3_API AMatch3GameState : public AGameStateBase
{
	GENERATED_BODY()

	// The GameMode is the only writer of game state. Being a friend lets it call
	// the private setters below, which are plain C++ so Blueprints cannot see them.
	friend class AMatch3GameMode;

public:
	// --- Events: Blueprints bind to these, C++ broadcasts them ---

	UPROPERTY(BlueprintAssignable, Category = "Match3|Events")
	FOnScoreChanged OnScoreChanged;

	UPROPERTY(BlueprintAssignable, Category = "Match3|Events")
	FOnMovesChanged OnMovesChanged;

	UPROPERTY(BlueprintAssignable, Category = "Match3|Events")
	FOnComboChanged OnComboChanged;

	UPROPERTY(BlueprintAssignable, Category = "Match3|Events")
	FOnGameOver OnGameOver;

	// --- Read-only access for Blueprints ---

	UFUNCTION(BlueprintPure, Category = "Match3")
	int32 GetScore() const { return Score; }

	UFUNCTION(BlueprintPure, Category = "Match3")
	int32 GetMovesRemaining() const { return MovesRemaining; }

	UFUNCTION(BlueprintPure, Category = "Match3")
	int32 GetTargetScore() const { return TargetScore; }

	UFUNCTION(BlueprintPure, Category = "Match3")
	int32 GetComboLevel() const { return ComboLevel; }

	/** Score / TargetScore clamped to 0..1, ready for a progress bar. */
	UFUNCTION(BlueprintPure, Category = "Match3")
	float GetScoreProgress() const;

	UFUNCTION(BlueprintPure, Category = "Match3")
	bool IsGameOver() const { return bGameOver; }

private:
	// --- C++-only setters, called by AMatch3GameMode ---

	void InitMatch(int32 MaxMoves, int32 Target);
	void AddScore(int32 Delta);
	void DecrementMoves();
	void SetComboLevel(int32 Level);
	void SetGameOver(bool bWon);

	// --- Data. VisibleInstanceOnly so you can watch it in the Details panel while playing ---

	UPROPERTY(VisibleInstanceOnly, Category = "Match3")
	int32 Score = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Match3")
	int32 MovesRemaining = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Match3")
	int32 TargetScore = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Match3")
	int32 ComboLevel = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Match3")
	bool bGameOver = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Match3")
	bool bPlayerWon = false;
};
