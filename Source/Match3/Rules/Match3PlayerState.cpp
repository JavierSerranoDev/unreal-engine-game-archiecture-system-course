// Match3PlayerState.cpp

#include "Rules/Match3PlayerState.h"
#include "Rules/PowerUpInventoryComponent.h"

AMatch3PlayerState::AMatch3PlayerState()
{
	Inventory = CreateDefaultSubobject<UPowerUpInventoryComponent>(TEXT("Inventory"));
}