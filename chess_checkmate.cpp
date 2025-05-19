#include <iostream>
#include <cmath>
#include <cctype>
#include <string>
#include <vector>
//#include <emscripten/emscripten.h>
//#include <emscripten/bind.h>

#define SIZE 8

class ChessGame {
private:
    char board[SIZE][SIZE];
    char currentPlayer;

    struct MoveRecord {
        int fromRow, fromCol, toRow, toCol;
        char movedPiece, capturedPiece;
    };

    std::vector<MoveRecord> moveHistory;
    int currentMoveIndex;

    bool isPathClear(int fromR, int fromC, int toR, int toC) const {
        int rowStep = (toR > fromR) ? 1 : ((toR < fromR) ? -1 : 0);
        int colStep = (toC > fromC) ? 1 : ((toC < fromC) ? -1 : 0);
        int r = fromR + rowStep;
        int c = fromC + colStep;

        while (r != toR || c != toC) {
            if (board[r][c] != ' ') return false;
            r += rowStep;
            c += colStep;
        }
        return true;
    }

    bool isSameColorPiece(int r, int c, char player) const {
        if (board[r][c] == ' ') return false;
        return (player == 'w') ? isupper(board[r][c]) : islower(board[r][c]);
    }

public:
    ChessGame() {
        initialize();
        currentPlayer = 'w';
        currentMoveIndex = -1;
    }

    void initialize() {
        // Black pieces
        board[0][0] = board[0][7] = 'r';
        board[0][1] = board[0][6] = 'n';
        board[0][2] = board[0][5] = 'b';
        board[0][3] = 'q';
        board[0][4] = 'k';
        for (int i = 0; i < SIZE; i++) board[1][i] = 'p';

        // White pieces
        board[7][0] = board[7][7] = 'R';
        board[7][1] = board[7][6] = 'N';
        board[7][2] = board[7][5] = 'B';
        board[7][3] = 'Q';
        board[7][4] = 'K';
        for (int i = 0; i < SIZE; i++) board[6][i] = 'P';

        // Empty spaces
        for (int i = 2; i < 6; i++)
            for (int j = 0; j < SIZE; j++)
                board[i][j] = ' ';

        moveHistory.clear();
        currentMoveIndex = -1;
    }

    std::string getBoardState() const {
        std::string state;
        for (int i = 0; i < SIZE; i++)
            for (int j = 0; j < SIZE; j++)
                state += board[i][j];
        return state;
    }

    char getCurrentPlayer() const {
        return currentPlayer;
    }

    bool isGameOver() const {
        bool whiteKing = false, blackKing = false;
        for (int i = 0; i < SIZE; i++)
            for (int j = 0; j < SIZE; j++) {
                if (board[i][j] == 'K') whiteKing = true;
                if (board[i][j] == 'k') blackKing = true;
            }
        return !(whiteKing && blackKing);
    }

    bool moveCheck(int fromR, int fromC, int toR, int toC, char player) const {
        if (board[fromR][fromC] == ' ' ||
            (player == 'w' && islower(board[fromR][fromC])) ||
            (player == 'b' && isupper(board[fromR][fromC])) ||
            isSameColorPiece(toR, toC, player)) return false;

        char piece = board[fromR][fromC];
        char type = toupper(piece);

        if (type == 'R' && (fromR == toR || fromC == toC))
            return isPathClear(fromR, fromC, toR, toC);
        if (type == 'N' &&
            ((abs(fromR - toR) == 2 && abs(fromC - toC) == 1) ||
             (abs(fromR - toR) == 1 && abs(fromC - toC) == 2)))
            return true;
        if (type == 'B' && abs(fromR - toR) == abs(fromC - toC))
            return isPathClear(fromR, fromC, toR, toC);
        if (type == 'Q' && 
            ((fromR == toR || fromC == toC) || abs(fromR - toR) == abs(fromC - toC)))
            return isPathClear(fromR, fromC, toR, toC);
        if (type == 'K' && abs(fromR - toR) <= 1 && abs(fromC - toC) <= 1)
            return true;
        if (type == 'P') {
            int dir = isupper(piece) ? -1 : 1;
            if (fromC == toC && board[toR][toC] == ' ') {
                if (toR == fromR + dir) return true;
                if ((isupper(piece) && fromR == 6 && toR == 4) ||
                    (islower(piece) && fromR == 1 && toR == 3))
                    return board[fromR + dir][fromC] == ' ';
            }
            if (abs(fromC - toC) == 1 && toR == fromR + dir)
                return board[toR][toC] != ' ' &&
                    ((isupper(piece) && islower(board[toR][toC])) ||
                     (islower(piece) && isupper(board[toR][toC])));
        }
        return false;
    }

    bool makeMove(int fromR, int fromC, int toR, int toC) {
        if (!moveCheck(fromR, fromC, toR, toC, currentPlayer)) return false;

        MoveRecord move = {fromR, fromC, toR, toC, board[fromR][fromC], board[toR][toC]};
        if (currentMoveIndex < (int)moveHistory.size() - 1)
            moveHistory.resize(currentMoveIndex + 1);
        moveHistory.push_back(move);
        currentMoveIndex++;

        board[toR][toC] = board[fromR][fromC];
        board[fromR][fromC] = ' ';
        currentPlayer = (currentPlayer == 'w') ? 'b' : 'w';
        return true;
    }

