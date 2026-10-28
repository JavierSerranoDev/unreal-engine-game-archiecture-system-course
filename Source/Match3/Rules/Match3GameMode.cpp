// Match3GameMode.cpp

#include "Rules/Match3GameMode.h"
#include "GameFramework/PlayerController.h"
#include "Board/Match3Board.h"
#include "Rules/Match3GameState.h"
#include "Rules/Match3LevelConfig.h"
#include "Rules/Match3PlayerState.h"
#include "PowerUps/PowerUpEffect.h"
#include "Rules/PowerUpInventoryComponent.h"

#include "Engine/World.h"

AMatch3GameMode::AMatch3GameMode()
{
	// C++ defaults. BP_Match3GameMode adds the controller, board and level config.
	GameStateClass = AMatch3GameState::StaticClass();
	PlayerStateClass = AMatch3PlayerState::StaticClass();
	DefaultPawnClass = nullptr; // The camera lives on the board; there is no pawn.
}

void AMatch3GameMode::StartPlay()
{
	AMatch3GameState* GS = GetMatch3GameState();
	if (!GS || !BoardClass || !LevelConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("AMatch3GameMode: set LevelConfig and BoardClass in BP_Match3GameMode, and keep AMatch3GameState as the GameState class."));
		Super::StartPlay();
		return;
	}

	GS->InitMatch(LevelConfig->MaxMoves, LevelConfig->TargetScore);

	// The GameMode spawns the board, so nobody has to place it in the level by hand.
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Owner = this;
	Board = GetWorld()->SpawnActor<AMatch3Board>(BoardClass, FTransform::Identity, SpawnInfo);
	if (Board)
	{
		Board->InitializeBoard(LevelConfig);
	}

	// Last, so the board and the GameState are ready when every actor gets BeginPlay.
	Super::StartPlay();
}

void AMatch3GameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AMatch3PlayerState* Player = NewPlayer ? NewPlayer->GetPlayerState<AMatch3PlayerState>() : nullptr;
	if (Player && Player->GetInventory() && LevelConfig)
	{
		Player->GetInventory()->SetCapacity(LevelConfig->InventoryCapacity);
	}
}

bool AMatch3GameMode::TryActivateReadyPowerUp(AMatch3PlayerState* Player)
{
	UPowerUpInventoryComponent* Inventory = Player ? Player->GetInventory() : nullptr;
	if (!Inventory || !Inventory->HasReady() || !Board || Board->GetState() != EBoardState::Idle)
	{
		return false;
	}

	// Peek, don't consume: the power-up leaves the queue only when the target is confirmed.
	const EPowerUpType Type = Inventory->PeekReady();
	const FPowerUpEntry Entry = LevelConfig->FindPowerUp(Type);
	if (!Entry.EffectClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("No EffectClass for %s in the level config."), *UEnum::GetValueAsString(Type));
		return false;
	}

	UPowerUpEffect* Effect = NewObject<UPowerUpEffect>(this, Entry.EffectClass);
	return Board->BeginTargeting(Type, Effect, Player);
}

bool AMatch3GameMode::ConsumeMove()
{
	// TODO(Student): Uncomment the solution below, then delete the placeholder return at the end.
	// Spends one move through the GameState.
	/*
	// Step 1: Get the GameState. It holds MovesRemaining, but only the GameMode may change it.
	AMatch3GameState* GS = GetMatch3GameState();

	// Step 2: Refuse the move if there is no GameState or no moves are left.
	//         Returning false tells the board not to perform the swap.
	if (!GS || GS->GetMovesRemaining() <= 0)
	{
		return false;
	}

	// Step 3: Spend the move. DecrementMoves() also broadcasts OnMovesChanged,
	//         which is how the HUD finds out without the GameMode knowing about it.
	GS->DecrementMoves();
	return true;
	*/

	// Placeholder so the starter compiles: swaps keep working, they just cost nothing yet.
	// Delete this line once the solution above is uncommented.
	return true;
}

int32 AMatch3GameMode::AwardMatch(const FMatchResult& Match, int32 ComboLevel)
{
	AMatch3GameState* GS = GetMatch3GameState();
	if (!GS)
	{
		return 0;
	}

	const int32 Points = CalculateMatchScore(Match.Count(), ComboLevel, LevelConfig->BasePoints);
	GS->AddScore(Points);
	GS->SetComboLevel(ComboLevel);
	return Points;
}

int32 AMatch3GameMode::AwardBlast(int32 CellCount)
{
	AMatch3GameState* GS = GetMatch3GameState();
	if (!GS)
	{
		return 0;
	}

	// A blast scores flat per cell: no combo, no length bonus.
	const int32 Points = LevelConfig->BasePoints * CellCount;
	GS->AddScore(Points);
	return Points;
}

void AMatch3GameMode::GrantPowerUp(EPowerUpType Type)
{
	// Single-player game: the power-up always goes to the first (only) player.
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	AMatch3PlayerState* Player = PC ? PC->GetPlayerState<AMatch3PlayerState>() : nullptr;
	if (Player && Player->GetInventory())
	{
		Player->GetInventory()->AddPowerUp(Type);
	}
}

void AMatch3GameMode::HandlePowerUpConfirmed(AMatch3PlayerState* Player, int32 CellCount)
{
	// The power-up leaves the queue only now, on confirm. Cancelling never gets here.
	if (Player && Player->GetInventory())
	{
		Player->GetInventory()->ConsumeReady();
	}
	AwardBlast(CellCount);
}

void AMatch3GameMode::EvaluateEndOfTurn()
{
	// TODO(Student): Uncomment the solution below.
	// Runs once the board settles, after every cascade has finished.
	// Until it is uncommented the game never ends.
	/*
	// Step 1: Get the GameState. Without it there is nothing to check.
	AMatch3GameState* GS = GetMatch3GameState();
	if (!GS)
	{
		return;
	}

	// Step 2: Reset the combo. ComboLevel only counts cascades within one turn.
	GS->SetComboLevel(0);

	// Step 3: Check the win first, so reaching the target on the last move is a win.
	// Step 4: Otherwise, running out of moves is a loss. If neither is true, play goes on.
	if (GS->GetScore() >= GS->GetTargetScore())
	{
		EndGame(true);
	}
	else if (GS->GetMovesRemaining() <= 0)
	{
		EndGame(false);
	}
	*/
}

int32 AMatch3GameMode::CalculateMatchScore(int32 Count, int32 Combo, int32 Base)
{
	return Base * Count * (Count - 2) * Combo;
}

void AMatch3GameMode::EndGame(bool bWon)
{
	// Lock the board first, so no input sneaks in while the game-over screen opens.
	if (Board)
	{
		Board->LockForGameOver();
	}
	if (AMatch3GameState* GS = GetMatch3GameState())
	{
		GS->SetGameOver(bWon);
	}
}

AMatch3GameState* AMatch3GameMode::GetMatch3GameState() const
{
	return GetGameState<AMatch3GameState>();
}