// PowerUpEffect.h
// Strategy pattern: each power-up is a Blueprint child of this class that only
// answers "which cells do I hit?". The board does the clearing, scoring and
// cascades, so a designer can add a new power-up without touching C++.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Core/Match3Types.h"
#include "PowerUpEffect.generated.h"

class AMatch3Board;

UCLASS(Abstract, Blueprintable, BlueprintType)
class MATCH3_API UPowerUpEffect : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Match3|PowerUp")
	EPowerUpType GetType() const { return Type; }

	UFUNCTION(BlueprintPure, Category = "Match3|PowerUp")
	ETargetMode GetTargetMode() const { return TargetMode; }

	UFUNCTION(BlueprintPure, Category = "Match3|PowerUp")
	FText GetPromptText() const { return PromptText; }

	/**
	 * The cells this power-up clears when aimed at Target.
	 * Each BP_PowerUp_* overrides this. It must only read the board, never change it.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Match3|PowerUp")
	TArray<FGridCoord> GetAffectedCells(AMatch3Board* Board, FGridCoord Target);

protected:
	/** C++ fallback when a Blueprint does not override it: just the target cell. */
	virtual TArray<FGridCoord> GetAffectedCells_Implementation(AMatch3Board* Board, FGridCoord Target);

	// Set these in each Blueprint child's Class Defaults.

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match3|PowerUp")
	EPowerUpType Type = EPowerUpType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match3|PowerUp")
	ETargetMode TargetMode = ETargetMode::Cell;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match3|PowerUp")
	FText PromptText;
};