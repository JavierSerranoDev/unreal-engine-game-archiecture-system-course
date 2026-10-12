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
	APuzzlePieceBase* PieceA = GetPieceAt(From);
	if (!PieceA || State != EBoardState::Idle)
	{
		return false;
	}

	// Off the board: shake the piece and spend nothing.
	const FGridCoord To = From.Neighbor(Dir);
	if (!IsValidCoord(To))
	{
		PieceA->PlayInvalidSwap(Dir);
		return false;
	}

	// The GameMode owns the moves. Every valid swap costs one, match or not.
	AMatch3GameMode* GameMode = GetWorld()->GetAuthGameMode<AMatch3GameMode>();
	if (!GameMode || !GameMode->ConsumeMove())
	{
		return false;
	}

	// A new turn starts, so the combo counts up from zero again.
	APuzzlePieceBase* PieceB = GetPieceAt(To);
	SwapInGrid(From, To);
	ComboLevel = 0;
	CascadeSteps = 0;
	SetState(EBoardState::Swapping);

	// The data is already swapped; now let both pieces animate there.
	PieceA->MoveToCoord(To, GridToWorld(To), Config->SwapDuration);
	if (PieceB)
	{
		PieceB->MoveToCoord(From, GridToWorld(From), Config->SwapDuration);
	}

	// Swaps never revert. Once the animation time is up, look for matches.
	GetWorldTimerManager().SetTimer(StepTimer, this, &AMatch3Board::ResolveStep, Config->SwapDuration);
	return true;
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
	// True when Coord holds a piece of Color. Off-board and empty cells never match.
	auto HasColorAt = [this](FGridCoord Coord, EPieceColor Color)
		{
			const APuzzlePieceBase* Piece = GetPieceAt(Coord);
			return Piece && Piece->GetColor() == Color;
		};

	TArray<FMatchResult> Matches;

	// Rows: walk left to right.
	for (int32 Row = 0; Row < GetRows(); ++Row)
	{
		for (int32 Col = 0; Col < GetCols(); ++Col)
		{
			const APuzzlePieceBase* Piece = GetPieceAt(FGridCoord(Row, Col));
			// Only start counting at the first piece of a run, so each run is reported once.
			if (!Piece || HasColorAt(FGridCoord(Row, Col - 1), Piece->GetColor()))
			{
				continue;
			}

			FMatchResult Run;
			Run.Color = Piece->GetColor();
			Run.bHorizontal = true;
			for (int32 C = Col; HasColorAt(FGridCoord(Row, C), Run.Color); ++C)
			{
				Run.Cells.Add(FGridCoord(Row, C));
			}

			if (Run.Count() >= 3)
			{
				Matches.Add(Run);
			}
		}
	}

	// Columns: the same idea, walking top to bottom.
	for (int32 Col = 0; Col < GetCols(); ++Col)
	{
		for (int32 Row = 0; Row < GetRows(); ++Row)
		{
			const APuzzlePieceBase* Piece = GetPieceAt(FGridCoord(Row, Col));
			if (!Piece || HasColorAt(FGridCoord(Row - 1, Col), Piece->GetColor()))
			{
				continue;
			}

			FMatchResult Run;
			Run.Color = Piece->GetColor();
			Run.bHorizontal = false;
			for (int32 R = Row; HasColorAt(FGridCoord(R, Col), Run.Color); ++R)
			{
				Run.Cells.Add(FGridCoord(R, Col));
			}

			if (Run.Count() >= 3)
			{
				Matches.Add(Run);
			}
		}
	}

	return Matches;
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
	// Data only. RequestSwap calls MoveToCoord afterwards for the visuals.
	Grid.Swap(ToIndex(A), ToIndex(B));
}