    bool isInCheck(char player) const {
        int kingR = -1, kingC = -1;
        char king = (player == 'w') ? 'K' : 'k';
        for (int r = 0; r < SIZE; r++)
            for (int c = 0; c < SIZE; c++)
                if (board[r][c] == king) {
                    kingR = r;
                    kingC = c;
                }

        char opp = (player == 'w') ? 'b' : 'w';
        for (int r = 0; r < SIZE; r++)
            for (int c = 0; c < SIZE; c++)
                if ((opp == 'w' && isupper(board[r][c])) ||
                    (opp == 'b' && islower(board[r][c])))
                    if (moveCheck(r, c, kingR, kingC, opp))
                        return true;

        return false;
    }

    bool hasLegalMoves(char player) {
        for (int fromR = 0; fromR < SIZE; fromR++)
            for (int fromC = 0; fromC < SIZE; fromC++)
                if ((player == 'w' && isupper(board[fromR][fromC])) ||
                    (player == 'b' && islower(board[fromR][fromC])))
                    for (int toR = 0; toR < SIZE; toR++)
                        for (int toC = 0; toC < SIZE; toC++)
                            if (moveCheck(fromR, fromC, toR, toC, player)) {
                                char backupFrom = board[fromR][fromC];
                                char backupTo = board[toR][toC];
                                board[toR][toC] = backupFrom;
                                board[fromR][fromC] = ' ';
                                bool inCheck = isInCheck(player);
                                board[fromR][fromC] = backupFrom;
                                board[toR][toC] = backupTo;
                                if (!inCheck) return true;
                            }
        return false;
    }

    std::string getGameStatus() {
        if (isGameOver()) return "Game Over";
        if (isInCheck(currentPlayer)) {
            if (!hasLegalMoves(currentPlayer)) return "Checkmate";
            return "Check";
        } else {
            if (!hasLegalMoves(currentPlayer)) return "Stalemate";
        }
        return "Ongoing";
    }

    void displayBoard() const {
        std::cout << "  a b c d e f g h" << std::endl;
        std::cout << " +-+-+-+-+-+-+-+-+" << std::endl;
        for (int i = 0; i < SIZE; i++) {
            std::cout << 8 - i << "|";
            for (int j = 0; j < SIZE; j++) {
                std::cout << board[i][j] << "|";
            }
            std::cout << 8 - i << std::endl;
            std::cout << " +-+-+-+-+-+-+-+-+" << std::endl;
        }
        std::cout << "  a b c d e f g h" << std::endl;
    }
};

// Emscripten Bindings
/**EMSCRIPTEN_BINDINGS(chess_module) {
    emscripten::class_<ChessGame>("ChessGame")
        .constructor<>()
        .function("initialize", &ChessGame::initialize)
        .function("getBoardState", &ChessGame::getBoardState)
        .function("getCurrentPlayer", &ChessGame::getCurrentPlayer)
        .function("makeMove", &ChessGame::makeMove)
        .function("isGameOver", &ChessGame::isGameOver)
        .function("getGameStatus", &ChessGame::getGameStatus);
}**/

// Convert algebraic notation (e.g., "e2e4") to board coordinates
bool parseMove(const std::string& moveStr, int& fromR, int& fromC, int& toR, int& toC) {
    if (moveStr.length() != 4) return false;
    
    fromC = moveStr[0] - 'a';
    fromR = 8 - (moveStr[1] - '0');
    toC = moveStr[2] - 'a';
    toR = 8 - (moveStr[3] - '0');
    
    if (fromR < 0 || fromR >= SIZE || fromC < 0 || fromC >= SIZE ||
        toR < 0 || toR >= SIZE || toC < 0 || toC >= SIZE)
        return false;
        
    return true;
}

int main() {
    ChessGame game;
    std::string input;
    bool gameRunning = true;
    
    std::cout << "Chess Game" << std::endl;
    std::cout << "Enter moves in algebraic notation (e.g., e2e4)" << std::endl;
    std::cout << "Commands: 'quit' to exit, 'status' for game status" << std::endl;
    
    while (gameRunning) {
        game.displayBoard();
        
        std::string status = game.getGameStatus();
        char currentPlayer = game.getCurrentPlayer();
        
        std::cout << "Status: " << status << std::endl;
        
        if (status == "Checkmate" || status == "Stalemate" || status == "Game Over") {
            std::cout << "Game ended: " << status << std::endl;
            break;
        }
        
        std::cout << (currentPlayer == 'w' ? "White" : "Black") << " to move: ";
        std::cin >> input;
        
        if (input == "quit") {
            gameRunning = false;
        } else if (input == "status") {
            std::cout << "Game status: " << game.getGameStatus() << std::endl;
            if (game.isInCheck(currentPlayer)) {
                std::cout << (currentPlayer == 'w' ? "White" : "Black") << " is in check!" << std::endl;
            }
        } else {
            int fromR, fromC, toR, toC;
            if (parseMove(input, fromR, fromC, toR, toC)) {
                if (!game.makeMove(fromR, fromC, toR, toC)) {
                    std::cout << "Invalid move! Try again." << std::endl;
                }
            } else {
                std::cout << "Invalid input format! Use format like 'e2e4'." << std::endl;
            }
        }
    }
    
    std::cout << "Thanks for playing!" << std::endl;
    return 0;
}