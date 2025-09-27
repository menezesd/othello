#include <SDL2/SDL.h>
#include <vector>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <array>

const int WinningValue = 32767;
const int LosingValue = -32767;
const int nply = 5;
const int SquareWidth = 60;
const int tlx = (640 - 480) / 2;
const int tly = 0;

// Transposition table entry
struct TTEntry {
    int value;
    int depth;
    int bestMove;
    enum Flag { EXACT, LOWER_BOUND, UPPER_BOUND } flag;
    
    TTEntry() : value(0), depth(0), bestMove(-1), flag(EXACT) {}
    TTEntry(int v, int d, int move, Flag f) : value(v), depth(d), bestMove(move), flag(f) {}
};

// TT Key structure that includes player-to-move
struct TTKey {
    std::array<int, 100> board;
    int player; // BLACK/WHITE
    
    bool operator==(const TTKey& other) const noexcept {
        return player == other.player && board == other.board;
    }
};

// Hash function for TT key
struct TTKeyHash {
    std::size_t operator()(const TTKey& key) const noexcept {
        std::size_t hash = 1469598103934665603ull;
        for(int value : key.board) {
            hash ^= std::hash<int>{}(value);
            hash *= 1099511628211ull;
        }
        hash ^= std::hash<int>{}(key.player);
        hash *= 1099511628211ull;
        return hash;
    }
};

class TranspositionTable {
private:
    std::unordered_map<TTKey, TTEntry, TTKeyHash> table;
    
public:
    void store(const std::array<int, 100>& board, int player, int value, int depth, int bestMove, TTEntry::Flag flag) {
        auto& slot = table[{board, player}];
        // Only replace if deeper or equal depth (depth-preferred replacement)
        if(depth >= slot.depth) {
            slot = TTEntry(value, depth, bestMove, flag);
        }
    }
    
    bool lookup(const std::array<int, 100>& board, int player, int depth, int alpha, int beta, int& value, int& bestMove) {
        auto it = table.find(TTKey{board, player});
        if(it == table.end()) return false;
        
        const TTEntry& entry = it->second;
        if(entry.depth < depth) return false;
        
        bestMove = entry.bestMove;
        
        switch(entry.flag) {
            case TTEntry::EXACT:
                value = entry.value;
                return true;
            case TTEntry::LOWER_BOUND:
                if(entry.value >= beta) {
                    value = entry.value;
                    return true;
                }
                break;
            case TTEntry::UPPER_BOUND:
                if(entry.value <= alpha) {
                    value = entry.value;
                    return true;
                }
                break;
        }
        return false;
    }
    
    void clear() {
        table.clear();
    }
    
    size_t size() const {
        return table.size();
    }
};

class TimeManager {
private:
    std::chrono::time_point<std::chrono::steady_clock> startTime;
    std::chrono::milliseconds timeLimit;
    bool timeLimitEnabled;
    
public:
    TimeManager() : timeLimit(2000), timeLimitEnabled(true) {} // Default 2 seconds
    
    void startTimer() {
        startTime = std::chrono::steady_clock::now();
    }
    
    void setTimeLimit(int milliseconds) {
        timeLimit = std::chrono::milliseconds(milliseconds);
    }
    
    void enableTimeLimit(bool enable) {
        timeLimitEnabled = enable;
    }
    
    bool timeUp() const {
        if (!timeLimitEnabled) return false;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
        return elapsed >= timeLimit;
    }
    
    int getElapsedMs() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
        return elapsed.count();
    }
    
    int getRemainingMs() const {
        if (!timeLimitEnabled) return INT_MAX;
        return std::max(0, (int)(timeLimit.count() - getElapsedMs()));
    }
};

// Helper function to count flips for move ordering
int countFlipsForMove(const class OthelloBoard& board, int move, int player);

class OthelloBoard {
public:
    static const int EMPTY = 0;
    static const int BLACK = 1;
    static const int WHITE = 2;
    static const int OUTER = 3;
    static const int AllDirections[8];
    static const int weights[100];
    int board[100];

    OthelloBoard() { initBoard(); }

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

    int opponent(int player) const {
        return (player == BLACK) ? WHITE : BLACK;
    }

