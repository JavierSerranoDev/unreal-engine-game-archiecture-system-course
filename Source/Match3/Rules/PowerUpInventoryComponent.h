// PowerUpInventoryComponent.h
// The player's power-up queue: first in, first out, capacity 3.
// Queue[0] is the "ready" slot, the only one the player can use.
// It stores only EPowerUpType; the level config knows everything else about a type.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/Match3Types.h"
#include "PowerUpInventoryComponent.generated.h"

UCLASS(ClassGroup = (Match3), meta = (BlueprintSpawnableComponent))
class MATCH3_API UPowerUpInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPowerUpInventoryComponent();

	// --- Events: the HUD binds to these ---

	UPROPERTY(BlueprintAssignable, Category = "Match3|Events")
	FOnInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Match3|Events")
	FOnPowerUpDiscarded OnPowerUpDiscarded;

	UPROPERTY(BlueprintAssignable, Category = "Match3|Events")
	FOnPowerUpActivated OnPowerUpActivated;

	// --- Writes. Plain C++ (no UFUNCTION) so only AMatch3GameMode can call them ---

	/**
	 * Appends Type to the back of the queue. When the queue is full, the ready
	 * one (Queue[0]) is discarded first. Returns true if one was discarded.
	 */
	bool AddPowerUp(EPowerUpType Type);

	/** Removes and returns the ready power-up, or None if the queue is empty. */
	EPowerUpType ConsumeReady();

	/** Set once by the GameMode from UMatch3LevelConfig::InventoryCapacity. */
	void SetCapacity(int32 NewCapacity);

	// --- Read-only access for Blueprints ---

	UFUNCTION(BlueprintPure, Category = "Match3|Inventory")
	bool HasReady() const { return Queue.Num() > 0; }

	/** The ready power-up without removing it, or None if the queue is empty. */
	UFUNCTION(BlueprintPure, Category = "Match3|Inventory")
	EPowerUpType PeekReady() const;

	UFUNCTION(BlueprintPure, Category = "Match3|Inventory")
	TArray<EPowerUpType> GetQueue() const { return Queue; }

	UFUNCTION(BlueprintPure, Category = "Match3|Inventory")
	int32 GetCapacity() const { return Capacity; }

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Match3|Inventory")
	TArray<EPowerUpType> Queue;

	UPROPERTY(VisibleInstanceOnly, Category = "Match3|Inventory")
	int32 Capacity = 3;
};