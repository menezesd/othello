import pygame
import sys
import copy
from typing import List, Tuple, Optional

# --- Constants ---
# Game settings
BOARD_DIM = 8  # Playable board dimension (8x8)
ARRAY_DIM = BOARD_DIM + 2  # Internal array dimension including border (10x10)
PLY_DEPTH = 9  # AI search depth 

# Colors
COLOR_BOARD_BG = (0, 128, 0)  # Green
COLOR_GRID = (0, 0, 0)      # Black
COLOR_BLACK_PIECE = (0, 0, 0)
COLOR_WHITE_PIECE = (255, 255, 255)
COLOR_HIGHLIGHT = (255, 255, 0, 128) # Yellowish highlight for valid moves

# Board state values
EMPTY = 0
BLACK = 1
WHITE = 2
OUTER = 3  # Border squares

# Screen dimensions
SCREEN_WIDTH = 640
SCREEN_HEIGHT = 480
SQUARE_WIDTH = SCREEN_HEIGHT // BOARD_DIM
BOARD_OFFSET_X = (SCREEN_WIDTH - (BOARD_DIM * SQUARE_WIDTH)) // 2
BOARD_OFFSET_Y = 0

# AI evaluation values
WINNING_VALUE = 32767
LOSING_VALUE = -32767

# Directions for checking flips (relative 1D array indices)
ALL_DIRECTIONS = [-ARRAY_DIM - 1, -ARRAY_DIM, -ARRAY_DIM + 1,
                  -1,                      1,
                   ARRAY_DIM - 1,  ARRAY_DIM,  ARRAY_DIM + 1] # [-11, -10, -9, -1, 1, 9, 10, 11]

# Positional weights for AI evaluation (indices match the 10x10 board)
# fmt: off
POSITIONAL_WEIGHTS = [
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0, 120, -20,  20,   5,   5,  20, -20, 120,   0,
    0, -20, -40,  -5,  -5,  -5,  -5, -40, -20,   0,
    0,  20,  -5,  15,   3,   3,  15,  -5,  20,   0,
    0,   5,  -5,   3,   3,   3,   3,  -5,   5,   0,
    0,   5,  -5,   3,   3,   3,   3,  -5,   5,   0,
    0,  20,  -5,  15,   3,   3,  15,  -5,  20,   0,
    0, -20, -40,  -5,  -5,  -5,  -5, -40, -20,   0,
    0, 120, -20,  20,   5,   5,  20, -20, 120,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
]
# fmt: on

Board = List[int] # Type alias for board representation

# --- Helper Functions ---
def opponent(player: int) -> int:
    """Returns the opponent of the given player."""
    return WHITE if player == BLACK else BLACK

def to_board_index(row: int, col: int) -> int:
    """Converts 0-based row/col to 1D board array index."""
    if 0 <= row < BOARD_DIM and 0 <= col < BOARD_DIM:
        return (row + 1) * ARRAY_DIM + (col + 1)
    return -1 # Invalid index

def to_row_col(index: int) -> Tuple[int, int]:
    """Converts 1D board array index to 0-based row/col."""
    if 1 <= index // ARRAY_DIM <= BOARD_DIM and 1 <= index % ARRAY_DIM <= BOARD_DIM:
        return index // ARRAY_DIM - 1, index % ARRAY_DIM - 1
    return -1, -1 # Invalid index

