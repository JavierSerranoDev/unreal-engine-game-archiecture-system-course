// PowerUpInventoryComponent.cpp

#include "Rules/PowerUpInventoryComponent.h"

UPowerUpInventoryComponent::UPowerUpInventoryComponent()
{
	// The inventory only reacts to calls; it never needs to tick.
	PrimaryComponentTick.bCanEverTick = false;
}

bool UPowerUpInventoryComponent::AddPowerUp(EPowerUpType Type)
{
	// TODO: implement (diagram 13).
	// 1. Ignore None.
	// 2. If Queue is full: remember Queue[0], RemoveAt(0), broadcast OnPowerUpDiscarded.
	// 3. Add Type to the back and broadcast OnInventoryChanged(Queue).
	// 4. Return whether one was discarded.
	return false;
}

EPowerUpType UPowerUpInventoryComponent::ConsumeReady()
{
	// TODO: implement (diagram 12).
	// If the queue is empty return None. Otherwise remove Queue[0],
	// broadcast OnPowerUpActivated(Type) then OnInventoryChanged(Queue), and return Type.
	return EPowerUpType::None;
}

void UPowerUpInventoryComponent::SetCapacity(int32 NewCapacity)
{
	Capacity = FMath::Max(1, NewCapacity);
}

EPowerUpType UPowerUpInventoryComponent::PeekReady() const
{
	return Queue.Num() > 0 ? Queue[0] : EPowerUpType::None;
}