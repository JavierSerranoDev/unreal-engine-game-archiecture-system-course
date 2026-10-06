// Match3PlayerState.h
// The player's own data. For this game that is just the power-up inventory.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Match3PlayerState.generated.h"

class UPowerUpInventoryComponent;

UCLASS()
class MATCH3_API AMatch3PlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AMatch3PlayerState();

	UFUNCTION(BlueprintPure, Category = "Match3")
	UPowerUpInventoryComponent* GetInventory() const { return Inventory; }

private:
	/** Created in the constructor, so every Match3 player always has one. */
	UPROPERTY(VisibleAnywhere, Category = "Match3")
	TObjectPtr<UPowerUpInventoryComponent> Inventory;
};