# --- Game Logic Class ---
class OthelloGame:
    """Manages the state and rules of an Othello game."""

    def __init__(self):
        """Initializes the game board and sets the starting player."""
        self.board: Board = self._create_initial_board()
        self.current_player: int = BLACK
        self.winner: Optional[int] = None # None if game ongoing, EMPTY for draw

    def _create_initial_board(self) -> Board:
        """Creates the standard Othello starting board."""
        board: Board = [OUTER] * (ARRAY_DIM * ARRAY_DIM)
        for r in range(BOARD_DIM):
            for c in range(BOARD_DIM):
                board[to_board_index(r, c)] = EMPTY

        # Initial pieces
        mid_low = BOARD_DIM // 2 - 1
        mid_high = BOARD_DIM // 2
        board[to_board_index(mid_low, mid_low)] = WHITE # Standard Othello starts with white upper-left
        board[to_board_index(mid_high, mid_high)] = WHITE
        board[to_board_index(mid_low, mid_high)] = BLACK
        board[to_board_index(mid_high, mid_low)] = BLACK
        return board

    def get_valid_moves(self, player: int) -> List[int]:
        """Returns a list of valid move indices for the given player."""
        return [
            idx for idx in range(len(self.board))
            if self.board[idx] == EMPTY and self._is_legal_move(idx, player)
        ]

    def _is_legal_move(self, move_index: int, player: int) -> bool:
        """Checks if a move is legal without considering if the square is empty."""
        if self.board[move_index] != EMPTY: # Optimization: check this in get_valid_moves
            return False
        # Check if any direction yields flips
        for direction in ALL_DIRECTIONS:
            if self._find_bracketing_piece(move_index + direction, player, direction):
                return True
        return False

    def _find_bracketing_piece(self, current_index: int, player: int, direction: int) -> Optional[int]:
        """
        Recursively searches in a direction for a piece of the player's color,
        passing over opponent's pieces. Returns the index of the bracketing piece
        or None if no bracket is found.
        """
        if current_index < 0 or current_index >= len(self.board) or self.board[current_index] == OUTER:
            return None # Off board
        if self.board[current_index] == player:
            return current_index # Found the bracketing piece
        if self.board[current_index] == EMPTY:
            return None # Gap found
        # It's an opponent's piece, keep searching
        return self._find_bracketing_piece(current_index + direction, player, direction)

    def make_move(self, move_index: int, player: int) -> bool:
        """
        Attempts to make a move for the player.
        Returns True if the move was successful, False otherwise.
        Updates the board and current player.
        """
        if player != self.current_player:
            print(f"Warning: Trying to move for player {player}, but it's {self.current_player}'s turn.")
            return False

        valid_moves = self.get_valid_moves(player)
        if move_index not in valid_moves:
            return False # Illegal move

        # Place the piece
        self.board[move_index] = player

        # Flip opponent's pieces
        for direction in ALL_DIRECTIONS:
            bracketing_index = self._find_bracketing_piece(move_index + direction, player, direction)
            if bracketing_index:
                pos = move_index + direction
                while pos != bracketing_index:
                    if 0 <= pos < len(self.board): # Bounds check for safety
                       self.board[pos] = player
                    pos += direction

        # Switch player and check game end
        self._update_player_and_check_end()
        return True

    def _update_player_and_check_end(self):
        """Switches player and determines if the game has ended."""
        next_player = opponent(self.current_player)
        if self.get_valid_moves(next_player):
            self.current_player = next_player
        elif self.get_valid_moves(self.current_player):
            # Opponent has no moves, current player plays again
            print(f"Player {next_player} has no moves. Player {self.current_player} plays again.")
            pass # Keep current player
        else:
            # Neither player has moves - game over
            self.current_player = EMPTY # Sentinel for game over
            self.winner = self._determine_winner()
            print("Game Over!")

    def is_game_over(self) -> bool:
        """Checks if the game has ended."""
        return self.current_player == EMPTY

    def get_score(self) -> Tuple[int, int]:
        """Returns the score (black_count, white_count)."""
        black_score = 0
        white_score = 0
        for r in range(BOARD_DIM):
            for c in range(BOARD_DIM):
                idx = to_board_index(r, c)
                if self.board[idx] == BLACK:
                    black_score += 1
                elif self.board[idx] == WHITE:
                    white_score += 1
        return black_score, white_score

    def _determine_winner(self) -> int:
        """Determines the winner based on the final score."""
        black_score, white_score = self.get_score()
        if black_score > white_score:
            return BLACK
        elif white_score > black_score:
            return WHITE
        else:
            return EMPTY # Draw

    @staticmethod
    def evaluate_board(board: Board, player: int) -> int:
        """
        Static evaluation function using positional weights.
        Higher value is better for the 'player'.
        """
        score = 0
        opp = opponent(player)
        for i in range(len(board)):
            if board[i] == player:
                score += POSITIONAL_WEIGHTS[i]
            elif board[i] == opp:
                score -= POSITIONAL_WEIGHTS[i]
        return score

    @staticmethod
    def final_value(board: Board, player: int) -> int:
        """
        Determines the game's final value (win/loss/draw) for the player.
        """
        black_score = board.count(BLACK)
        white_score = board.count(WHITE)
        diff = black_score - white_score if player == BLACK else white_score - black_score

        if diff > 0:
            return WINNING_VALUE
        elif diff < 0:
            return LOSING_VALUE
        else:
            return 0 # Draw

