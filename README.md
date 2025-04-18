# othello

# Othello Game (Python/Pygame)

A classic Othello (Reversi) board game implementation using Python and the Pygame library. Play against an AI opponent powered by the Minimax algorithm with Alpha-Beta pruning.

## Features

* Graphical Othello board and pieces rendered using Pygame.
* Full implementation of Othello rules, including capturing opponent pieces.
* Human vs. AI gameplay.
* AI opponent utilizes the Minimax algorithm with Alpha-Beta pruning for move selection.
* AI uses a positional weighting heuristic to evaluate board states.
* Valid moves for the human player are visually highlighted.
* Real-time score display showing the count for Black and White pieces.
* Automatic detection of game end conditions (when neither player has a valid move).
* Clear indication of the winner (Black, White, or Draw) upon game completion.
* Clean, object-oriented code structure (refactored from an initial procedural version).

## Requirements

* Python 3.x (tested with 3.7+)
* Pygame library

## How to Play

1.  **Run the game script** from your terminal.
2.  The game window will open.
3.  By default, you play as **Black**, and the AI plays as **White**.
4.  On your turn, valid moves will be indicated by small, semi-transparent yellow circles.
5.  **Click on one of the highlighted squares** to place your piece. The corresponding opponent pieces will be flipped.
6.  The AI will then automatically calculate and make its move.
7.  The game continues alternating turns. If a player has no valid moves, their turn is skipped.
8.  The game ends when neither player has any valid moves left.
9.  The final score and the winner (or Draw) will be displayed at the bottom of the screen.

## Configuration (Optional)

You can modify the following constants near the top of the `.py` file:

* `HUMAN_PLAYER`: Change this from `BLACK` to `WHITE` in the `main()` function if you want to play as White against a Black AI.
* `PLY_DEPTH`: Adjust the AI's search depth. Higher values mean a stronger (but slower) AI. Lower values mean a weaker (but faster) AI. The default is reasonably challenging. Values above 6 or 7 can become very slow depending on your hardware.
* Colors and screen dimensions can also be adjusted in the constants section if desired.

## Code Structure

* **Constants:** Defines game parameters, colors, board states, dimensions, and AI settings.
* **Helper Functions:** Utility functions like `opponent()`, `to_board_index()`, `to_row_col()`.
* **`OthelloGame` class:** Encapsulates the game board, current player, rules logic (checking/making moves, determining winner, calculating score).
* **`AIPlayer` class:** Contains the AI logic, including the `alphabeta` search algorithm and board evaluation methods.
* **Drawing Functions:** `draw_board()`, `draw_valid_moves()`, `draw_score()` handle rendering the game state using Pygame.
* **`main()` function:** Initializes Pygame, creates game objects, runs the main game loop, handles events, and manages player/AI turns.


Files: 
OTHELLO.BAS: original Othello game for QuickBASIC
OTHELLO_FB.BAS: othello game for FreeBASIC
othello.py: Python version using pygame library for graphics

