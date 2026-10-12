// PowerUpInventoryComponent.cpp

#include "Rules/PowerUpInventoryComponent.h"

UPowerUpInventoryComponent::UPowerUpInventoryComponent()
{
	// The inventory only reacts to calls; it never needs to tick.
	PrimaryComponentTick.bCanEverTick = false;
}

bool UPowerUpInventoryComponent::AddPowerUp(EPowerUpType Type)
{
	if (Type == EPowerUpType::None)
	{
		return false;
	}

	bool bDiscarded = false;

	if (Queue.Num() >= Capacity)
	{
		const EPowerUpType DiscardedType = Queue[0];
		Queue.RemoveAt(0);
		OnPowerUpDiscarded.Broadcast(DiscardedType);
		bDiscarded = true;
	}

	Queue.Add(Type);
	OnInventoryChanged.Broadcast(Queue);

	return bDiscarded;
}

EPowerUpType UPowerUpInventoryComponent::ConsumeReady()
{
	if (Queue.Num() == 0)
	{
		return EPowerUpType::None;
	}

	const EPowerUpType Type = Queue[0];
	Queue.RemoveAt(0);
	OnPowerUpActivated.Broadcast(Type);
	OnInventoryChanged.Broadcast(Queue);
	return Type;
}

void UPowerUpInventoryComponent::SetCapacity(int32 NewCapacity)
{
	Capacity = FMath::Max(1, NewCapacity);
}

EPowerUpType UPowerUpInventoryComponent::PeekReady() const
{
	return Queue.Num() > 0 ? Queue[0] : EPowerUpType::None;
}