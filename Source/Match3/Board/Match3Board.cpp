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

namespace
{
	/** The board reports to the GameMode, which owns the rules. Null if the level uses another mode. */
	AMatch3GameMode* GetMatch3GameMode(const UObject* WorldContext)
	{
		const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
		return World ? World->GetAuthGameMode<AMatch3GameMode>() : nullptr;
	}

	/**
	 * Cleans up what a power-up effect returned: drops duplicates (an X Bomb's
	 * diagonals share the target) and cells with no piece. Effects are Blueprints,
	 * so the board never trusts their answer blindly.
	 */
	TArray<FGridCoord> GetUniqueOccupiedCells(const AMatch3Board& Board, const TArray<FGridCoord>& Cells)
	{
		TArray<FGridCoord> Result;
		for (const FGridCoord& Cell : Cells)
		{
			if (Board.GetPieceAt(Cell) != nullptr)
			{
				Result.AddUnique(Cell);
			}
		}
		return Result;
	}
}

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
		UE_LOG(LogTemp, Error, TEXT("AMatch3Board: the level config or its PieceClass is missing."));
		return;
	}

	Config = InConfig;
	SetState(EBoardState::Initializing);

	// One slot per cell, all empty. FillInitialBoard puts a piece in every slot.
	Grid.Init(nullptr, GetRows() * GetCols());
	FillInitialBoard();

	SetState(EBoardState::Idle);
}

bool AMatch3Board::BeginTargeting(EPowerUpType Type, UPowerUpEffect* Effect, AMatch3PlayerState* InInstigator)
{
	if (State != EBoardState::Idle || !Effect || Type == EPowerUpType::None)
	{
		return false;
	}

	PendingPowerUp = Type;
	ActiveEffect = Effect;
	TargetingInstigator = InInstigator;
	SetState(EBoardState::Targeting);

	// The controller listens to this to switch input mode and show the prompt.
	OnTargetingStarted.Broadcast(Type, Effect->GetTargetMode(), Effect->GetPromptText());
	return true;
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
	if (State != EBoardState::Idle || !PieceA)
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
	AMatch3GameMode* GameMode = GetMatch3GameMode(this);
	if (!GameMode || !GameMode->ConsumeMove())
	{
		return false;
	}

	// A new turn starts: the combo counts up again from zero.
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
	if (State != EBoardState::Targeting || !ActiveEffect)
	{
		return;
	}

	// Turn off the old highlight first, so moving the mouse never leaves cells lit.
	ClearPreview();
	if (!IsValidCoord(Coord))
	{
		return;
	}

	PreviewCells = GetUniqueOccupiedCells(*this, ActiveEffect->GetAffectedCells(this, Coord));
	for (const FGridCoord& Cell : PreviewCells)
	{
		GetPieceAt(Cell)->SetHighlighted(true);
	}
}

bool AMatch3Board::ConfirmTarget(FGridCoord Coord)
{
	if (State != EBoardState::Targeting || !ActiveEffect || !IsValidCoord(Coord))
	{
		return false;
	}

	const TArray<FGridCoord> Cells = GetUniqueOccupiedCells(*this, ActiveEffect->GetAffectedCells(this, Coord));
	if (Cells.Num() == 0)
	{
		return false; // Nothing to hit: stay in Targeting so the player can aim again.
	}

	// The GameMode consumes the power-up and scores the blast. No move is spent.
	if (AMatch3GameMode* GameMode = GetMatch3GameMode(this))
	{
		GameMode->HandlePowerUpConfirmed(TargetingInstigator, Cells.Num());
	}

	ClearPreview();
	PendingPowerUp = EPowerUpType::None;
	ActiveEffect = nullptr;
	TargetingInstigator = nullptr;

	// A blast is a turn of its own, so any cascade it causes starts at combo 1.
	ComboLevel = 0;
	CascadeSteps = 0;
	SetState(EBoardState::Resolving);
	OnTargetingEnded.Broadcast(true);

	// Straight to Clearing: the effect already chose the cells, so there is no
	// matching step. Power-ups caught in the blast are destroyed, not collected.
	for (const FGridCoord& Cell : Cells)
	{
		GetPieceAt(Cell)->PlayMatched(Config->ClearDuration);
	}
	GetWorldTimerManager().SetTimer(StepTimer, FTimerDelegate::CreateWeakLambda(this, [this, Cells]()
	{
		ClearCells(Cells);
		CollapseAndRefill();
	}), Config->ClearDuration, false);
	return true;
}

