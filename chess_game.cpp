#include <iostream>
#include <cmath>
#include <cctype>
#include <string>
#include <vector>
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>

#define SIZE 8

class ChessGame {
private:
    char board[SIZE][SIZE];
    char currentPlayer;
    
    // Store move history for undo/redo functionality
    struct MoveRecord {
        int fromRow;
        int fromCol;
        int toRow;
        int toCol;
        char movedPiece;
        char capturedPiece;
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

public:
    ChessGame() {
        initialize();
        currentPlayer = 'w'; // White starts
        currentMoveIndex = -1; // No moves made yet
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
        if (!moveCheck(fromR, fromC, toR, toC, currentPlayer)) {
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
        
        // If we're not at the end of the history, truncate future moves
        if (currentMoveIndex < (int)moveHistory.size() - 1) {
            moveHistory.resize(currentMoveIndex + 1);
        }
        
        // Add the move to history
        moveHistory.push_back(move);
        currentMoveIndex++;
        
        // Make the move
        board[toR][toC] = board[fromR][fromC];
        board[fromR][fromC] = ' ';
        
        // Switch player
        currentPlayer = (currentPlayer == 'w') ? 'b' : 'w';
        
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
        board[move.toRow][move.toCol] = move.movedPiece;
        board[move.fromRow][move.fromCol] = ' ';
        
        // Switch player
        currentPlayer = (currentPlayer == 'w') ? 'b' : 'w';
        
        // Update the current move index
        currentMoveIndex++;
        
        return true;
    }

    bool isGameOver() const {
        bool whiteKingFound = false;
        bool blackKingFound = false;
        
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                if (board[i][j] == 'K') whiteKingFound = true;
                if (board[i][j] == 'k') blackKingFound = true;
            }
        }
        
        return !whiteKingFound || !blackKingFound;
    }
    
    std::string getMoveHistory() const {
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
};

// Emscripten bindings to expose the C++ class to JavaScript
EMSCRIPTEN_BINDINGS(chess_module) {
    emscripten::class_<ChessGame>("ChessGame")
        .constructor<>()
        .function("initialize", &ChessGame::initialize)
        .function("getBoardState", &ChessGame::getBoardState)
        .function("getCurrentPlayer", &ChessGame::getCurrentPlayer)
        .function("makeMove", &ChessGame::makeMove)
        .function("undoMove", &ChessGame::undoMove)
        .function("redoMove", &ChessGame::redoMove)
        .function("isGameOver", &ChessGame::isGameOver)
        .function("canUndo", &ChessGame::canUndo)
        .function("canRedo", &ChessGame::canRedo)
        .function("getMoveHistory", &ChessGame::getMoveHistory)
        .function("getCurrentMoveIndex", &ChessGame::getCurrentMoveIndex);
}