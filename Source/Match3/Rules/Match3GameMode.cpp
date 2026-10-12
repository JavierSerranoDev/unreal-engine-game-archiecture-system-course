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
	// TODO: implement (diagram 12).
	// Needs a ready power-up and an Idle board. Find its FPowerUpEntry in LevelConfig,
	// NewObject the EffectClass, and call Board->BeginTargeting(Type, Effect, Player).
	return false;
}

bool AMatch3GameMode::ConsumeMove()
{
	AMatch3GameState* GS = GetMatch3GameState();
	if (!GS || GS->GetMovesRemaining() <= 0)
	{
		return false;
	}

	GS->DecrementMoves();
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
	// TODO: implement (diagram 13).
	// Find the player's AMatch3PlayerState and call AddPowerUp(Type) on its inventory.
}

void AMatch3GameMode::HandlePowerUpConfirmed(AMatch3PlayerState* Player, int32 CellCount)
{
	// TODO: implement (diagram 12). ConsumeReady on Player's inventory, then AwardBlast(CellCount).
}

void AMatch3GameMode::EvaluateEndOfTurn()
{
	AMatch3GameState* GS = GetMatch3GameState();
	if (!GS)
	{
		return;
	}

	GS->SetComboLevel(0);

	// Win is checked first: reaching the target on the last move is a win.
	if (GS->GetScore() >= GS->GetTargetScore())
	{
		EndGame(true);
	}
	else if (GS->GetMovesRemaining() <= 0)
	{
		EndGame(false);
	}
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