void AMatch3Board::CancelTargeting()
{
	if (State != EBoardState::Targeting)
	{
		return;
	}

	ClearPreview();
	PendingPowerUp = EPowerUpType::None;
	ActiveEffect = nullptr;
	TargetingInstigator = nullptr;
	SetState(EBoardState::Idle);

	// Nothing was consumed, so the power-up is still in the ready slot.
	OnTargetingEnded.Broadcast(false);
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
	// True when Coord holds a piece of Color. Off-board and empty cells are never a match.
	auto HasColorAt = [this](FGridCoord Coord, EPieceColor Color)
	{
		const APuzzlePieceBase* Piece = GetPieceAt(Coord);
		return Piece && Piece->GetColor() == Color;
	};

	TArray<FMatchResult> Matches;
	for (int32 Row = 0; Row < GetRows(); ++Row)
	{
		for (int32 Col = 0; Col < GetCols(); ++Col)
		{
			const FGridCoord Start(Row, Col);
			const APuzzlePieceBase* Piece = GetPieceAt(Start);
			if (!Piece)
			{
				continue;
			}

			// Check the row (walking right) and the column (walking down) from this cell.
			for (const bool bHorizontal : { true, false })
			{
				const ESwipeDirection Forward = bHorizontal ? ESwipeDirection::Right : ESwipeDirection::Down;
				const ESwipeDirection Back = bHorizontal ? ESwipeDirection::Left : ESwipeDirection::Up;

				// Only count from the first piece of a run, so each run is reported once.
				if (HasColorAt(Start.Neighbor(Back), Piece->GetColor()))
				{
					continue;
				}

				FMatchResult Run;
				Run.Color = Piece->GetColor();
				Run.bHorizontal = bHorizontal;
				for (FGridCoord Cell = Start; HasColorAt(Cell, Run.Color); Cell = Cell.Neighbor(Forward))
				{
					Run.Cells.Add(Cell);
				}

				if (Run.Count() >= 3)
				{
					Matches.Add(Run);
				}
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
	// Row by row, left to right, so the cells to the left and above are always
	// filled by the time PickColorWithoutMatch looks at them.
	for (int32 Row = 0; Row < GetRows(); ++Row)
	{
		for (int32 Col = 0; Col < GetCols(); ++Col)
		{
			const FGridCoord Coord(Row, Col);
			SpawnPiece(Coord, PickColorWithoutMatch(Coord), RollPowerUp(), false);
		}
	}
}

APuzzlePieceBase* AMatch3Board::SpawnPiece(FGridCoord Coord, EPieceColor Color, EPowerUpType PowerUp, bool bFromAbove)
{
	if (!Config || !Config->PieceClass || !IsValidCoord(Coord))
	{
		return nullptr;
	}

	// A refill piece starts one board-height above its slot, so a column's new pieces
	// keep their order and slide in from beyond the top edge. The caller moves it down.
	const FGridCoord SpawnCoord = bFromAbove ? FGridCoord(Coord.Row - GetRows(), Coord.Col) : Coord;
	const FTransform SpawnTransform(GetActorRotation(), GridToWorld(SpawnCoord));

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APuzzlePieceBase* Piece = GetWorld()->SpawnActor<APuzzlePieceBase>(Config->PieceClass, SpawnTransform, Params);
	if (!Piece)
	{
		return nullptr;
	}

	// The piece stores its target Coord even while it is still above the board.
	Piece->InitPiece(Coord, Color, PowerUp, Config);
	Grid[ToIndex(Coord)] = Piece;
	return Piece;
}

EPieceColor AMatch3Board::PickColorWithoutMatch(FGridCoord Coord) const
{
	// The colors in play, then drop any that would complete 3 in a row.
	TArray<EPieceColor> Candidates;
	for (int32 Index = 0; Index < Config->NumColors; ++Index)
	{
		Candidates.Add(static_cast<EPieceColor>(Index));
	}

	// The board fills row by row, so only the two cells to the left and the two
	// above already exist. If both share a color, this cell must not repeat it.
	const APuzzlePieceBase* Left1 = GetPieceAt(FGridCoord(Coord.Row, Coord.Col - 1));
	const APuzzlePieceBase* Left2 = GetPieceAt(FGridCoord(Coord.Row, Coord.Col - 2));
	if (Left1 && Left2 && Left1->GetColor() == Left2->GetColor())
	{
		Candidates.Remove(Left1->GetColor());
	}

	const APuzzlePieceBase* Up1 = GetPieceAt(FGridCoord(Coord.Row - 1, Coord.Col));
	const APuzzlePieceBase* Up2 = GetPieceAt(FGridCoord(Coord.Row - 2, Coord.Col));
	if (Up1 && Up2 && Up1->GetColor() == Up2->GetColor())
	{
		Candidates.Remove(Up1->GetColor());
	}

	// At most two colors are removed and NumColors is at least 3, so one is always left.
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
		UE_LOG(LogTemp, Warning, TEXT("AMatch3Board: cascade stopped after %d steps. This is a bug guard, check the refill."), CascadeSteps);
		FinishTurn();
		return;
	}

	++CascadeSteps;
	++ComboLevel;

	// Score every run. An L or T shape is two runs that share a cell, so the
	// cells go into a set: each piece is collected and cleared only once.
	AMatch3GameMode* GameMode = GetMatch3GameMode(this);
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
	const float Fall = Config->FallDuration;

	for (int32 Col = 0; Col < GetCols(); ++Col)
	{
		// The lowest slot in this column still waiting for a piece. Rows grow downward,
		// so we walk from the bottom row up and drop each piece into that slot.
		int32 TargetRow = GetRows() - 1;
		for (int32 Row = GetRows() - 1; Row >= 0; --Row)
		{
			const FGridCoord From(Row, Col);
			APuzzlePieceBase* Piece = GetPieceAt(From);
			if (!Piece)
			{
				continue;
			}

			const FGridCoord To(TargetRow, Col);
			if (To != From)
			{
				Grid[ToIndex(To)] = Piece;
				Grid[ToIndex(From)] = nullptr;
				Piece->MoveToCoord(To, GridToWorld(To), Fall);
			}
			--TargetRow;
		}

		// Every row from TargetRow up to the top is empty now: drop in new pieces.
		// Refills pick any color, so they can start a cascade.
		for (int32 Row = TargetRow; Row >= 0; --Row)
		{
			const FGridCoord Coord(Row, Col);
			const EPieceColor Color = static_cast<EPieceColor>(FMath::RandRange(0, Config->NumColors - 1));
			if (APuzzlePieceBase* NewPiece = SpawnPiece(Coord, Color, RollPowerUp(), true))
			{
				NewPiece->MoveToCoord(Coord, GridToWorld(Coord), Fall);
			}
		}
	}

	// When the fall is over, check the board again: this is the cascade loop.
	GetWorldTimerManager().SetTimer(StepTimer, this, &AMatch3Board::ResolveStep, Fall);
}

void AMatch3Board::FinishTurn()
{
	// Win and lose are checked only here, once the board has settled.
	if (AMatch3GameMode* GameMode = GetMatch3GameMode(this))
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