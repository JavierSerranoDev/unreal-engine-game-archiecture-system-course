// Match3GameMode.cpp

#include "Match3GameMode.h"
#include "GameFramework/PlayerController.h"
#include "Match3Board.h"
#include "Match3GameState.h"
#include "Match3LevelConfig.h"
#include "Match3PlayerState.h"
#include "PowerUpEffect.h"
#include "PowerUpInventoryComponent.h"

AMatch3GameMode::AMatch3GameMode()
{
	// C++ defaults. BP_Match3GameMode adds the controller, board and level config.
	GameStateClass = AMatch3GameState::StaticClass();
	PlayerStateClass = AMatch3PlayerState::StaticClass();
	DefaultPawnClass = nullptr; // The camera lives on the board; there is no pawn.
}

void AMatch3GameMode::StartPlay()
{
	// TODO: implement (diagram 9), before calling Super.
	// Check LevelConfig and BoardClass, InitMatch on the GameState with MaxMoves and TargetScore,
	// spawn BoardClass, then InitializeBoard(LevelConfig).

	// Last, so the board and the GameState are ready when every actor gets BeginPlay.
	Super::StartPlay();
}

void AMatch3GameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// TODO: implement.
	// Get the AMatch3PlayerState from NewPlayer and call
	// GetInventory()->SetCapacity(LevelConfig->InventoryCapacity).
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
	// TODO: implement (STUDENT TODO in the starter project).
	// Return false if no moves are left. Otherwise DecrementMoves on the GameState and return true.
	return false;
}

int32 AMatch3GameMode::AwardMatch(const FMatchResult& Match, int32 ComboLevel)
{
	// TODO: implement (diagram 11).
	// CalculateMatchScore with Match.Count() and LevelConfig->BasePoints,
	// then AddScore and SetComboLevel on the GameState. Return the points.
	return 0;
}

int32 AMatch3GameMode::AwardBlast(int32 CellCount)
{
	// TODO: implement (diagram 12). Base x CellCount, AddScore, return the points.
	return 0;
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
	// TODO: implement (STUDENT TODO in the starter project).
	// SetComboLevel(0). Then: Score at or above TargetScore -> EndGame(true);
	// otherwise MovesRemaining is 0 -> EndGame(false). Win is checked first.
}

int32 AMatch3GameMode::CalculateMatchScore(int32 Count, int32 Combo, int32 Base)
{
	return Base * Count * (Count - 2) * Combo;
}

void AMatch3GameMode::EndGame(bool bWon)
{
	// TODO: implement (diagram 11). Board->LockForGameOver(), then SetGameOver(bWon) on the GameState.
}

AMatch3GameState* AMatch3GameMode::GetMatch3GameState() const
{
	return GetGameState<AMatch3GameState>();
}