    bool legalMove(int move, int player) const {
        if(board[move] != EMPTY) return false;
        for(int d = 0; d < 8; ++d) {
            int dir = AllDirections[d];
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

    bool hasLegalMoves(int player) const {
        for(int i = 11; i <= 88; ++i) {
            if(i % 10 == 0 || i % 10 == 9) { 
                i += (i % 10 == 9); // Skip border fast
                continue; 
            }
            if(board[i] == EMPTY && legalMove(i, player)) return true;
        }
        return false;
    }

    int findBracketingPiece(int square, int player, int dir) const {
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
        for(int d = 0; d < 8; ++d)
            makeFlips(move, player, AllDirections[d]);
    }

    // Store flipped positions for undo
    struct UndoInfo {
        int move;
        std::vector<int> flippedPositions;
    };

    std::vector<int> makeFlipsAndRecord(int move, int player, int dir) {
        std::vector<int> flipped;
        int bracketer = findBracketingPiece(move + dir, player, dir);
        if(bracketer) {
            for(int pos = move + dir; pos != bracketer; pos += dir) {
                flipped.push_back(pos);
                board[pos] = player;
            }
        }
        return flipped;
    }

    UndoInfo makeMoveWithUndo(int move, int player) {
        UndoInfo undo;
        undo.move = move;
        board[move] = player;
        
        for(int d = 0; d < 8; ++d) {
            std::vector<int> flipped = makeFlipsAndRecord(move, player, AllDirections[d]);
            undo.flippedPositions.insert(undo.flippedPositions.end(), flipped.begin(), flipped.end());
        }
        
        return undo;
    }

    void unmakeMove(const UndoInfo& undo, int player) {
        board[undo.move] = EMPTY;
        int opponent_player = opponent(player);
        for(int pos : undo.flippedPositions) {
            board[pos] = opponent_player;
        }
    }
    
    // Get board as array for hashing
    std::array<int, 100> getBoardArray() const {
        std::array<int, 100> arr;
        std::copy(board, board + 100, arr.begin());
        return arr;
    }
    
    // Helper methods for advanced evaluation
    int countPieces() const {
        int count = 0;
        for(int i = 11; i <= 88; i++) {
            if(i % 10 != 0 && i % 10 != 9) { // Skip border
                if(board[i] == BLACK || board[i] == WHITE) count++;
            }
        }
        return count;
    }
    
    int countDiscs(int player) const {
        int count = 0;
        for(int i = 11; i <= 88; i++) {
            if(i % 10 != 0 && i % 10 != 9) { // Skip border
                if(board[i] == player) count++;
            }
        }
        return count;
    }
    
    int mobility(int player) const {
        int playerMoves = 0, opponentMoves = 0;
        for(int move = 11; move <= 88; move++) {
            if(move % 10 != 0 && move % 10 != 9) { // Skip border
                if(legalMove(move, player)) playerMoves++;
                if(legalMove(move, opponent(player))) opponentMoves++;
            }
        }
        return (playerMoves - opponentMoves) * 10;
    }
    
    int cornerControl(int player) const {
        int corners[] = {11, 18, 81, 88}; // Corner positions
        int score = 0;
        for(int corner : corners) {
            if(board[corner] == player) score += 100;
            else if(board[corner] == opponent(player)) score -= 100;
        }
        return score;
    }
    
    int edgeControl(int player) const {
        // Edge positions (excluding corners and X-squares)
        int edgePositions[] = {12,13,14,15,16,17, 21,31,41,51,61,71, 28,38,48,58,68,78, 82,83,84,85,86,87};
        int score = 0;
        for(int pos : edgePositions) {
            if(board[pos] == player) score += 5;
            else if(board[pos] == opponent(player)) score -= 5;
        }
        return score;
    }
    
    bool isStableInDirection(int pos, int player, int dir) const {
        // Check if piece is stable in one direction (either blocked by edge or friendly pieces)
        int next = pos + dir;
        while(board[next] == player) {
            next += dir;
        }
        return (board[next] == OUTER); // Reached edge
    }
    
    bool isStable(int pos, int player) const {
        if(board[pos] != player) return false;
        
        // Corner pieces are always stable
        if(pos == 11 || pos == 18 || pos == 81 || pos == 88) return true;
        
        // Check if stable in at least one direction pair
        bool horizontalStable = isStableInDirection(pos, player, -1) || isStableInDirection(pos, player, 1);
        bool verticalStable = isStableInDirection(pos, player, -10) || isStableInDirection(pos, player, 10);
        bool diagonal1Stable = isStableInDirection(pos, player, -11) || isStableInDirection(pos, player, 11);
        bool diagonal2Stable = isStableInDirection(pos, player, -9) || isStableInDirection(pos, player, 9);
        
        return horizontalStable && verticalStable && diagonal1Stable && diagonal2Stable;
    }
    
    int stability(int player) const {
        int stable = 0;
        for(int pos = 11; pos <= 88; pos++) {
            if(pos % 10 != 0 && pos % 10 != 9) { // Skip border
                if(board[pos] == player && isStable(pos, player)) {
                    stable += 10; // Stable pieces are valuable
                } else if(board[pos] == opponent(player) && isStable(pos, opponent(player))) {
                    stable -= 10;
                }
            }
        }
        return stable;
    }
    
    int dangerousSquares(int player) const {
        // X-squares adjacent to corners - only penalize if corner isn't controlled
        const struct { int xSquare; int corner; } dangerousSpots[] = {
            {22, 11}, {12, 11}, {21, 11},  // Corner 11 (top-left)
            {27, 18}, {17, 18}, {28, 18},  // Corner 18 (top-right)
            {72, 81}, {82, 81}, {71, 81},  // Corner 81 (bottom-left)
            {77, 88}, {87, 88}, {78, 88}   // Corner 88 (bottom-right)
        };
        
        int penalty = 0;
        for(const auto& spot : dangerousSpots) {
            // Only penalize X-squares if we don't control the adjacent corner
            if(board[spot.corner] != player) {
                if(board[spot.xSquare] == player) penalty -= 25;
                else if(board[spot.xSquare] == opponent(player)) penalty += 25;
            }
        }
        return penalty;
    }
    
    int parity(int player) const {
        int emptySquares = 64 - countPieces(); // 64 squares on board
        // In endgame, having the last move can be advantageous
        return (emptySquares % 2 == 1) ? 3 : -3; // Odd means we move last
    }
    
    int advancedEvaluation(int player) const {
        int totalPieces = countPieces();
        
        int mobilityScore = mobility(player);
        int cornerScore = cornerControl(player);
        int edgeScore = edgeControl(player);
        int stabilityScore = stability(player);
        int dangerScore = dangerousSquares(player);
        int parityScore = parity(player);
        
        // Weight factors based on game phase
        if(totalPieces <= 20) {
            // Opening: Prioritize mobility, avoid dangerous squares
            return mobilityScore * 4 + cornerScore * 3 + dangerScore * 2;
        } else if(totalPieces <= 50) {
            // Midgame: Balanced approach
            return mobilityScore * 2 + stabilityScore + cornerScore * 2 + 
                   edgeScore + dangerScore;
        } else {
            // Endgame: Focus on disc count, corners, and parity
            int discDiff = (countDiscs(player) - countDiscs(opponent(player)));
            return discDiff * 3 + cornerScore * 3 + stabilityScore + parityScore;
        }
    }
};

const int OthelloBoard::AllDirections[8] = {-11, -10, -9, -1, 1, 9, 10, 11};
const int OthelloBoard::weights[100] = {
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

// Fast flip counting for move ordering
int countFlipsForMove(const OthelloBoard& board, int move, int player) {
    int flips = 0;
    for(int k = 0; k < 8; ++k) {
        int dir = OthelloBoard::AllDirections[k];
        int pos = move + dir;
        int run = 0;
        if(board.board[pos] != board.opponent(player)) continue;
        while(board.board[pos] == board.opponent(player)) {
            ++run;
            pos += dir;
        }
        if(board.board[pos] == player) flips += run;
    }
    return flips;
}

class OthelloRenderer {
public:
    SDL_Renderer* renderer;
    
    OthelloRenderer(SDL_Renderer* r) : renderer(r) {}
    
    void drawGrid() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        for(int i = 0; i <= 8; i++) {
            SDL_RenderDrawLine(renderer, tlx + i*SquareWidth, tly, 
                            tlx + i*SquareWidth, tly + 480);
            SDL_RenderDrawLine(renderer, tlx, tly + i*SquareWidth,
                            tlx + 480, tly + i*SquareWidth);
        }
    }
    
    void drawCircle(int centerX, int centerY, int radius, SDL_Color color) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        for(int w = 0; w < radius * 2; w++) {
            for(int h = 0; h < radius * 2; h++) {
                int dx = radius - w;
                int dy = radius - h;
                if((dx*dx + dy*dy) <= (radius * radius)) {
                    SDL_RenderDrawPoint(renderer, centerX + dx, centerY + dy);
                }
            }
        }
    }
    
    void drawPieces(const OthelloBoard& board) {
        for(int y = 1; y <= 8; y++) {
            for(int x = 1; x <= 8; x++) {
                int pos = y*10 + x;
                if(board.board[pos] == OthelloBoard::EMPTY) continue;
                
                SDL_Color color = (board.board[pos] == OthelloBoard::BLACK) ? 
                    SDL_Color{0,0,0,255} : SDL_Color{255,255,255,255};
                
                int centerX = tlx + x*SquareWidth - SquareWidth/2;
                int centerY = tly + y*SquareWidth - SquareWidth/2;
                drawCircle(centerX, centerY, 25, color);
            }
        }
    }
    
    void highlightLegalMoves(const OthelloBoard& board, int player) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow highlight
        for(int y = 1; y <= 8; y++) {
            for(int x = 1; x <= 8; x++) {
                int pos = y*10 + x;
                if(board.legalMove(pos, player)) {
                    SDL_Rect highlight = {
                        tlx + (x-1)*SquareWidth + 2,
                        tly + (y-1)*SquareWidth + 2,
                        SquareWidth - 4, SquareWidth - 4
                    };
                    SDL_RenderDrawRect(renderer, &highlight);
                }
            }
        }
    }
    