# --- AI Player Class ---
class AIPlayer:
    """Handles the AI's move selection using minimax with alpha-beta pruning."""

    def __init__(self, ai_player: int, depth: int = PLY_DEPTH):
        self.ai_player = ai_player
        self.depth = depth

    def get_best_move(self, game: OthelloGame) -> Optional[int]:
        """
        Calculates and returns the best move index for the AI player.
        Returns None if no valid moves are available.
        """
        # AI player must match the game's current player
        if game.current_player != self.ai_player:
             print(f"Error: AI ({self.ai_player}) trying to move on opponent's ({game.current_player}) turn.")
             return None # Should not happen if logic is correct

        print(f"AI ({self.ai_player}) is thinking...")
        score, best_move = self._alphabeta(
            game.board, # Pass a copy of the board state
            self.ai_player,
            LOSING_VALUE - 1,
            WINNING_VALUE + 1,
            self.depth
        )
        print(f"AI chooses move index {best_move} with predicted score {score}")
        return best_move


    def _alphabeta(self, board: Board, player: int, alpha: int, beta: int, depth: int) -> Tuple[int, Optional[int]]:
        """
        Minimax algorithm with alpha-beta pruning.
        Returns the best score and the corresponding move index.
        Operates on a copy of the board state.
        """
        # Base case: maximum depth reached or game potentially over
        if depth == 0:
            return OthelloGame.evaluate_board(board, player), None

        valid_moves = self._get_valid_moves_for_board(board, player)

        # Check if game is over or player must pass
        if not valid_moves:
            opp = opponent(player)
            if not self._get_valid_moves_for_board(board, opp):
                # Game over
                return OthelloGame.final_value(board, player), None
            else:
                # Player must pass turn
                # Invert score as we are evaluating from the opponent's perspective
                score, _ = self._alphabeta(board, opp, -beta, -alpha, depth - 1)
                return -score, None # No move made by current player

        best_move: Optional[int] = None
        best_score = LOSING_VALUE - 1 # Initialize with a very low score

        for move in valid_moves:
            # Create a temporary board state for this move
            temp_board = self._make_move_on_board(board, move, player)

            # Recursive call for the opponent
            score, _ = self._alphabeta(temp_board, opponent(player), -beta, -alpha, depth - 1)
            score = -score # Negate score from opponent's perspective

            # Update best score and move
            if score > best_score:
                best_score = score
                best_move = move

            # Alpha-beta pruning
            if best_score > alpha:
                alpha = best_score
            if alpha >= beta:
                break # Prune remaining branches

        return best_score, best_move

    # --- Helper methods for AI that operate on arbitrary boards ---
    # These mirror methods from OthelloGame but take the board as an argument

    def _get_valid_moves_for_board(self, board: Board, player: int) -> List[int]:
        """Finds valid moves for a player on a given board state."""
        return [
            idx for idx in range(len(board))
            if board[idx] == EMPTY and self._is_legal_move_on_board(board, idx, player)
        ]

    def _is_legal_move_on_board(self, board: Board, move_index: int, player: int) -> bool:
        """Checks legality on a given board state (assumes square is empty)."""
        for direction in ALL_DIRECTIONS:
            if self._find_bracketing_piece_on_board(board, move_index + direction, player, direction):
                return True
        return False

    def _find_bracketing_piece_on_board(self, board: Board, current_index: int, player: int, direction: int) -> Optional[int]:
        """Finds bracketing piece on a given board state."""
        if current_index < 0 or current_index >= len(board) or board[current_index] == OUTER:
            return None
        if board[current_index] == player:
            return current_index
        if board[current_index] == EMPTY:
            return None
        return self._find_bracketing_piece_on_board(board, current_index + direction, player, direction)

    def _make_move_on_board(self, board: Board, move_index: int, player: int) -> Board:
        """Applies a move to a copy of the board and returns the new board."""
        new_board = board[:] # Create a shallow copy
        new_board[move_index] = player
        for direction in ALL_DIRECTIONS:
            bracketing_index = self._find_bracketing_piece_on_board(new_board, move_index + direction, player, direction)
            if bracketing_index:
                pos = move_index + direction
                while pos != bracketing_index:
                    if 0 <= pos < len(new_board):
                        new_board[pos] = player
                    pos += direction
        return new_board


