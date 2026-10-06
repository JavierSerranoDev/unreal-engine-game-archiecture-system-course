// PuzzlePieceBase.cpp
// Each command updates the data first, then hands the visual part to Blueprint.

#include "PuzzlePieceBase.h"
#include "Components/SceneComponent.h"
#include "Match3LevelConfig.h"

APuzzlePieceBase::APuzzlePieceBase()
{
	// Pieces never tick; timelines in BP_PuzzlePiece do the animation.
	PrimaryActorTick.bCanEverTick = false;

	// A plain root so BP_PuzzlePiece can attach its meshes under it.
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void APuzzlePieceBase::InitPiece(FGridCoord Coord, EPieceColor InColor, EPowerUpType InPowerUp, UMatch3LevelConfig* InConfig)
{
	GridCoord = Coord;
	Color = InColor;
	PowerUp = InPowerUp;
	Config = InConfig;
	bHighlighted = false;

	OnPieceInitialized();
}

void APuzzlePieceBase::MoveToCoord(FGridCoord NewCoord, FVector WorldTarget, float Duration)
{
	GridCoord = NewCoord;
	OnMoveTo(WorldTarget, Duration);
}

void APuzzlePieceBase::PlayMatched(float Duration)
{
	OnMatched(Duration);
}

void APuzzlePieceBase::SetHighlighted(bool bOn)
{
	if (bHighlighted == bOn)
	{
		return;
	}
	bHighlighted = bOn;
	OnHighlightChanged(bOn);
}

void APuzzlePieceBase::PlayInvalidSwap(ESwipeDirection Dir)
{
	OnInvalidSwap(Dir);
}