    void render(const OthelloBoard& board, int currentPlayer) {
        SDL_SetRenderDrawColor(renderer, 0, 128, 0, 255);
        SDL_RenderClear(renderer);
        drawGrid();
        highlightLegalMoves(board, currentPlayer);
        drawPieces(board);
        SDL_RenderPresent(renderer);
    }
};

class OthelloGame {
public:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    OthelloRenderer* othelloRenderer = nullptr;
    OthelloBoard board;
    std::vector<int> bestm;
    int player;
    int human;
    int computer;
    TranspositionTable transTable;
    TimeManager timeManager;
    bool timeExpired;
    int historyHeuristic[100]; // History heuristic for move ordering

    OthelloGame()
        : bestm(nply+1), player(OthelloBoard::BLACK), human(OthelloBoard::BLACK), computer(OthelloBoard::WHITE), timeExpired(false) {
        // Set AI thinking time based on game phase
        timeManager.setTimeLimit(2000); // 2 seconds per move
        // Initialize history heuristic
        for(int i = 0; i < 100; ++i) historyHeuristic[i] = 0;
    }

    void initSDL() {
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow("Othello", SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        othelloRenderer = new OthelloRenderer(renderer);
    }

    void cleanupSDL() {
        delete othelloRenderer;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void showBoard() {
        othelloRenderer->render(board, player);
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

    int alphabeta(int player, int alpha, int beta, int ply) {
        // Check if time limit exceeded
        if(timeManager.timeUp()) {
            timeExpired = true;
            return alpha; // Return current lower bound to maintain consistency
        }
        
        int originalAlpha = alpha;
        std::array<int, 100> boardArray = board.getBoardArray();
        
        // Check transposition table
        int ttValue, ttMove = -1;
        if(transTable.lookup(boardArray, player, ply, alpha, beta, ttValue, ttMove)) {
            if(ply > 0) bestm[ply] = ttMove;
            return ttValue;
        }
        
        if(ply == 0) {
            int evaluation = board.advancedEvaluation(player);
            transTable.store(boardArray, player, evaluation, ply, -1, TTEntry::EXACT);
            return evaluation;
        }
        
        // Fast move generation: only check inner 8x8 squares and empty cells
        std::vector<int> moves;
        moves.reserve(20); // Reserve space for efficiency
        for(int i = 11; i <= 88; ++i) {
            if(i % 10 == 0 || i % 10 == 9) { 
                i += (i % 10 == 9); // Skip border fast: jump to next row
                continue; 
            }
            if(board.board[i] == OthelloBoard::EMPTY && board.legalMove(i, player)) {
                moves.push_back(i);
            }
        }
        
        // Move ordering: prioritize TT move, then sort by advanced criteria
        int startSort = 0;
        if(ttMove != -1) {
            auto it = std::find(moves.begin(), moves.end(), ttMove);
            if(it != moves.end()) {
                moves.erase(it);
                moves.insert(moves.begin(), ttMove);
                startSort = 1;
            }
        }
        
        // Enhanced move ordering: corners → history → flips → static weights
        std::sort(moves.begin() + startSort, moves.end(), [this, player](int a, int b) {
            // Prioritize corners first
            bool aIsCorner = (a == 11 || a == 18 || a == 81 || a == 88);
            bool bIsCorner = (b == 11 || b == 18 || b == 81 || b == 88);
            if(aIsCorner != bIsCorner) return aIsCorner;
            
            // Then history heuristic
            int ha = historyHeuristic[a], hb = historyHeuristic[b];
            if(ha != hb) return ha > hb;
            
            // Then moves that flip more pieces
            int fa = countFlipsForMove(board, a, player);
            int fb = countFlipsForMove(board, b, player);
            if(fa != fb) return fa > fb;
            
            // Finally use static position weights
            return OthelloBoard::weights[a] > OthelloBoard::weights[b];
        });
        
        if(moves.empty()) {
            if(board.hasLegalMoves(board.opponent(player))) {
                int val = -alphabeta(board.opponent(player), -beta, -alpha, ply-1);
                transTable.store(boardArray, player, val, ply, -1, TTEntry::EXACT);
                return val;
            }
            int diff = 0;
            for(int i = 0; i < 100; ++i) diff += (board.board[i] == player) - (board.board[i] == board.opponent(player));
            int val = (diff > 0) ? WinningValue : (diff < 0) ? LosingValue : 0;
            transTable.store(boardArray, player, val, ply, -1, TTEntry::EXACT);
            return val;
        }
        
        int bestVal = INT_MIN;
        int bestMove = -1;
        for(int move : moves) {
            // Check time limit during search
            if(timeExpired) break;
            
            OthelloBoard::UndoInfo undo = board.makeMoveWithUndo(move, player);
            int val = -alphabeta(board.opponent(player), -beta, -alpha, ply-1);
            board.unmakeMove(undo, player);
            
            if(timeExpired) break;
            
            if(val > bestVal) {
                bestVal = val;
                bestMove = move;
                if(bestVal > alpha) {
                    alpha = bestVal;
                    bestm[ply] = bestMove;
                }
                if(alpha >= beta) {
                    // Update history heuristic on cutoff
                    historyHeuristic[bestMove] += ply * ply;
                    break;
                }
            }
        }
        
        // Store in transposition table
        TTEntry::Flag flag;
        if(bestVal <= originalAlpha) {
            flag = TTEntry::UPPER_BOUND;
        } else if(bestVal >= beta) {
            flag = TTEntry::LOWER_BOUND;
        } else {
            flag = TTEntry::EXACT;
        }
        transTable.store(boardArray, player, bestVal, ply, bestMove, flag);
        
        return bestVal;
    }

    int iterativeDeepening(int player, int maxDepth) {
        int bestMove = -1;
        timeExpired = false;
        int lastScore = 0;
        
        // Clear transposition table at start of search for new position
        transTable.clear();
        
        // Start the timer
        timeManager.startTimer();
        
        for(int depth = 1; depth <= maxDepth; depth++) {
            // Check if we have enough time for another iteration
            if(timeManager.getRemainingMs() < 100) { // Need at least 100ms for next depth
                break;
            }
            
            // Aspiration windows: narrow search around last score
            int delta = 64; // window half-size
            int alpha = lastScore - delta;
            int beta = lastScore + delta;
            int score;
            
            // Aspiration window loop
            for(;;) {
                bestm.assign(depth + 1, -1);
                score = alphabeta(player, alpha, beta, depth);
                
                if(timeExpired) break;
                
                // Check if we need to widen the window
                if(score <= alpha) {
                    // Fail-low: widen down
                    alpha -= delta;
                    delta <<= 1; // Double window size
                    continue;
                } else if(score >= beta) {
                    // Fail-high: widen up
                    beta += delta;
                    delta <<= 1; // Double window size
                    continue;
                } else {
                    // Success: score is within window
                    lastScore = score;
                    break;
                }
            }
            
            // If time expired during search, use previous depth result
            if(timeExpired) {
                break;
            }
            
            if(bestm[depth] != -1) {
                bestMove = bestm[depth];
            }
            
            // Optional: Print search info
            // printf("Depth %d completed in %dms, move: %d, score: %d\n", depth, timeManager.getElapsedMs(), bestMove, score);
        }
        
        return bestMove;
    }
    
    // Adaptive time management based on game phase
    void adjustTimeLimit() {
        int totalPieces = 0;
        for(int i = 0; i < 100; i++) {
            if(board.board[i] == OthelloBoard::BLACK || board.board[i] == OthelloBoard::WHITE) {
                totalPieces++;
            }
        }
        
        // Adjust time based on game phase
        if(totalPieces <= 20) {
            // Opening: use less time
            timeManager.setTimeLimit(1500); // 1.5 seconds
        } else if(totalPieces <= 50) {
            // Midgame: use standard time
            timeManager.setTimeLimit(2000); // 2 seconds
        } else {
            // Endgame: use more time for critical decisions
            timeManager.setTimeLimit(3000); // 3 seconds
        }
    }

    void run() {
        player = OthelloBoard::BLACK;
        human = OthelloBoard::BLACK;
        computer = board.opponent(human);
        bool gameRunning = true;
        
        while(gameRunning) {
            showBoard();
            
            // Check if current player has legal moves
            if(!board.hasLegalMoves(player)) {
                // Check if opponent has legal moves
                if(!board.hasLegalMoves(board.opponent(player))) {
                    // Game over - no legal moves for either player
                    gameRunning = false;
                    break;
                } else {
                    // Pass - switch to opponent
                    player = board.opponent(player);
                    continue;
                }
            }
            
            if(player == human) {
                int move = getMove();
                if(board.legalMove(move, player)) {
                    board.makeMove(move, player);
                    player = board.opponent(player);
                }
            } else {
                // Adjust time limit based on game phase
                adjustTimeLimit();
                
                int move = iterativeDeepening(player, nply);
                
                // Optional: Print search statistics (can be removed for production)
                // printf("AI move: %d, Time: %dms, TT entries: %zu\n", 
                //        move, timeManager.getElapsedMs(), transTable.size());
                
                // Optional: Print evaluation breakdown (for debugging)
                /*
                printf("Evaluation breakdown for move %d:\n", move);
                printf("  Mobility: %d\n", board.mobility(player));
                printf("  Corners: %d\n", board.cornerControl(player));
                printf("  Edges: %d\n", board.edgeControl(player));
                printf("  Stability: %d\n", board.stability(player));
                printf("  Dangerous: %d\n", board.dangerousSquares(player));
                printf("  Total: %d\n", board.advancedEvaluation(player));
                */
                
                if(move != -1) {
                    board.makeMove(move, player);
                    player = board.opponent(player);
                } else {
                    // Fallback: find any legal move if time-controlled search failed
                    for(int fallbackMove = 0; fallbackMove < 100; fallbackMove++) {
                        if(board.legalMove(fallbackMove, player)) {
                            board.makeMove(fallbackMove, player);
                            player = board.opponent(player);
                            break;
                        }
                    }
                }
            }
        }
        
        // Show final board state
        showBoard();
        SDL_Delay(3000); // Show result for 3 seconds
    }
};

int main(int argc, char* argv[]) {
    OthelloGame game;
    game.initSDL();
    game.run();
    game.cleanupSDL();
    return 0;
}