# --- Drawing Functions ---
def draw_board(screen: pygame.Surface, game: OthelloGame):
    """Draws the Othello board and pieces."""
    screen.fill(COLOR_BOARD_BG)

    # Draw grid lines
    for i in range(BOARD_DIM + 1):
        x = BOARD_OFFSET_X + i * SQUARE_WIDTH
        pygame.draw.line(screen, COLOR_GRID, (x, BOARD_OFFSET_Y), (x, BOARD_OFFSET_Y + BOARD_DIM * SQUARE_WIDTH))
        y = BOARD_OFFSET_Y + i * SQUARE_WIDTH
        pygame.draw.line(screen, COLOR_GRID, (BOARD_OFFSET_X, y), (BOARD_OFFSET_X + BOARD_DIM * SQUARE_WIDTH, y))

    # Draw pieces
    piece_radius = SQUARE_WIDTH // 2 - 5
    for r in range(BOARD_DIM):
        for c in range(BOARD_DIM):
            idx = to_board_index(r, c)
            piece = game.board[idx]
            if piece in [BLACK, WHITE]:
                color = COLOR_BLACK_PIECE if piece == BLACK else COLOR_WHITE_PIECE
                center_x = BOARD_OFFSET_X + c * SQUARE_WIDTH + SQUARE_WIDTH // 2
                center_y = BOARD_OFFSET_Y + r * SQUARE_WIDTH + SQUARE_WIDTH // 2
                pygame.draw.circle(screen, color, (center_x, center_y), piece_radius)

def draw_valid_moves(screen: pygame.Surface, valid_moves: List[int]):
    """Highlights valid moves for the current player."""
    highlight_radius = SQUARE_WIDTH // 4
    highlight_surface = pygame.Surface((highlight_radius * 2, highlight_radius * 2), pygame.SRCALPHA)
    pygame.draw.circle(highlight_surface, COLOR_HIGHLIGHT, (highlight_radius, highlight_radius), highlight_radius)

    for move_index in valid_moves:
        r, c = to_row_col(move_index)
        if r != -1: # Check if conversion was valid
            center_x = BOARD_OFFSET_X + c * SQUARE_WIDTH + SQUARE_WIDTH // 2
            center_y = BOARD_OFFSET_Y + r * SQUARE_WIDTH + SQUARE_WIDTH // 2
            screen.blit(highlight_surface, (center_x - highlight_radius, center_y - highlight_radius))

