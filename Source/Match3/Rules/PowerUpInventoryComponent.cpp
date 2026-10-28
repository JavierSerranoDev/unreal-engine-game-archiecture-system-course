// PowerUpInventoryComponent.cpp

#include "Rules/PowerUpInventoryComponent.h"

UPowerUpInventoryComponent::UPowerUpInventoryComponent()
{
	// The inventory only reacts to calls; it never needs to tick.
	PrimaryComponentTick.bCanEverTick = false;
}

bool UPowerUpInventoryComponent::AddPowerUp(EPowerUpType Type)
{
	// TODO(Student): Uncomment the solution below, then delete the placeholder return at the end.
	// FIFO queue with Capacity slots. Queue[0] is the ready one.
	/*
	// Step 1: None is not a real power-up, so there is nothing to add.
	if (Type == EPowerUpType::None)
	{
		return false;
	}

	bool bDiscarded = false;

	// Step 2: If the queue is full, make room by dropping the oldest one (Queue[0]).
	//         First in, first out: RemoveAt(0) moves the rest forward one slot.
	//         Broadcast OnPowerUpDiscarded so the UI can show which one was lost.
	if (Queue.Num() >= Capacity)
	{
		const EPowerUpType DiscardedType = Queue[0];
		Queue.RemoveAt(0);
		OnPowerUpDiscarded.Broadcast(DiscardedType);
		bDiscarded = true;
	}

	// Step 3: The new power-up always goes to the back of the queue.
	Queue.Add(Type);

	// Step 4: Tell listeners (the HUD slots) the queue changed, passing the new contents.
	OnInventoryChanged.Broadcast(Queue);

	// Step 5: Report whether one had to be discarded to make room.
	return bDiscarded;
	*/

	// Placeholder so the starter compiles: collected power-ups never reach the inventory.
	// Delete this line once the solution above is uncommented.
	return false;
}

EPowerUpType UPowerUpInventoryComponent::ConsumeReady()
{
	// TODO(Student): Uncomment the solution below, then delete the placeholder return at the end.
	// Removes and returns the ready power-up (Queue[0]).
	/*
	// Step 1: An empty queue has nothing ready.
	if (Queue.Num() == 0)
	{
		return EPowerUpType::None;
	}

	// Step 2: Take the ready one from the front, then remove it so the rest move forward.
	const EPowerUpType Type = Queue[0];
	Queue.RemoveAt(0);

	// Step 3: Broadcast that it was used, then the new queue contents for the HUD.
	OnPowerUpActivated.Broadcast(Type);
	OnInventoryChanged.Broadcast(Queue);

	// Step 4: Return the type that was consumed.
	return Type;
	*/

	// Placeholder so the starter compiles: nothing is ever consumed.
	// Delete this line once the solution above is uncommented.
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