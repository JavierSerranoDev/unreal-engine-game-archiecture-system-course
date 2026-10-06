// PuzzlePieceBase.h
// A thin piece: C++ stores where it is and what it is, then fires
// BlueprintImplementableEvents. BP_PuzzlePiece implements those with
// timelines and materials and never decides anything about the game.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/Match3Types.h"
#include "PuzzlePieceBase.generated.h"

class UMatch3LevelConfig;

UCLASS(Abstract, Blueprintable)
class MATCH3_API APuzzlePieceBase : public AActor
{
	GENERATED_BODY()

public:
	APuzzlePieceBase();

	// --- Commands from AMatch3Board. Plain C++, so Blueprints cannot move or recolor a piece ---

	/** Stores the data, then fires OnPieceInitialized so the Blueprint can apply its visuals. */
	void InitPiece(FGridCoord Coord, EPieceColor InColor, EPowerUpType InPowerUp, UMatch3LevelConfig* InConfig);

	/** Updates GridCoord right away (the data), then lets the Blueprint animate to WorldTarget (the look). */
	void MoveToCoord(FGridCoord NewCoord, FVector WorldTarget, float Duration);

	/** Plays the pop. The board destroys the actor after Duration. */
	void PlayMatched(float Duration);

	/** Turns the power-up target preview on or off. */
	void SetHighlighted(bool bOn);

	/** Shakes toward Dir when a swipe points off the board. */
	void PlayInvalidSwap(ESwipeDirection Dir);

	// --- Read-only access for Blueprints ---

	UFUNCTION(BlueprintPure, Category = "Match3|Piece")
	FGridCoord GetGridCoord() const { return GridCoord; }

	UFUNCTION(BlueprintPure, Category = "Match3|Piece")
	EPieceColor GetColor() const { return Color; }

	UFUNCTION(BlueprintPure, Category = "Match3|Piece")
	EPowerUpType GetPowerUp() const { return PowerUp; }

	UFUNCTION(BlueprintPure, Category = "Match3|Piece")
	bool HasPowerUp() const { return PowerUp != EPowerUpType::None; }

protected:
	// --- Blueprint events: declared here, implemented in BP_PuzzlePiece ---

	/** Apply the color material and power-up icon. Config is already set. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Match3|Piece")
	void OnPieceInitialized();

	/** Animate from the current location to WorldTarget. Play the timeline at 1 / Duration. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Match3|Piece")
	void OnMoveTo(FVector WorldTarget, float Duration);

	/** Play the pop animation. Do not destroy the actor; the board does that. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Match3|Piece")
	void OnMatched(float Duration);

	UFUNCTION(BlueprintImplementableEvent, Category = "Match3|Piece")
	void OnHighlightChanged(bool bOn);

	UFUNCTION(BlueprintImplementableEvent, Category = "Match3|Piece")
	void OnInvalidSwap(ESwipeDirection Dir);

	// --- Data. Blueprints read it to choose visuals; only C++ writes it ---

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Match3|Piece")
	FGridCoord GridCoord;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Match3|Piece")
	EPieceColor Color = EPieceColor::Red;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Match3|Piece")
	EPowerUpType PowerUp = EPowerUpType::None;

	/** The level config, so the Blueprint can call FindColorVisual and FindPowerUp. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Match3|Piece")
	TObjectPtr<UMatch3LevelConfig> Config;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Match3|Piece")
	bool bHighlighted = false;
};