#include <iostream>
#include <cmath>
#include <cctype>
#include <string>
#include <vector>
// #include <emscripten/emscripten.h>
// #include <emscripten/bind.h>

#define SIZE 8

class ChessGame {
private:
    char board[SIZE][SIZE];
    char currentPlayer;
    bool inCheck;
    
    // Store move history for undo/redo functionality
    struct MoveRecord {
        int fromRow;
        int fromCol;
        int toRow;
        int toCol;
        char movedPiece;
        char capturedPiece;
        bool wasCheck;
        bool wasCheckmate;
        bool wasPromotion;
        char promotedTo;
    };
    
    std::vector<MoveRecord> moveHistory;
    int currentMoveIndex; // Current position in move history
    
    // Helper function to check if path is clear for sliding pieces
    bool isPathClear(int fromR, int fromC, int toR, int toC) const {
        int rowStep = (toR > fromR) ? 1 : ((toR < fromR) ? -1 : 0);
        int colStep = (toC > fromC) ? 1 : ((toC < fromC) ? -1 : 0);
        
        int r = fromR + rowStep;
        int c = fromC + colStep;
        
        while (r != toR || c != toC) {
            if (board[r][c] != ' ') {
                return false;
            }
            r += rowStep;
            c += colStep;
        }
        
        return true;
    }
    
    // Check if destination square has a piece of the same color
    bool isSameColorPiece(int r, int c, char player) const {
        if (board[r][c] == ' ') return false;
        
        if (player == 'w') {
            return isupper(board[r][c]);
        } else {
            return islower(board[r][c]);
        }
    }
    
    // Find the position of the king for a given player
    bool findKing(char player, int& kingRow, int& kingCol) const {
        char kingChar = (player == 'w') ? 'K' : 'k';
        
        for (int r = 0; r < SIZE; r++) {
            for (int c = 0; c < SIZE; c++) {
                if (board[r][c] == kingChar) {
                    kingRow = r;
                    kingCol = c;
                    return true;
                }
            }
        }
        
        return false; // King not found (shouldn't happen in a valid game)
    }
    
