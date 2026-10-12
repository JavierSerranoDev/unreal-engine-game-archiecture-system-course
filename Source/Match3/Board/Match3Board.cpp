// Match3Board.cpp

#include "Board/Match3Board.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Rules/Match3GameMode.h"
#include "Rules/Match3LevelConfig.h"
#include "Rules/Match3PlayerState.h"
#include "PowerUps/PowerUpEffect.h"
#include "Board/PuzzlePieceBase.h"

AMatch3Board::AMatch3Board()
{
	// The board reacts to requests and timers; it never needs to tick.
	PrimaryActorTick.bCanEverTick = false;

	// A plain root so BP_Match3Board can attach its camera and background under it.
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

// ---------------------------------------------------------------------------
// Commands from the GameMode
// ---------------------------------------------------------------------------

void AMatch3Board::InitializeBoard(UMatch3LevelConfig* InConfig)
{
	if (!InConfig || !InConfig->PieceClass)
	{
		UE_LOG(LogTemp, Error, TEXT("AMatch3Board::InitializeBoard: Invalid config, invalid piece class."));
		return;
	}

	Config = InConfig;
	SetState(EBoardState::Initializing);
	Grid.Init(nullptr, GetRows() * GetCols());
	FillInitialBoard();
	SetState(EBoardState::Idle);
}

bool AMatch3Board::BeginTargeting(EPowerUpType Type, UPowerUpEffect* Effect, AMatch3PlayerState* InInstigator)
{
	// TODO: implement (diagram 12).
	// Only from Idle with a valid Effect. Store PendingPowerUp, ActiveEffect and
	// TargetingInstigator, SetState(Targeting), broadcast OnTargetingStarted.
	return false;
}

void AMatch3Board::LockForGameOver()
{
	GetWorldTimerManager().ClearTimer(StepTimer);
	ClearPreview();
	ActiveEffect = nullptr;
	TargetingInstigator = nullptr;
	SetState(EBoardState::GameOver);
}

// ---------------------------------------------------------------------------
// Requests from the player controller
// ---------------------------------------------------------------------------

bool AMatch3Board::RequestSwap(FGridCoord From, ESwipeDirection Dir)
{
	// TODO: implement (diagram 10).
	// Ignore unless Idle. If From.Neighbor(Dir) is off the board, PlayInvalidSwap and return false.
	// Otherwise ConsumeMove on the GameMode, SwapInGrid, reset ComboLevel and CascadeSteps,
	// SetState(Swapping), MoveToCoord both pieces, and start StepTimer for SwapDuration -> ResolveStep.
	return false;
}

void AMatch3Board::PreviewTarget(FGridCoord Coord)
{
	// TODO: implement (diagram 12).
	// Only in Targeting. ClearPreview, ask ActiveEffect->GetAffectedCells,
	// SetHighlighted(true) on those pieces and remember them in PreviewCells.
}

bool AMatch3Board::ConfirmTarget(FGridCoord Coord)
{
	// TODO: implement (diagram 12).
	// Only in Targeting on a valid coord. Get the cells, report HandlePowerUpConfirmed
	// to the GameMode, broadcast OnTargetingEnded(true), PlayMatched on the cells,
	// SetState(Resolving) and run the clear -> fall -> ResolveStep loop.
	// Power-ups inside the blast are not collected.
	return false;
}

void AMatch3Board::CancelTargeting()
{
	// TODO: implement (diagram 12).
	// Only in Targeting. ClearPreview, drop ActiveEffect, SetState(Idle),
	// broadcast OnTargetingEnded(false). The power-up stays in the inventory.
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

APuzzlePieceBase* AMatch3Board::GetPieceAt(FGridCoord Coord) const
{
	if (!IsValidCoord(Coord))
	{
		return nullptr;
	}
	const int32 Index = ToIndex(Coord);
	return Grid.IsValidIndex(Index) ? Grid[Index].Get() : nullptr;
}

bool AMatch3Board::IsValidCoord(FGridCoord Coord) const
{
	return Coord.IsValid(GetRows(), GetCols());
}

FVector AMatch3Board::GridToWorld(FGridCoord Coord) const
{
	// The board lies flat on the actor's local XY plane, centered on the actor,
	// for a top-down camera: columns go along +Y (screen right) and rows go
	// along -X (screen down), so Row 0 is the top row.
	const float TileSize = Config ? Config->TileSize : 100.f;
	const float CenterRow = (GetRows() - 1) * 0.5f;
	const float CenterCol = (GetCols() - 1) * 0.5f;

	const FVector Local((CenterRow - Coord.Row) * TileSize, (Coord.Col - CenterCol) * TileSize, 0.f);
	return GetActorTransform().TransformPosition(Local);
}

int32 AMatch3Board::GetRows() const
{
	return Config ? Config->Rows : 0;
}

int32 AMatch3Board::GetCols() const
{
	return Config ? Config->Cols : 0;
}

TArray<FMatchResult> AMatch3Board::FindMatches() const
{
	// TODO: implement (STUDENT TODO in the starter project).
	// Scan each row, then each column, for runs of 3 or more pieces of the same color.
	// Add one FMatchResult per run. Empty cells break a run.
	return TArray<FMatchResult>();
}

// ---------------------------------------------------------------------------
// Filling
// ---------------------------------------------------------------------------

void AMatch3Board::FillInitialBoard()
{
	// Row by row, left to right, so the cells to the left and above already exist when PickColorWithoutMatch checks them.
	for (int32 Row = 0; Row < GetRows(); ++Row)
	{
		for (int32 Col = 0; Col < GetCols(); ++Col)
		{
			const FGridCoord Coord(Row, Col);
			const EPieceColor Color = PickColorWithoutMatch(Coord);
			const EPowerUpType PowerUp = RollPowerUp();
			SpawnPiece(Coord, Color, PowerUp, false);
		}
	}
}

APuzzlePieceBase* AMatch3Board::SpawnPiece(FGridCoord Coord, EPieceColor Color, EPowerUpType PowerUp, bool bFromAbove)
{
	if (!Config || !Config->PieceClass || !IsValidCoord(Coord))
	{
		UE_LOG(LogTemp, Error, TEXT("AMatch3Board::SpawnPiece: invalid config, piece class or coord."));
		return nullptr;
	}

	// A refill piece starts one board-height above its cell and the caller moves it down.
	const FGridCoord SpawnCoord = bFromAbove ? FGridCoord(Coord.Row - GetRows(), Coord.Col) : Coord;
	const FTransform SpawnTransform(GetActorRotation(), GridToWorld(SpawnCoord));

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Owner = this;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APuzzlePieceBase* Piece = GetWorld()->SpawnActor<APuzzlePieceBase>(Config->PieceClass, SpawnTransform, SpawnInfo);
	if (!Piece)
	{
		return nullptr;
	}

	// The data says which cell the piece belongs to, even while it is still above the board.
	Piece->InitPiece(Coord, Color, PowerUp, Config);
	Grid[ToIndex(Coord)] = Piece;
	return Piece;
}

EPieceColor AMatch3Board::PickColorWithoutMatch(FGridCoord Coord) const
{
	// 1. Every color in play.
	TArray<EPieceColor> Candidates;
	for (int32 Index = 0; Index < Config->NumColors; ++Index)
	{
		Candidates.Add(static_cast<EPieceColor>(Index));
	}

	// 2. Two same-colored pieces to the left: this cell must not repeat that color.
	const APuzzlePieceBase* Left1 = GetPieceAt(FGridCoord(Coord.Row, Coord.Col - 1));
	const APuzzlePieceBase* Left2 = GetPieceAt(FGridCoord(Coord.Row, Coord.Col - 2));
	if (Left1 && Left2 && Left1->GetColor() == Left2->GetColor())
	{
		Candidates.Remove(Left1->GetColor());
	}

	// 3. Two same-colored pieces above: this cell must not repeat that color.
	const APuzzlePieceBase* Up1 = GetPieceAt(FGridCoord(Coord.Row - 1, Coord.Col));
	const APuzzlePieceBase* Up2 = GetPieceAt(FGridCoord(Coord.Row - 2, Coord.Col));
	if (Up1 && Up2 && Up1->GetColor() == Up2->GetColor())
	{
		Candidates.Remove(Up1->GetColor());
	}

	// 4. Any color that is left is safe.
	return Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
}

EPowerUpType AMatch3Board::RollPowerUp() const
{
	if (Config->PowerUps.Num() == 0 || FMath::FRand() >= Config->PowerUpSpawnChance)
	{
		return EPowerUpType::None;
	}

	// Every power-up listed in the level config is equally likely.
	const int32 Index = FMath::RandRange(0, Config->PowerUps.Num() - 1);
	return Config->PowerUps[Index].Type;
}

// ---------------------------------------------------------------------------
// Turn flow
// ---------------------------------------------------------------------------

void AMatch3Board::SwapInGrid(FGridCoord A, FGridCoord B)
{
	// TODO: implement. Swap the two Grid slots (data only; MoveToCoord does the visuals).
}

void AMatch3Board::ResolveStep()
{
	// TODO: implement (diagram 11).
	// SetState(Resolving), FindMatches. None, or CascadeSteps reached MaxCascadeSafety: FinishTurn.
	// Otherwise ComboLevel + 1, AwardMatch each match, GrantPowerUp for matched pieces that carry one,
	// PlayMatched, then after ClearDuration ClearCells + CollapseAndRefill, and after FallDuration ResolveStep again.
}

void AMatch3Board::ClearCells(const TArray<FGridCoord>& Cells)
{
	// TODO: implement. Destroy each piece and null its Grid slot.
}

void AMatch3Board::CollapseAndRefill()
{
	// TODO: implement (diagram 11).
	// Per column, from the bottom row up: move pieces down into empty slots (MoveToCoord, FallDuration),
	// then SpawnPiece from above into the slots left at the top.
}

void AMatch3Board::FinishTurn()
{
	// TODO: implement (diagram 11).
	// Ask the GameMode to EvaluateEndOfTurn. If it did not lock the board for game over, SetState(Idle).
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void AMatch3Board::ClearPreview()
{
	for (const FGridCoord& Cell : PreviewCells)
	{
		if (APuzzlePieceBase* Piece = GetPieceAt(Cell))
		{
			Piece->SetHighlighted(false);
		}
	}
	PreviewCells.Reset();
}

void AMatch3Board::SetState(EBoardState NewState)
{
	if (State == NewState)
	{
		return;
	}
	const EBoardState OldState = State;
	State = NewState;
	OnBoardStateChanged.Broadcast(OldState, NewState);
}

int32 AMatch3Board::ToIndex(FGridCoord Coord) const
{
	return Coord.Row * GetCols() + Coord.Col;
}