def draw_score(screen: pygame.Surface, game: OthelloGame, font: pygame.font.Font):
    """Displays the current score and winner."""
    black_score, white_score = game.get_score()
    score_text = f"Black: {black_score}  White: {white_score}"
    text_surface = font.render(score_text, True, COLOR_WHITE_PIECE)
    screen.blit(text_surface, (10, SCREEN_HEIGHT - 30))

    if game.is_game_over():
        winner_text = ""
        if game.winner == BLACK:
            winner_text = "Black Wins!"
        elif game.winner == WHITE:
            winner_text = "White Wins!"
        else:
            winner_text = "Draw!"
        win_surface = font.render(winner_text, True, (255, 215, 0)) # Gold color
        win_rect = win_surface.get_rect(center=(SCREEN_WIDTH // 2, SCREEN_HEIGHT - 20))
        screen.blit(win_surface, win_rect)


# --- Main Game Execution ---
def main():
    """Sets up and runs the Othello game loop."""
    pygame.init()
    screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
    pygame.display.set_caption("Othello (Refactored)")
    clock = pygame.time.Clock()
    font = pygame.font.Font(None, 36) # Default font

    game = OthelloGame()
    human_player = BLACK # Or set to WHITE if you want to play as white
    ai_player_color = opponent(human_player)
    ai = AIPlayer(ai_player_color, depth=PLY_DEPTH)

    running = True
    valid_moves_cache = [] # Cache valid moves for drawing

    while running:
        # --- Event Handling ---
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.MOUSEBUTTONDOWN and not game.is_game_over():
                # Handle human move only if it's their turn
                if game.current_player == human_player:
                    mouse_x, mouse_y = pygame.mouse.get_pos()
                    # Convert screen coordinates to board coordinates
                    col = (mouse_x - BOARD_OFFSET_X) // SQUARE_WIDTH
                    row = (mouse_y - BOARD_OFFSET_Y) // SQUARE_WIDTH

                    move_index = to_board_index(row, col)

                    if move_index != -1:
                        if game.make_move(move_index, human_player):
                            print(f"Human ({human_player}) moved to ({row}, {col}) index {move_index}")
                            valid_moves_cache = [] # Clear cache after move
                        else:
                            print(f"Invalid move at ({row}, {col})")
                    else:
                         print("Clicked outside board")

        # --- AI Turn ---
        if not game.is_game_over() and game.current_player == ai_player_color:
            ai_move = ai.get_best_move(game)
            if ai_move is not None:
                if game.make_move(ai_move, ai_player_color):
                     r, c = to_row_col(ai_move)
                     print(f"AI ({ai_player_color}) moved to ({r}, {c}) index {ai_move}")
                     valid_moves_cache = [] # Clear cache after move
                else:
                     # This case should ideally not happen if AI chooses from valid moves
                     print(f"Error: AI ({ai_player_color}) chose an invalid move {ai_move}")
                     # As a fallback, let AI pass if stuck (though get_best_move should handle passes)
                     game._update_player_and_check_end()

            else:
                # AI has no move (should be handled by pass logic in make_move/update_player)
                print(f"AI ({ai_player_color}) has no moves. Passing turn.")
                # Explicitly update player status if get_best_move returns None unexpectedly
                game._update_player_and_check_end()


        # --- Update valid moves cache if needed ---
        if not game.is_game_over() and game.current_player == human_player and not valid_moves_cache:
             valid_moves_cache = game.get_valid_moves(human_player)

        # --- Drawing ---
        draw_board(screen, game)
        if not game.is_game_over() and game.current_player == human_player:
            draw_valid_moves(screen, valid_moves_cache)
        draw_score(screen, game, font)
        pygame.display.flip()

        # --- Frame Rate ---
        clock.tick(30) # Limit FPS

    pygame.quit()
    sys.exit()

if __name__ == "__main__":
    main()
