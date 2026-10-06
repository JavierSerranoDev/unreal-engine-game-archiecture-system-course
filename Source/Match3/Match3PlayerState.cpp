// Match3PlayerState.cpp

#include "Match3PlayerState.h"
#include "PowerUpInventoryComponent.h"

AMatch3PlayerState::AMatch3PlayerState()
{
	Inventory = CreateDefaultSubobject<UPowerUpInventoryComponent>(TEXT("Inventory"));
}
