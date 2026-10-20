// PowerUpInventoryComponent.cpp

#include "Rules/PowerUpInventoryComponent.h"

UPowerUpInventoryComponent::UPowerUpInventoryComponent()
{
	// The inventory only reacts to calls; it never needs to tick.
	PrimaryComponentTick.bCanEverTick = false;
}

bool UPowerUpInventoryComponent::AddPowerUp(EPowerUpType Type)
{
	// TODO(Student): FIFO queue, Capacity slots, Queue[0] is the ready one.
	// - Ignore EPowerUpType::None.
	// - If the queue is full, remove Queue[0] and broadcast OnPowerUpDiscarded with it.
	// - Append Type to the back and broadcast OnInventoryChanged with the new Queue.
	// - Return true only if one was discarded.
	// Until then collected power-ups never reach the inventory.
	return false;
}

EPowerUpType UPowerUpInventoryComponent::ConsumeReady()
{
	// TODO(Student): Remove and return the ready power-up (Queue[0]).
	// - An empty queue returns EPowerUpType::None.
	// - Broadcast OnPowerUpActivated with the type, then OnInventoryChanged with the new Queue.
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