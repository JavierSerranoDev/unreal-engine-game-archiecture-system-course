// Match3GameState.cpp
// Every setter changes one value and then broadcasts it, so the HUD never has to poll.

#include "Rules/Match3GameState.h"

float AMatch3GameState::GetScoreProgress() const
{
	if (TargetScore <= 0)
	{
		return 0.f;
	}
	return FMath::Clamp(static_cast<float>(Score) / static_cast<float>(TargetScore), 0.f, 1.f);
}

void AMatch3GameState::InitMatch(int32 MaxMoves, int32 Target)
{
	Score = 0;
	MovesRemaining = MaxMoves;
	TargetScore = Target;
	ComboLevel = 0;
	bGameOver = false;
	bPlayerWon = false;

	// Nobody is bound yet at startup (the HUD reads the starting values in RefreshAll),
	// but broadcasting keeps a mid-game restart correct too.
	OnScoreChanged.Broadcast(Score, 0);
	OnMovesChanged.Broadcast(MovesRemaining);
	OnComboChanged.Broadcast(ComboLevel);
}

void AMatch3GameState::AddScore(int32 Delta)
{
	if (Delta == 0)
	{
		return;
	}
	Score += Delta;
	OnScoreChanged.Broadcast(Score, Delta);
}

void AMatch3GameState::DecrementMoves()
{
	MovesRemaining = FMath::Max(0, MovesRemaining - 1);
	OnMovesChanged.Broadcast(MovesRemaining);
}

void AMatch3GameState::SetComboLevel(int32 Level)
{
	if (ComboLevel == Level)
	{
		return;
	}
	ComboLevel = Level;
	OnComboChanged.Broadcast(ComboLevel);
}

void AMatch3GameState::SetGameOver(bool bWon)
{
	if (bGameOver)
	{
		return;
	}
	bGameOver = true;
	bPlayerWon = bWon;
	OnGameOver.Broadcast(bWon, Score);
}