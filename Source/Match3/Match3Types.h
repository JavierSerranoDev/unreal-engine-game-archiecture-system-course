// Match3Types.h
// The shared vocabulary of the game: enums, small structs and every delegate.
// Both C++ and Blueprints use these, so everything is BlueprintType.
// See "2. Shared types and delegates" in the architecture document.

#pragma once

#include "CoreMinimal.h"
#include "Match3Types.generated.h"

class UPowerUpEffect;
class UMaterialInterface;
class UTexture2D;

// ---------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------

/** The four piece colors. NumColors in the level config picks how many are used. */
UENUM(BlueprintType)
enum class EPieceColor : uint8
{
	Red,
	Blue,
	Green,
	Yellow
};

/** What a piece carries, and what the inventory stores. None means a plain piece. */
UENUM(BlueprintType)
enum class EPowerUpType : uint8
{
	None,
	RowBlast,
	ColumnBlast,
	ColorBomb,
	XBomb
};

/** The board is always in exactly one of these. Input is accepted only in Idle and Targeting. */
UENUM(BlueprintType)
enum class EBoardState : uint8
{
	Initializing,
	Idle,
	Swapping,
	Resolving,
	Targeting,
	GameOver
};

/** The four swipe directions. The dominant axis of the drag picks one. */
UENUM(BlueprintType)
enum class ESwipeDirection : uint8
{
	Up,
	Down,
	Left,
	Right
};

/** How a power-up picks its cells. Used by the presentation layer for prompts and previews. */
UENUM(BlueprintType)
enum class ETargetMode : uint8
{
	Row,
	Column,
	Color,
	Cell
};

// ---------------------------------------------------------------------------
// Structs
// ---------------------------------------------------------------------------

/**
 * A cell on the board.
 * Row 0 is the top row and rows grow downward, the same way screen Y does.
 * That is why Up means Row - 1 and pieces fall toward higher rows.
 */
USTRUCT(BlueprintType)
struct FGridCoord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 Row = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 Col = 0;

	FGridCoord() = default;
	FGridCoord(int32 InRow, int32 InCol) : Row(InRow), Col(InCol) {}

	/** True when this cell is inside a board of Rows x Cols. */
	bool IsValid(int32 Rows, int32 Cols) const
	{
		return Row >= 0 && Row < Rows && Col >= 0 && Col < Cols;
	}

	/** The cell one step away in Dir. It may be off the board, so check IsValid after. */
	FGridCoord Neighbor(ESwipeDirection Dir) const
	{
		switch (Dir)
		{
		case ESwipeDirection::Up:    return FGridCoord(Row - 1, Col);
		case ESwipeDirection::Down:  return FGridCoord(Row + 1, Col);
		case ESwipeDirection::Left:  return FGridCoord(Row, Col - 1);
		case ESwipeDirection::Right: return FGridCoord(Row, Col + 1);
		}
		return *this;
	}

	bool operator==(const FGridCoord& Other) const
	{
		return Row == Other.Row && Col == Other.Col;
	}

	bool operator!=(const FGridCoord& Other) const
	{
		return !(*this == Other);
	}

	/** Lets FGridCoord be used in TSet and TMap, for example to merge overlapping matches. */
	friend uint32 GetTypeHash(const FGridCoord& Coord)
	{
		return HashCombineFast(::GetTypeHash(Coord.Row), ::GetTypeHash(Coord.Col));
	}
};

/** One run of 3 or more same-colored pieces in a row or a column. FindMatches produces these. */
USTRUCT(BlueprintType)
struct FMatchResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Match")
	TArray<FGridCoord> Cells;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Match")
	EPieceColor Color = EPieceColor::Red;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Match")
	bool bHorizontal = true;

	int32 Count() const { return Cells.Num(); }
};

/** How one color looks. Read by BP_PuzzlePiece when it is initialized. */
USTRUCT(BlueprintType)
struct FPieceColorVisual
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	EPieceColor Color = EPieceColor::Red;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UMaterialInterface> Material = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FLinearColor Tint = FLinearColor::White;
};

/**
 * Everything about one power-up type: the effect class that picks its cells,
 * plus the icon and name the HUD shows. The inventory only stores the Type.
 */
USTRUCT(BlueprintType)
struct FPowerUpEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PowerUp")
	EPowerUpType Type = EPowerUpType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PowerUp")
	TSubclassOf<UPowerUpEffect> EffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PowerUp")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PowerUp")
	FText DisplayName;
};

// ---------------------------------------------------------------------------
// Delegates
// Dynamic multicast so widgets and controllers can bind to them in Blueprint.
// C++ broadcasts them; Blueprints only listen.
// ---------------------------------------------------------------------------

// AMatch3GameState
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScoreChanged, int32, NewScore, int32, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMovesChanged, int32, MovesRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComboChanged, int32, ComboLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGameOver, bool, bWon, int32, FinalScore);

// UPowerUpInventoryComponent
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryChanged, const TArray<EPowerUpType>&, Queue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPowerUpDiscarded, EPowerUpType, Type);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPowerUpActivated, EPowerUpType, Type);

// AMatch3Board
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBoardStateChanged, EBoardState, OldState, EBoardState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTargetingStarted, EPowerUpType, Type, ETargetMode, Mode, FText, Prompt);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetingEnded, bool, bConfirmed);
