// PowerUpEffect.cpp

#include "PowerUps/PowerUpEffect.h"
#include "Board/Match3Board.h"

TArray<FGridCoord> UPowerUpEffect::GetAffectedCells_Implementation(AMatch3Board* Board, FGridCoord Target)
{
	TArray<FGridCoord> Cells;
	if (Board && Board->IsValidCoord(Target))
	{
		Cells.Add(Target);
	}
	return Cells;
}