    // Check if a square is under attack by the opponent
    bool isSquareUnderAttack(int row, int col, char attackingPlayer) const {
        // Check attacks from all 8 directions (for queen, rook, bishop)
        const int directions[8][2] = {
            {-1, 0}, {1, 0}, {0, -1}, {0, 1},  // Rook/Queen directions
            {-1, -1}, {-1, 1}, {1, -1}, {1, 1}  // Bishop/Queen directions
        };
        
        // Check sliding pieces (queen, rook, bishop)
        for (int d = 0; d < 8; d++) {
            int dr = directions[d][0];
            int dc = directions[d][1];
            int r = row + dr;
            int c = col + dc;
            
            while (r >= 0 && r < SIZE && c >= 0 && c < SIZE) {
                if (board[r][c] != ' ') {
                    char piece = board[r][c];
                    bool isPieceFromAttackingPlayer = (attackingPlayer == 'w') ? isupper(piece) : islower(piece);
                    
                    if (isPieceFromAttackingPlayer) {
                        char pieceType = toupper(piece);
                        
                        // Check if this piece can attack in this direction
                        if (pieceType == 'Q' || 
                            (pieceType == 'R' && d < 4) ||  // Rook can only attack in first 4 directions
                            (pieceType == 'B' && d >= 4)) {  // Bishop can only attack in last 4 directions
                            return true;
                        }
                    }
                    
                    // Blocked by a piece, stop checking this direction
                    break;
                }
                
                r += dr;
                c += dc;
            }
        }
        
        // Check knight attacks
        const int knightMoves[8][2] = {
            {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
            {1, -2}, {1, 2}, {2, -1}, {2, 1}
        };
        
        for (int k = 0; k < 8; k++) {
            int r = row + knightMoves[k][0];
            int c = col + knightMoves[k][1];
            
            if (r >= 0 && r < SIZE && c >= 0 && c < SIZE) {
                char piece = board[r][c];
                bool isPieceFromAttackingPlayer = (attackingPlayer == 'w') ? isupper(piece) : islower(piece);
                
                if (isPieceFromAttackingPlayer && toupper(piece) == 'N') {
                    return true;
                }
            }
        }
        
        // Check pawn attacks
        int pawnDirection = (attackingPlayer == 'w') ? -1 : 1;  // White pawns attack downward, black pawns attack upward
        char pawnChar = (attackingPlayer == 'w') ? 'P' : 'p';
        
        for (int dc : {-1, 1}) {  // Pawns attack diagonally
            int r = row + pawnDirection;
            int c = col + dc;
            
            if (r >= 0 && r < SIZE && c >= 0 && c < SIZE && board[r][c] == pawnChar) {
                return true;
            }
        }
        
        // Check king attacks (for adjacent squares)
        const int kingMoves[8][2] = {
            {-1, -1}, {-1, 0}, {-1, 1}, {0, -1},
            {0, 1}, {1, -1}, {1, 0}, {1, 1}
        };
        
        char kingChar = (attackingPlayer == 'w') ? 'K' : 'k';
        
        for (int k = 0; k < 8; k++) {
            int r = row + kingMoves[k][0];
            int c = col + kingMoves[k][1];
            
            if (r >= 0 && r < SIZE && c >= 0 && c < SIZE && board[r][c] == kingChar) {
                return true;
            }
        }
        
        return false;
    }
    
    // Check if the current player is in check
    bool isInCheck(char player) const {
        int kingRow, kingCol;
        if (!findKing(player, kingRow, kingCol)) {
            return false;  // King not found (shouldn't happen in a valid game)
        }
        
        char opponentPlayer = (player == 'w') ? 'b' : 'w';
        return isSquareUnderAttack(kingRow, kingCol, opponentPlayer);
    }
    
    // Check if a move would leave the player's king in check
    bool wouldBeInCheck(int fromR, int fromC, int toR, int toC, char player) const {
        // Make a temporary copy of the board
        char tempBoard[SIZE][SIZE];
        for (int r = 0; r < SIZE; r++) {
            for (int c = 0; c < SIZE; c++) {
                tempBoard[r][c] = board[r][c];
            }
        }
        
        // Temporarily make the move on the copy
        ChessGame* nonConstThis = const_cast<ChessGame*>(this);
        char originalPiece = nonConstThis->board[fromR][fromC];
        char capturedPiece = nonConstThis->board[toR][toC];
        
        nonConstThis->board[toR][toC] = nonConstThis->board[fromR][fromC];
        nonConstThis->board[fromR][fromC] = ' ';
        
        // Check if the king is in check after the move
        bool inCheck = isInCheck(player);
        
        // Restore the original board
        nonConstThis->board[fromR][fromC] = originalPiece;
        nonConstThis->board[toR][toC] = capturedPiece;
        
        return inCheck;
    }
    
    // Check if the current player has any legal moves
    bool hasLegalMoves(char player) const {
        for (int fromR = 0; fromR < SIZE; fromR++) {
            for (int fromC = 0; fromC < SIZE; fromC++) {
                char piece = board[fromR][fromC];
                
                // Skip empty squares and opponent's pieces
                if (piece == ' ' || (player == 'w' && islower(piece)) || (player == 'b' && isupper(piece))) {
                    continue;
                }
                
                // Try all possible destination squares
                for (int toR = 0; toR < SIZE; toR++) {
                    for (int toC = 0; toC < SIZE; toC++) {
                        // Check if the move is valid and doesn't leave the king in check
                        if (moveCheck(fromR, fromC, toR, toC, player) && 
                            !wouldBeInCheck(fromR, fromC, toR, toC, player)) {
                            return true;
                        }
                    }
                }
            }
        }
        
        return false;
    }
    
    // Get algebraic notation for a square (e.g., "e4")
    std::string getSquareNotation(int row, int col) const {
        std::string notation;
        notation += ('a' + col);
        notation += ('8' - row);
        return notation;
    }
    
    // Get piece notation for move history
    std::string getPieceNotation(char piece) const {
        char pieceType = toupper(piece);
        if (pieceType == 'P') return "";
        if (pieceType == 'N') return "N";
        if (pieceType == 'B') return "B";
        if (pieceType == 'R') return "R";
        if (pieceType == 'Q') return "Q";
        if (pieceType == 'K') return "K";
        return "";
    }

public:
    ChessGame() {
        initialize();
        currentPlayer = 'w'; // White starts
        currentMoveIndex = -1; // No moves made yet
        inCheck = false;
    }

    void initialize() {
        // Initialize black pieces (lowercase)
        board[0][0] = board[0][7] = 'r';
        board[0][1] = board[0][6] = 'n';
        board[0][2] = board[0][5] = 'b';
        board[0][3] = 'q';
        board[0][4] = 'k';
        for (int i = 0; i < SIZE; i++) {
            board[1][i] = 'p';
        }

        // Initialize white pieces (uppercase)
        board[7][0] = board[7][7] = 'R';
        board[7][1] = board[7][6] = 'N';
        board[7][2] = board[7][5] = 'B';
        board[7][3] = 'Q';
        board[7][4] = 'K';
        for (int i = 0; i < SIZE; i++) {
            board[6][i] = 'P';
        }

        // Initialize empty spaces
        for (int i = 2; i < 6; i++) {
            for (int j = 0; j < SIZE; j++) {
                board[i][j] = ' ';
            }
        }
        
        // Clear move history
        moveHistory.clear();
        currentMoveIndex = -1;
        currentPlayer = 'w';
        inCheck = false;
    }

    std::string getBoardState() const {
        std::string state;
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                state += board[i][j];
            }
        }
        return state;
    }
    
    char getCurrentPlayer() const {
        return currentPlayer;
    }
    
    bool canUndo() const {
        return currentMoveIndex >= 0;
    }
    
    bool canRedo() const {
        return currentMoveIndex < (int)moveHistory.size() - 1;
    }
    
    bool isInCheckState() const {
        return inCheck;
    }
    
    bool isCheckmate() const {
        return inCheck && !hasLegalMoves(currentPlayer);
    }
    
    bool isStalemate() const {
        return !inCheck && !hasLegalMoves(currentPlayer);
    }

    bool validMove(const std::string& moveStr, int& col, int& row) const {
        if (moveStr.length() == 2 && 
            moveStr[0] >= 'a' && moveStr[0] <= 'h' && 
            moveStr[1] >= '1' && moveStr[1] <= '8') {
            col = moveStr[0] - 'a';
            row = 8 - (moveStr[1] - '0');
            return true;
        }
        return false;
    }

    bool moveCheck(int fromR, int fromC, int toR, int toC, char player) const {
        // Check if the piece belongs to the current player
        if (board[fromR][fromC] == ' ' || 
            (player == 'w' && islower(board[fromR][fromC])) || 
            (player == 'b' && isupper(board[fromR][fromC]))) {
            return false;
        }
        
        // Check if destination has a piece of the same color
        if (isSameColorPiece(toR, toC, player)) {
            return false;
        }

        char piece = board[fromR][fromC];
        char pieceType = toupper(piece);
        
        // Rook movement (horizontal or vertical)
        if (pieceType == 'R' && (fromR == toR || fromC == toC)) {
            return isPathClear(fromR, fromC, toR, toC);
        }

        // Knight movement (L-shape)
        if (pieceType == 'N' && 
            ((abs(fromR - toR) == 1 && abs(fromC - toC) == 2) || 
             (abs(fromR - toR) == 2 && abs(fromC - toC) == 1))) {
            return true; // Knights can jump over pieces
        }

        // Bishop movement (diagonal)
        if (pieceType == 'B' && (abs(fromR - toR) == abs(fromC - toC))) {
            return isPathClear(fromR, fromC, toR, toC);
        }

        // Queen movement (combination of rook and bishop)
        if (pieceType == 'Q' && 
            ((fromR == toR || fromC == toC) || (abs(fromR - toR) == abs(fromC - toC)))) {
            return isPathClear(fromR, fromC, toR, toC);
        }
        
        // King movement (one square in any direction)
        if (pieceType == 'K' && abs(fromR - toR) <= 1 && abs(fromC - toC) <= 1) {
            return true;
        }

        // Pawn movement
        if (pieceType == 'P') {
            int direction = (isupper(piece)) ? -1 : 1; // White moves up (-1), Black moves down (+1)
            
            // Forward movement (no capture)
            if (fromC == toC && board[toR][toC] == ' ') {
                // Single square forward
                if (toR == fromR + direction) {
                    return true;
                }
                
                // Double square forward from starting position
                if ((isupper(piece) && fromR == 6 && toR == 4) || 
                    (islower(piece) && fromR == 1 && toR == 3)) {
                    return board[fromR + direction][fromC] == ' '; // Check if path is clear
                }
            }
            
            // Diagonal capture
            if (abs(fromC - toC) == 1 && toR == fromR + direction) {
                return board[toR][toC] != ' ' && 
                       ((isupper(piece) && islower(board[toR][toC])) || 
                        (islower(piece) && isupper(board[toR][toC])));
            }
        }

        return false;
    }

    bool makeMove(int fromR, int fromC, int toR, int toC) {
        // Check if the move is valid according to chess rules
        if (!moveCheck(fromR, fromC, toR, toC, currentPlayer)) {
            return false;
        }
        
        // Check if the move would leave the king in check
        if (wouldBeInCheck(fromR, fromC, toR, toC, currentPlayer)) {
            return false;
        }
        
        // Record the move for undo/redo
        MoveRecord move;
        move.fromRow = fromR;
        move.fromCol = fromC;
        move.toRow = toR;
        move.toCol = toC;
        move.movedPiece = board[fromR][fromC];
        move.capturedPiece = board[toR][toC];
        move.wasPromotion = false;
        move.promotedTo = ' ';
        
        // If we're not at the end of the history, truncate future moves
        if (currentMoveIndex < (int)moveHistory.size() - 1) {
            moveHistory.resize(currentMoveIndex + 1);
        }
        
        // Make the move
        board[toR][toC] = board[fromR][fromC];
        board[fromR][fromC] = ' ';
        
        // Handle pawn promotion (automatically promote to queen for simplicity)
        if (toupper(move.movedPiece) == 'P') {
            // White pawn reaches the top row or black pawn reaches the bottom row
            if ((isupper(move.movedPiece) && toR == 0) || (islower(move.movedPiece) && toR == 7)) {
                move.wasPromotion = true;
                move.promotedTo = isupper(move.movedPiece) ? 'Q' : 'q';
                board[toR][toC] = move.promotedTo;
            }
        }
        
        // Switch player
        currentPlayer = (currentPlayer == 'w') ? 'b' : 'w';
        
        // Check if the opponent is now in check or checkmate
        inCheck = isInCheck(currentPlayer);
        move.wasCheck = inCheck;
        move.wasCheckmate = isCheckmate();
        
        // Add the move to history
        moveHistory.push_back(move);
        currentMoveIndex++;
        
        return true;
    }
    
    bool undoMove() {
        if (!canUndo()) {
            return false;
        }
        
        // Get the last move
        const MoveRecord& move = moveHistory[currentMoveIndex];
        
        // Restore the board state
        board[move.fromRow][move.fromCol] = move.movedPiece;
        board[move.toRow][move.toCol] = move.capturedPiece;
        
        // Switch back to the previous player
        currentPlayer = (currentPlayer == 'w') ? 'b' : 'w';
        
        // Update check status
        inCheck = (currentMoveIndex > 0) ? moveHistory[currentMoveIndex - 1].wasCheck : false;
        
        // Update the current move index
        currentMoveIndex--;
        
        return true;
    }
    
    bool redoMove() {
        if (!canRedo()) {
            return false;
        }
        
        // Get the next move
        const MoveRecord& move = moveHistory[currentMoveIndex + 1];
        
        // Apply the move
        board[move.toRow][move.toCol] = move.wasPromotion ? move.promotedTo : move.movedPiece;
        board[move.fromRow][move.fromCol] = ' ';
        
        // Switch player
        currentPlayer = (currentPlayer == 'w') ? 'b' : 'w';
        
        // Update check status
        inCheck = move.wasCheck;
        
        // Update the current move index
        currentMoveIndex++;
        
        return true;
    }

    bool isGameOver() const {
        // Game is over if there's a checkmate or stalemate
        return isCheckmate() || isStalemate() || !hasKings();
    }
    
    bool hasKings() const {
        bool whiteKingFound = false;
        bool blackKingFound = false;
        
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                if (board[i][j] == 'K') whiteKingFound = true;
                if (board[i][j] == 'k') blackKingFound = true;
                
                if (whiteKingFound && blackKingFound) return true;
            }
        }
        
        return whiteKingFound && blackKingFound;
    }
    
    std::string getMoveHistory() const {
        std::string history;
        for (size_t i = 0; i < moveHistory.size(); i++) {
            const MoveRecord& move = moveHistory[i];
            
            // Add move number for white's moves
            if (i % 2 == 0) {
                history += std::to_string(i / 2 + 1) + ". ";
            }
            
            // Add piece notation (except for pawns)
            history += getPieceNotation(move.movedPiece);
            
            // Add source and destination squares
            history += getSquareNotation(move.fromRow, move.fromCol);
            
            // Use 'x' for captures
            if (move.capturedPiece != ' ') {
                history += "x";
            } else {
                history += "-";
            }
            
            history += getSquareNotation(move.toRow, move.toCol);
            
            // Add promotion indicator
            if (move.wasPromotion) {
                history += "=" + std::string(1, toupper(move.promotedTo));
            }
            
            // Add check/checkmate indicator
            if (move.wasCheckmate) {
                history += "#";
            } else if (move.wasCheck) {
                history += "+";
            }
            
            // Add separator
            if (i < moveHistory.size() - 1) {
                if (i % 2 == 0) {
                    history += " ";  // Space between white and black moves
                } else {
                    history += ", ";  // Comma between move pairs
                }
            }
        }
        return history;
    }
    
    std::string getRawMoveHistory() const {
        std::string history;
        for (size_t i = 0; i < moveHistory.size(); i++) {
            const MoveRecord& move = moveHistory[i];
            char fromCol = 'a' + move.fromCol;
            char fromRow = '8' - move.fromRow;
            char toCol = 'a' + move.toCol;
            char toRow = '8' - move.toRow;
            
            history += fromCol;
            history += fromRow;
            history += toCol;
            history += toRow;
            
            if (i < moveHistory.size() - 1) {
                history += ",";
            }
        }
        return history;
    }
    
    int getCurrentMoveIndex() const {
        return currentMoveIndex;
    }
    
    std::string getGameStatus() const {
        if (isCheckmate()) {
            return std::string("checkmate_") + (currentPlayer == 'w' ? "black" : "white");
        } else if (isStalemate()) {
            return "stalemate";
        } else if (inCheck) {
            return std::string("check_") + (currentPlayer == 'w' ? "white" : "black");
        } else {
            return "ongoing";
        }
    }
};

// Emscripten bindings to expose the C++ class to JavaScript
// EMSCRIPTEN_BINDINGS(chess_module) {
//     emscripten::class_<ChessGame>("ChessGame")
//         .constructor<>()
//         .function("initialize", &ChessGame::initialize)
//         .function("getBoardState", &ChessGame::getBoardState)
//         .function("getCurrentPlayer", &ChessGame::getCurrentPlayer)
//         .function("makeMove", &ChessGame::makeMove)
//         .function("undoMove", &ChessGame::undoMove)
//         .function("redoMove", &ChessGame::redoMove)
//         .function("isGameOver", &ChessGame::isGameOver)
//         .function("isInCheckState", &ChessGame::isInCheckState)
//         .function("isCheckmate", &ChessGame::isCheckmate)
//         .function("isStalemate", &ChessGame::isStalemate)
//         .function("canUndo", &ChessGame::canUndo)
//         .function("canRedo", &ChessGame::canRedo)
//         .function("getMoveHistory", &ChessGame::getMoveHistory)
//         .function("getRawMoveHistory", &ChessGame::getRawMoveHistory)
//         .function("getCurrentMoveIndex", &ChessGame::getCurrentMoveIndex)
//         .function("getGameStatus", &ChessGame::getGameStatus);
// }