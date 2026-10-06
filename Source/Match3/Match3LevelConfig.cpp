// Match3LevelConfig.cpp

#include "Match3LevelConfig.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "PowerUpEffect.h"

FPowerUpEntry UMatch3LevelConfig::FindPowerUp(EPowerUpType Type) const
{
	for (const FPowerUpEntry& Entry : PowerUps)
	{
		if (Entry.Type == Type)
		{
			return Entry;
		}
	}
	return FPowerUpEntry();
}

FPieceColorVisual UMatch3LevelConfig::FindColorVisual(EPieceColor Color) const
{
	for (const FPieceColorVisual& Visual : ColorVisuals)
	{
		if (Visual.Color == Color)
		{
			return Visual;
		}
	}

	FPieceColorVisual Fallback;
	Fallback.Color = Color;
	return Fallback;
}
