/*
* @file botTemplate.cpp
* @author Arun Tejasvi Chaganty <arunchaganty@gmail.com>
* @date 2010-02-04
* Template for users to create their own bots
*/

#include "Othello.h"
#include "OthelloBoard.h"
#include "OthelloPlayer.h"
#include <cstdlib>
#include <limits.h>
#include <time.h>
using namespace std;
using namespace Desdemona;

class MyBot: public OthelloPlayer
{
    public:
        /**
         * Initialisation routines here
         * This could do anything from open up a cache of "best moves" to
         * spawning a background processing thread. 
         */
        MyBot( Turn turn );

        /**
         * Play something 
         */
        virtual Move play( const OthelloBoard& board );
    private:
};

MyBot::MyBot( Turn turn )
    : OthelloPlayer( turn )
{
}

// ply-depth
const int plyDepth = 4;
const int BoardSize = 8;


// evaluate net corners coins currently we are holding
int evaluateCorner(const OthelloBoard& board, Turn turn) {

    int myNumCor = 0;
    int otherCor = 0;

    int dx[] = {0, BoardSize - 1};
    int dy[] = {0, BoardSize - 1};

    for(int i = 0; i < 2; i++) {
        for(int j = 0; j < 2; j++) {
            int pos1 = dx[i];
            int pos2 = dy[j];
            if(board.get(pos1, pos2) == turn)
                myNumCor++;

            else if(board.get(pos1, pos2) == other(turn))
                otherCor++;
        }
    }

    return 100 * (myNumCor - otherCor) / (myNumCor + otherCor + 1);
}

// evaluate the net mobility in the current board position
int evaluateMobility(const OthelloBoard& board, Turn turn) {

    int myMob = board.getValidMoves(turn).size();
    int otherMob = board.getValidMoves(other(turn)).size();

    return 100 * (myMob - otherMob) / (myMob + otherMob + 1);
}

// helper function for coin stability
bool areDiagonalsFilled(int i, int j, const OthelloBoard& board )
{
    int l = i, r = j;
    while(0 <= l && l < 8 && 0 <= r && r < 8){
        if(board.get(l,r) == EMPTY)
            return 0;
        l++, r++;
    }
    l = i, r = j;
    while(0 <= l && l < 8 && 0 <= r && r < 8){
        if(board.get(l,r) == EMPTY)
            return 0;
        l++, r--;
    }
    l = i, r = j;
    while(0 <= l && l < 8 && 0 <= r && r < 8){
        if(board.get(l,r) == EMPTY)
            return 0;
        l--, r++;
    }
    l = i, r = j;
    while(0 <= l && l < 8 && 0 <= r && r < 8){
        if(board.get(l,r) == EMPTY)
            return 0;
        l--, r--;
    }
    return 1;
}

// evaluating coin stability for the player 
int evalStability(const OthelloBoard& board, Turn turn)
{
    Turn myTurn = turn;
    Turn opTurn = other(turn);

    bool rowFd[8];
    bool colFd[8];

    bool check;
    for (int i = 0; i < 8; i++){
        check = true;
        for (int j = 0; j < 8; j++){
            if (board.get(i, j) == EMPTY)
                check = false;
        }
        if (check)
            rowFd[i] = true;
    }
    for (int j = 0; j < 8; j++){
        check = true;
        for (int i = 0; i < 8; i++){
            if (board.get(i, j) == EMPTY)
                check = false;
        }
        if (check)
            colFd[j] = true;
    }
    int stability = 0;
    for( int i = 0; i < 8; i ++){
        for(int j = 0; j < 8; j ++){
            if(rowFd[i] && colFd[j] && areDiagonalsFilled(i,j,board) && myTurn == board.get(i, j))
                stability++;
            if(rowFd[i] && colFd[j] && areDiagonalsFilled(i,j,board) && opTurn == board.get(i, j))
                stability--;
        }
    }
    if(rowFd[0] && myTurn == board.get(0, 0)) stability += 8;
    if(rowFd[7] && myTurn == board.get(7, 0)) stability += 8;
    if(colFd[0] && myTurn == board.get(0, 7)) stability += 8;
    if(colFd[7] && myTurn == board.get(7, 7)) stability += 8;
    if(rowFd[0] && opTurn == board.get(0, 0)) stability -= 8;
    if(rowFd[7] && opTurn == board.get(7, 0)) stability -= 8;
    if(colFd[0] && opTurn == board.get(0, 7)) stability -= 8;
    if(colFd[7] && opTurn == board.get(7, 7)) stability -= 8;

    return stability;
}

