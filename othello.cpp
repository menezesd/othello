#include <SDL2/SDL.h>
#include <vector>
#include <algorithm>
#include <climits>

const int EMPTY = 0;
const int BLACK = 1;
const int WHITE = 2;
const int OUTER = 3;
const int WinningValue = 32767;
const int LosingValue = -32767;
const int nply = 5;
const int SquareWidth = 60;
const int tlx = (640 - 480) / 2;
const int tly = 0;
const int AllDirections[8] = {-11, -10, -9, -1, 1, 9, 10, 11};
const int weights[100] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 120, -20, 20, 5, 5, 20, -20, 120, 0,
    0, -20, -40, -5, -5, -5, -5, -40, -20, 0,
    0, 20, -5, 15, 3, 3, 15, -5, 20, 0,
    0, 5, -5, 3, 3, 3, 3, -5, 5, 0,
    0, 5, -5, 3, 3, 3, 3, -5, 5, 0,
    0, 20, -5, 15, 3, 3, 15, -5, 20, 0,
    0, -20, -40, -5, -5, -5, -5, -40, -20, 0,
    0, 120, -20, 20, 5, 5, 20, -20, 120, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
int board[100];
std::vector<int> bestm(nply+1);

int opponent(int player) {
    return (player == BLACK) ? WHITE : BLACK;
}

void initBoard() {
    for(int i = 0; i < 100; i++) {
        if(i < 10 || i >= 90 || i%10 == 0 || i%10 == 9)
            board[i] = OUTER;
        else
            board[i] = EMPTY;
    }
    board[44] = BLACK;
    board[45] = WHITE;
    board[54] = WHITE;
    board[55] = BLACK;
}

void showBoard() {
    SDL_SetRenderDrawColor(renderer, 0, 128, 0, 255);
    SDL_RenderClear(renderer);
    
    // Draw grid
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    for(int i = 0; i <= 8; i++) {
        SDL_RenderDrawLine(renderer, tlx + i*SquareWidth, tly, 
                         tlx + i*SquareWidth, tly + 480);
        SDL_RenderDrawLine(renderer, tlx, tly + i*SquareWidth,
                         tlx + 480, tly + i*SquareWidth);
    }
    
    // Draw pieces
    for(int y = 1; y <= 8; y++) {
        for(int x = 1; x <= 8; x++) {
            int pos = y*10 + x;
            if(board[pos] == EMPTY) continue;
            
            SDL_Color color = (board[pos] == BLACK) ? 
                SDL_Color{0,0,0,255} : SDL_Color{255,255,255,255};
            
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            SDL_Rect piece = {
                tlx + x*SquareWidth - SquareWidth/2 - 25,
                tly + y*SquareWidth - SquareWidth/2 - 25,
                50, 50
            };
            SDL_RenderFillRect(renderer, &piece);
        }
    }
    SDL_RenderPresent(renderer);
}

int getMove() {
    SDL_Event e;
    while(true) {
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) exit(0);
            if(e.type == SDL_MOUSEBUTTONDOWN) {
                int x, y;
                SDL_GetMouseState(&x, &y);
                int col = (x - tlx)/SquareWidth + 1;
                int row = (y - tly)/SquareWidth + 1;
                if(col >= 1 && col <= 8 && row >= 1 && row <= 8)
                    return row*10 + col;
            }
        }
        SDL_Delay(10);
    }
}

bool legalMove(int move, int player) {
    if(board[move] != EMPTY) return false;
    for(int dir : AllDirections) {
        int pos = move + dir;
        if(board[pos] != opponent(player)) continue;
        while(true) {
            pos += dir;
            if(board[pos] == player) return true;
            if(board[pos] == EMPTY || board[pos] == OUTER) break;
        }
    }
    return false;
}

int findBracketingPiece(int square, int player, int dir) {
    if(board[square] == player) return square;
    if(board[square] == opponent(player))
        return findBracketingPiece(square + dir, player, dir);
    return 0;
}

void makeFlips(int move, int player, int dir) {
    int bracketer = findBracketingPiece(move + dir, player, dir);
    if(bracketer) {
        for(int pos = move + dir; pos != bracketer; pos += dir)
            board[pos] = player;
    }
}

void makeMove(int move, int player) {
    board[move] = player;
    for(int dir : AllDirections)
        makeFlips(move, player, dir);
}

int alphabeta(int player, int alpha, int beta, int ply) {
    if(ply == 0) {
        int sum = 0;
        for(int i = 0; i < 100; i++) {
            if(board[i] == player) sum += weights[i];
            if(board[i] == opponent(player)) sum -= weights[i];
        }
        return sum;
    }
    
    std::vector<int> moves;
    for(int move = 0; move < 100; move++) {
        if(legalMove(move, player)) moves.push_back(move);
    }
    
    if(moves.empty()) {
        if(std::any_of(board, board+100, [&](int pos){ 
            return legalMove(pos, opponent(player)); 
        })) {
            return -alphabeta(opponent(player), -beta, -alpha, ply-1);
        }
        int diff = 0;
        for(int pos : board) diff += (pos == player) - (pos == opponent(player));
        return (diff > 0) ? WinningValue : (diff < 0) ? LosingValue : 0;
    }
    
    int bestVal = INT_MIN;
    int bestMove = -1;
    for(int move : moves) {
        int tempBoard[100];
        std::copy(board, board+100, tempBoard);
        makeMove(move, player);
        int val = -alphabeta(opponent(player), -beta, -alpha, ply-1);
        std::copy(tempBoard, tempBoard+100, board);
        
        if(val > bestVal) {
            bestVal = val;
            bestMove = move;
            if(bestVal > alpha) {
                alpha = bestVal;
                bestm[ply] = bestMove;
            }
            if(alpha >= beta) break;
        }
    }
    return bestVal;
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Othello", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    
    initBoard();
    int player = BLACK;
    int human = BLACK;
    int computer = opponent(human);
    
    while(true) {
        showBoard();
        
        if(player == human) {
            int move = getMove();
            if(legalMove(move, player)) {
                makeMove(move, player);
                player = opponent(player);
            }
        } else {
            alphabeta(player, LosingValue, WinningValue, nply);
            int move = bestm[nply];
            if(move != -1) {
                makeMove(move, player);
                player = opponent(player);
            }
        }
        
        if(!std::any_of(board, board+100, [](int pos){ return pos == EMPTY; }))
            break;
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