void AMatch3Board::ResolveStep()
{
	SetState(EBoardState::Resolving);

	const TArray<FMatchResult> Matches = FindMatches();
	if (Matches.Num() == 0)
	{
		FinishTurn(); // The board has settled.
		return;
	}

	if (CascadeSteps >= Config->MaxCascadeSafety)
	{
		UE_LOG(LogTemp, Warning, TEXT("AMatch3Board: cascade stopped after %d steps. Bug guard, check the refill."), CascadeSteps);
		FinishTurn();
		return;
	}

	++CascadeSteps;
	++ComboLevel;

	// Score every run. An L or T shape is two runs sharing a cell,
	// so the cells go into a set: each piece is collected and cleared once.
	AMatch3GameMode* GameMode = GetWorld()->GetAuthGameMode<AMatch3GameMode>();
	TSet<FGridCoord> MatchedCells;
	for (const FMatchResult& Match : Matches)
	{
		if (GameMode)
		{
			GameMode->AwardMatch(Match, ComboLevel);
		}
		MatchedCells.Append(Match.Cells);
	}

	const TArray<FGridCoord> Cells = MatchedCells.Array();
	for (const FGridCoord& Cell : Cells)
	{
		APuzzlePieceBase* Piece = GetPieceAt(Cell);
		if (GameMode && Piece->HasPowerUp())
		{
			GameMode->GrantPowerUp(Piece->GetPowerUp());
		}
		Piece->PlayMatched(Config->ClearDuration);
	}

	// Let the pop play, then remove the pieces and let the rest fall.
	GetWorldTimerManager().SetTimer(StepTimer, FTimerDelegate::CreateWeakLambda(this, [this, Cells]()
		{
			ClearCells(Cells);
			CollapseAndRefill();
		}), Config->ClearDuration, false);
}

void AMatch3Board::ClearCells(const TArray<FGridCoord>& Cells)
{
	for (const FGridCoord& Cell : Cells)
	{
		if (APuzzlePieceBase* Piece = GetPieceAt(Cell))
		{
			Piece->Destroy();
			Grid[ToIndex(Cell)] = nullptr;
		}
	}
}

void AMatch3Board::CollapseAndRefill()
{
	for (int32 Col = 0; Col < GetCols(); ++Col)
	{
		int32 TargetRow = GetRows() - 1;

		for (int32 Row = GetRows() - 1; Row >= 0; --Row)
		{
			const FGridCoord Coord(Row, Col);
			if (APuzzlePieceBase* Piece = GetPieceAt(Coord))
			{
				if (Row != TargetRow)
				{
					// Drop the piece into the lowest empty slot in this column.
					const FGridCoord To(TargetRow, Col);
					Grid[ToIndex(To)] = Piece;
					Grid[ToIndex(Coord)] = nullptr;
					Piece->MoveToCoord(To, GridToWorld(To), Config->FallDuration);
				}
				--TargetRow;
			}
		}
		// Fill the empty cells above with new pieces.
		for (int32 Row = TargetRow; Row >= 0; --Row)
		{
			const FGridCoord Coord(Row, Col);
			const EPieceColor Color = static_cast<EPieceColor>(FMath::RandRange(0, Config->NumColors - 1));
			const EPowerUpType PowerUp = RollPowerUp();
			if (APuzzlePieceBase* NewPiece = SpawnPiece(Coord, Color, PowerUp, true))
			{
				NewPiece->MoveToCoord(Coord, GridToWorld(Coord), Config->FallDuration);
			}
		}
	}

	// When the fall is over, check the board again: this is the cascade loop.
	GetWorldTimerManager().SetTimer(StepTimer, this, &AMatch3Board::ResolveStep, Config->FallDuration);
}

void AMatch3Board::FinishTurn()
{
	// Win and lose are checked only here, once the board has settled.
	if (AMatch3GameMode* GameMode = GetWorld()->GetAuthGameMode<AMatch3GameMode>())
	{
		GameMode->EvaluateEndOfTurn();
	}

	// EvaluateEndOfTurn may have ended the game and locked the board.
	if (State != EBoardState::GameOver)
	{
		SetState(EBoardState::Idle);
	}
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