// evaluation function for the terminal nodes
// given weight accordinginly to different factors
int evalFunc(const OthelloBoard board, Turn turn) {
    int myCoins = 0, otherCoins = 0;
    if(turn == BLACK) {
        myCoins = board.getBlackCount();
        otherCoins = board.getRedCount();
    }

    else {
        myCoins = board.getRedCount();
        otherCoins = board.getBlackCount();        
    }

    int cnt = myCoins + otherCoins;
    int stab = 0;
    // int stab = evalStability(board, turn);

    int eCD = 100 * (myCoins - otherCoins) / (myCoins + otherCoins + 1);
    int remCoins = BoardSize * BoardSize - cnt;
    int eP = remCoins % 2 ? 1 : -1;

    if(cnt < 20) {
        return 1000 * evaluateCorner(board, turn) + 50 * evaluateMobility(board, turn) + 5 * stab;
    }
    else if(cnt <= 58) {
        return 1000 * evaluateCorner(board, turn) + 20 * evaluateMobility(board, turn) + 10 * eCD + 100 * eP + 50 * stab;
    }
    else {
        return 1000 * evaluateCorner(board, turn) + 100 * evaluateMobility(board, turn) + 500 * eCD + 500 * eP + 500 * stab;   
    }
}

// alpha-beta pruning with 4-ply search
int alphaBeta(OthelloBoard curBoard, int alpha, int beta, Turn curTurn, int curDepth, Turn turn) {
    
    if((curDepth == plyDepth))
        return evalFunc(curBoard, turn);

    list<Move> LVM = curBoard.getValidMoves(curTurn);

    int lSz = LVM.size();

    if(lSz == 0) {
        return evalFunc(curBoard, turn);
    }

    else if(curTurn == turn) {
        auto it = LVM.begin();
        for(int i = 0; i < lSz; i++, it++) {
            OthelloBoard newBoard = curBoard;
            newBoard.makeMove(curTurn, *it);

            alpha = max(alpha, alphaBeta(newBoard, alpha, beta, other(curTurn), curDepth + 1, turn));
            if(alpha >= beta)
                return beta;
            if(i == lSz - 1)
                return alpha;
        }
        return INT_MAX;
    }
    else {
        auto it = LVM.begin();
        for(int i = 0; i < lSz; i++, it++) {
            OthelloBoard newBoard = curBoard;
            newBoard.makeMove(curTurn, *it);

            beta = min(beta, alphaBeta(newBoard, alpha, beta, other(curTurn), curDepth + 1, turn));
            if(alpha >= beta)
                return alpha;
            if(i == lSz - 1)
                return beta;
        }
        return INT_MIN;
    }
}

Move MyBot::play( const OthelloBoard& board )
{   
    Move fiMove(-1, -1);
    int val = INT_MIN;

    list<Move> posMoves = board.getValidMoves(turn);
    int numMoves = posMoves.size();
    auto it = posMoves.begin();
    for(int i = 0; i < numMoves; i++, it++) {
        OthelloBoard newBoard = board;
        newBoard.makeMove(turn, *it);

        int vith = alphaBeta(newBoard, INT_MIN, INT_MAX, other(turn), 0, turn);
        if(vith > val) {
            val = vith;
            fiMove = *it;
        }
    }
    return fiMove;
}

// The following lines are _very_ important to create a bot module for Desdemona

extern "C" {
    OthelloPlayer* createBot( Turn turn )
    {
        return new MyBot( turn );
    }

    void destroyBot( OthelloPlayer* bot )
    {
        delete bot;
    }
}