#define NDEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

/*
	Chinese chess board is 10 x 9,
	to speed up rules checking, I added 2 lines for both the top, left, bottom and right sides.
*/
#define BOARD_ROW_LEN 14
#define BOARD_COL_LEN 13
#define BOARD_ACTUAL_ROW_LEN 10
#define BOARD_ACTUAL_COL_LEN 9
#define BOARD_ACTUAL_ROW_BEGIN 2
#define BOARD_ACTUAL_COL_BEGIN 2

/* if a pawn has crossed the faced river, then he can move forward, left or right. */
#define BOARD_RIVER_UP    (BOARD_ACTUAL_ROW_BEGIN + 4)
#define BOARD_RIVER_DOWN  (BOARD_ACTUAL_ROW_BEGIN + 5)

/* piece: general and advisor must stay within the 9 palace. */
#define BOARD_9_PALACE_UP_TOP     (BOARD_ACTUAL_ROW_BEGIN)
#define BOARD_9_PALACE_UP_BOTTOM  (BOARD_ACTUAL_ROW_BEGIN + 2)
#define BOARD_9_PALACE_UP_LEFT    (BOARD_ACTUAL_COL_BEGIN + 3)
#define BOARD_9_PALACE_UP_RIGHT   (BOARD_ACTUAL_COL_BEGIN + 5)

#define BOARD_9_PALACE_DOWN_TOP     (BOARD_ACTUAL_ROW_BEGIN + 7)
#define BOARD_9_PALACE_DOWN_BOTTOM  (BOARD_ACTUAL_ROW_BEGIN + 9)
#define BOARD_9_PALACE_DOWN_LEFT    (BOARD_ACTUAL_COL_BEGIN + 3)
#define BOARD_9_PALACE_DOWN_RIGHT   (BOARD_ACTUAL_COL_BEGIN + 5)

/* 1024 means 512 rounds, I think this is big enough for one game. */
#define MAX_HISOTRY_BUF_LEN 1024

/* The max number of steps a player can take in a single turn. */
#define MAX_ONE_SIDE_POSSIBLE_MOVES_LEN 256

/* user input string length limitation. */
#define MAX_USER_INPUT_BUFFER_LEN 8

/* the length of buffer for converting a move to string. */
#define MOVE_TO_STR_BUFFER_LEN 5

/* compare max and min macros, never use function as parameter, otherwise the function will be called twice. */
#define COMPARE_MAX(left, right) ((left) > (right) ? (left) : (right))
#define COMPARE_MIN(left, right) ((left) < (right) ? (left) : (right))

/* piece side. */
enum PieceSide{
    PS_UP,         /* upper side player. */
    PS_DOWN,       /* down side player. */
    PS_EXTRA       /* neither upper nor down, for example: empty or out of chess board. */
};

/* piece type. */
enum PieceType{
    PT_PAWN,       /* pawn. */
    PT_CANNON,     /* cannon. */
    PT_ROOK,       /* rook. */
    PT_KNIGHT,     /* knight. */
    PT_BISHOP,     /* bishop. */
    PT_ADVISOR,    /* advisor. */
    PT_GENERAL,    /* general. */
    PT_EMPTY,      /* empty here. */
    PT_OUT         /* out of chess board. */
};

/* piece. */
enum Piece{
    P_UP,              /* upper pawn. */
    P_UC,              /* upper cannon. */
    P_UR,              /* upper rook. */
    P_UN,              /* upper knight. */
    P_UB,              /* upper bishop. */
    P_UA,              /* upper advisor. */
    P_UG,              /* upper general. */
    P_DP,              /* down pawn. */
    P_DC,              /* down cannon. */
    P_DR,              /* down rook. */
    P_DN,              /* down knight. */
    P_DB,              /* down bishop. */
    P_DA,              /* down advisor. */
    P_DG,              /* down general. */
    P_EE,              /* empty. */
    P_EO,              /* out of chess board. used for speeding up rules checking. */
    PIECE_TOTAL_LEN    /* total number of pieces, you maybe never need this. */
};

static const char piece_get_char[] = {
    /* cause #include<> just copy the content, so why not write those things to a txt file rather than writing in code ? */
    #include "chessBoardPieceChar.txt"
};

static const enum PieceSide piece_get_side[] = {
    PS_UP, PS_UP, PS_UP, PS_UP, PS_UP, PS_UP, PS_UP,
    PS_DOWN, PS_DOWN, PS_DOWN, PS_DOWN, PS_DOWN, PS_DOWN, PS_DOWN,
    PS_EXTRA, PS_EXTRA
};

static const enum PieceType piece_get_type[] = {
    PT_PAWN, PT_CANNON, PT_ROOK, PT_KNIGHT, PT_BISHOP, PT_ADVISOR, PT_GENERAL,
    PT_PAWN, PT_CANNON, PT_ROOK, PT_KNIGHT, PT_BISHOP, PT_ADVISOR, PT_GENERAL,
    PT_EMPTY, PT_OUT
};

static const enum PieceSide piece_side_get_reverse_side[] = {
    PS_DOWN,     /* upper side reverse is down. */
    PS_UP,       /* down side reverse is up. */
    PS_EXTRA     /* extra remains extra. */
};

/*
	every piece's value. 
	upper side piece's value is negative, down side is positive. 
*/
static const int piece_get_value[] = {
    #include "chessBoardPieceValue.txt"
};

/* 
	every piece's position value on the chess board. 
	upper side piece's value is negative, down side is positive. 
*/
static const int piece_get_pos_value[][BOARD_ACTUAL_ROW_LEN][BOARD_ACTUAL_COL_LEN] = {
    #include "chessBoardPosValue.txt"
};

/* 
    a default chess board, used as a template for new board.
    P_EO is used here for speeding up rules checking.
*/
static const enum Piece CHESS_BOARD_DEFAULT_TEMPLATE[BOARD_ROW_LEN][BOARD_COL_LEN] = {
    { P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO },
    { P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO },
    { P_EO, P_EO, P_UR, P_UN, P_UB, P_UA, P_UG, P_UA, P_UB, P_UN, P_UR, P_EO, P_EO },
    { P_EO, P_EO, P_EE, P_EE, P_EE, P_EE, P_EE, P_EE, P_EE, P_EE, P_EE, P_EO, P_EO },
    { P_EO, P_EO, P_EE, P_UC, P_EE, P_EE, P_EE, P_EE, P_EE, P_UC, P_EE, P_EO, P_EO },
    { P_EO, P_EO, P_UP, P_EE, P_UP, P_EE, P_UP, P_EE, P_UP, P_EE, P_UP, P_EO, P_EO },
    { P_EO, P_EO, P_EE, P_EE, P_EE, P_EE, P_EE, P_EE, P_EE, P_EE, P_EE, P_EO, P_EO },
    { P_EO, P_EO, P_EE, P_EE, P_EE, P_EE, P_EE, P_EE, P_EE, P_EE, P_EE, P_EO, P_EO },
    { P_EO, P_EO, P_DP, P_EE, P_DP, P_EE, P_DP, P_EE, P_DP, P_EE, P_DP, P_EO, P_EO },
    { P_EO, P_EO, P_EE, P_DC, P_EE, P_EE, P_EE, P_EE, P_EE, P_DC, P_EE, P_EO, P_EO },
    { P_EO, P_EO, P_EE, P_EE, P_EE, P_EE, P_EE, P_EE, P_EE, P_EE, P_EE, P_EO, P_EO },
    { P_EO, P_EO, P_DR, P_DN, P_DB, P_DA, P_DG, P_DA, P_DB, P_DN, P_DR, P_EO, P_EO },
    { P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO },
    { P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO, P_EO },
};

/* move node, reprensent a move. */
struct MoveNode{
    int beginRow;
    int beginCol;
    int endRow;
    int endCol;
};

/* history node, used for undo the previous move. */
struct HistoryNode{
    struct MoveNode move;
    enum Piece beginPiece;
    enum Piece endPiece;
};

/* chess board. */
struct ChessBoard{
    enum Piece data[BOARD_ROW_LEN][BOARD_COL_LEN];
    struct HistoryNode history[MAX_HISOTRY_BUF_LEN];
    size_t historyLength;
};

/* possible moves. */
struct PossibleMoves{
    struct MoveNode data[MAX_ONE_SIDE_POSSIBLE_MOVES_LEN];
    size_t len;
};

/* 
    wrapper on malloc().
    if out of memory, then log the error and exit the program. 
    you should call free() on the returned value later.
*/
static void* safe_malloc(size_t size){
    void* buffer = malloc(size);
    if (buffer == NULL){
        fprintf(stderr, "%s: %s(%d) error: malloc() out of memory\n", __FILE__, __FUNCTION__, __LINE__);
        exit(EXIT_FAILURE);
    }

    return buffer;
}

/* 
    making a new chess board. 
    you should call free() on the returned value later.
*/
static struct ChessBoard* board_make_new(void){
    struct ChessBoard* cb = (struct ChessBoard*)safe_malloc(sizeof(struct ChessBoard));
    memcpy(cb->data, &CHESS_BOARD_DEFAULT_TEMPLATE, BOARD_ROW_LEN * BOARD_COL_LEN * sizeof(enum Piece));
    cb->historyLength = 0;

    return cb;
}

static void board_print_to_console(const struct ChessBoard* cb){
    assert(cb != NULL);

    int splitLineIndex = BOARD_ACTUAL_ROW_BEGIN + (BOARD_ACTUAL_ROW_LEN / 2);
    int endRow = BOARD_ACTUAL_ROW_BEGIN + BOARD_ACTUAL_ROW_LEN;
    int endCol = BOARD_ACTUAL_COL_BEGIN + BOARD_ACTUAL_COL_LEN;
    int n = BOARD_ACTUAL_ROW_LEN - 1;

    printf("\n    +-------------------+\n");

    int r, c;
    for (r = BOARD_ACTUAL_ROW_BEGIN; r < endRow; ++r) {
        if (r == splitLineIndex){
            printf("    |===================|\n");
            printf("    |===================|\n");
        }

        printf(" %d  | ", n--);

        for (c = BOARD_ACTUAL_COL_BEGIN; c < endCol; ++c){
            printf("%c ", piece_get_char[(cb->data[r][c])]);
        }

        printf("|\n");
    }

    printf("    +-------------------+\n");
    printf("\n      a b c d e f g h i\n\n");
}

/* record history and move, never check any game rules. */
static void board_move(struct ChessBoard* cb, const struct MoveNode* moveNode){
    assert(cb != NULL && moveNode != NULL);

    enum Piece beginPiece = cb->data[moveNode->beginRow][moveNode->beginCol];
    enum Piece endPiece = cb->data[moveNode->endRow][moveNode->endCol];

    /* first, historyLength too big is considered as a draw, then exit the program. */
    if (cb->historyLength == MAX_HISOTRY_BUF_LEN){
        free(cb);
        printf("Draw: You have been battling with AI for a very long time and have exceeded the max number of rounds in this game.");
        exit(EXIT_SUCCESS);
    }

    /* record the history. */
    struct HistoryNode* currentHistoryNode = &(cb->history[cb->historyLength]);
    memcpy(currentHistoryNode, moveNode, sizeof(struct MoveNode));
    currentHistoryNode->beginPiece = beginPiece;
    currentHistoryNode->endPiece = endPiece;

    ++(cb->historyLength);

    /* move the pieces. */
    cb->data[moveNode->beginRow][moveNode->beginCol] = P_EE;
    cb->data[moveNode->endRow][moveNode->endCol] = beginPiece;
}

/* undo the previous move, if history is empty, do nothing. */
static void board_undo(struct ChessBoard* cb){
    assert(cb != NULL);

    if (cb->historyLength > 0){
        --(cb->historyLength);
        struct HistoryNode* hist = &(cb->history[cb->historyLength]);

        cb->data[hist->move.beginRow][hist->move.beginCol] = hist->beginPiece;
        cb->data[hist->move.endRow][hist->move.endCol] = hist->endPiece;
    }
}

static void possible_move_insert(struct PossibleMoves* pm, int beginRow, int beginCol, int endRow, int endCol){
    assert(pm != NULL);

    struct MoveNode* currentMoveNode = &(pm->data[pm->len]);

    currentMoveNode->beginRow = beginRow;
    currentMoveNode->beginCol = beginCol;
    currentMoveNode->endRow = endRow;
    currentMoveNode->endCol = endCol;

    ++(pm->len);
}

static void board_try_insert_possible_move(struct ChessBoard* cb, struct PossibleMoves* pm, int beginRow, int beginCol, int endRow, int endCol){
    assert(cb != NULL && pm != NULL);

    enum Piece beginP = cb->data[beginRow][beginCol];
    enum Piece endP = cb->data[endRow][endCol];

    if (endP != P_EO && piece_get_side[beginP] != piece_get_side[endP]){   /* not out of chess board, and not the same side. */
        possible_move_insert(pm, beginRow, beginCol, endRow, endCol);
    }
}

static void board_gen_possible_moves_for_pawn(struct ChessBoard* cb, struct PossibleMoves* pm, int r, int c, enum PieceSide side){
	assert(cb != NULL && pm != NULL);

    if (side == PS_UP){
        board_try_insert_possible_move(cb, pm,  r, c, r + 1, c);

        if (r > BOARD_RIVER_UP){    /* cross the river ? */
            board_try_insert_possible_move(cb, pm, r, c, r, c - 1);
            board_try_insert_possible_move(cb, pm, r, c, r, c + 1);
        }
    }
    else if (side == PS_DOWN){
        board_try_insert_possible_move(cb, pm, r, c, r - 1, c);

        if (r < BOARD_RIVER_DOWN){
            board_try_insert_possible_move(cb, pm, r, c, r, c - 1);
            board_try_insert_possible_move(cb, pm, r, c, r, c + 1);
        }
    }
}

static void board_gen_possible_moves_for_cannon_one_direction(struct ChessBoard* cb, struct PossibleMoves* pm, int r, int c, int rGap, int cGap, enum PieceSide side){
    assert(cb != NULL && pm != NULL);

    int row, col;
    enum Piece p;

    for (row = r + rGap, col = c + cGap; ;row += rGap, col += cGap){
        p = cb->data[row][col];

        if (p == P_EE){    /* empty piece, then insert it. */
            possible_move_insert(pm, r, c, row, col);
        }
        else {   /* upper piece, down piece or out of chess board, break immediately. */
            break;
        }
    }

    if (p != P_EO){   /* not out of chess board, check if we can add an enemy piece. */
        for (row = row + rGap, col = col + cGap; ;row += rGap, col += cGap){
            p = cb->data[row][col];
        
            if (p == P_EE){    /* empty, then continue search. */
                continue;
            }
            else if (piece_get_side[p] == piece_side_get_reverse_side[side]){   /* enemy piece, then insert it and break. */
                possible_move_insert(pm, r, c, row, col);
                break;
            }
            else {    /* self side piece or out of chess board, break. */
                break;
            }
        }
    }
}

static void board_gen_possible_moves_for_cannon(struct ChessBoard* cb, struct PossibleMoves* pm, int r, int c, enum PieceSide side){
	assert(cb != NULL && pm != NULL);

    /* go up, down, left, right. */
    board_gen_possible_moves_for_cannon_one_direction(cb, pm, r, c, -1, 0, side);
    board_gen_possible_moves_for_cannon_one_direction(cb, pm, r, c, +1, 0, side);
    board_gen_possible_moves_for_cannon_one_direction(cb, pm, r, c, 0, -1, side);
    board_gen_possible_moves_for_cannon_one_direction(cb, pm, r, c, 0, +1, side);
}

static void board_gen_possible_moves_for_rook_one_direction(struct ChessBoard* cb, struct PossibleMoves* pm, int r, int c, int rGap, int cGap, enum PieceSide side){
    assert(cb != NULL && pm != NULL);

    int row, col;
    enum Piece p;

    for (row = r + rGap, col = c + cGap; ;row += rGap, col += cGap){
        p = cb->data[row][col];

        if (p == P_EE){    /* empty piece, then insert it. */
            possible_move_insert(pm, r, c, row, col);
        }
        else {   /* upper piece, down piece or out of chess board, break immediately. */
            break;
        }
    }

    if (piece_get_side[p] == piece_side_get_reverse_side[side]){   /* enemy piece, then insert it. */
        possible_move_insert(pm, r, c, row, col);
    }
}

static void board_gen_possible_moves_for_rook(struct ChessBoard* cb, struct PossibleMoves* pm, int r, int c, enum PieceSide side){
	assert(cb != NULL && pm != NULL);

    /* go up, down, left, right. */
    board_gen_possible_moves_for_rook_one_direction(cb, pm, r, c, -1, 0, side);
    board_gen_possible_moves_for_rook_one_direction(cb, pm, r, c, +1, 0, side);
    board_gen_possible_moves_for_rook_one_direction(cb, pm, r, c, 0, -1, side);
    board_gen_possible_moves_for_rook_one_direction(cb, pm, r, c, 0, +1, side);
}

static void board_gen_possible_moves_for_knight(struct ChessBoard* cb, struct PossibleMoves* pm, int r, int c, enum PieceSide side){
	assert(cb != NULL && pm != NULL);

    enum Piece p;
    if ((p = cb->data[r + 1][c]) == P_EE){    /* if not lame horse leg ? */
        board_try_insert_possible_move(cb, pm, r, c, r + 2, c + 1);
        board_try_insert_possible_move(cb, pm, r, c, r + 2, c - 1);
    }

    if ((p = cb->data[r - 1][c]) == P_EE){
        board_try_insert_possible_move(cb, pm, r, c, r - 2, c + 1);
        board_try_insert_possible_move(cb, pm, r, c, r - 2, c - 1);
    }

    if ((p = cb->data[r][c + 1]) == P_EE){
        board_try_insert_possible_move(cb, pm, r, c, r + 1, c + 2);
        board_try_insert_possible_move(cb, pm, r, c, r - 1, c + 2);
    }

    if ((p = cb->data[r][c - 1]) == P_EE){
        board_try_insert_possible_move(cb, pm, r, c, r + 1, c - 2);
        board_try_insert_possible_move(cb, pm, r, c, r - 1, c - 2);
    }
}

static void board_gen_possible_moves_for_bishop(struct ChessBoard* cb, struct PossibleMoves* pm, int r, int c, enum PieceSide side){
	assert(cb != NULL && pm != NULL);

    enum Piece p;
    if (side == PS_UP){
        if (r + 2 <= BOARD_RIVER_UP){       /* bishop can't cross river. */
            if ((p = cb->data[r + 1][c + 1]) == P_EE){    /* bishop can move only if Xiang Yan is empty. */
                board_try_insert_possible_move(cb, pm, r, c, r + 2, c + 2);
            }

            if ((p = cb->data[r + 1][c - 1]) == P_EE){
                board_try_insert_possible_move(cb, pm, r, c, r + 2, c - 2);
            }
        }

        if ((p = cb->data[r - 1][c + 1]) == P_EE){
            board_try_insert_possible_move(cb, pm, r, c, r - 2, c + 2);
        }

        if ((p = cb->data[r - 1][c - 1]) == P_EE){
            board_try_insert_possible_move(cb, pm, r, c, r - 2, c - 2);
        }
    }
    else if (side == PS_DOWN){
        if (r - 2 >= BOARD_RIVER_DOWN){
            if ((p = cb->data[r - 1][c + 1]) == P_EE){
                board_try_insert_possible_move(cb, pm, r, c, r - 2, c + 2);
            }

            if ((p = cb->data[r - 1][c - 1]) == P_EE){
                board_try_insert_possible_move(cb, pm, r, c, r - 2, c - 2);
            }
        }

        if ((p = cb->data[r + 1][c + 1]) == P_EE){
            board_try_insert_possible_move(cb, pm, r, c, r + 2, c + 2);
        }

        if ((p = cb->data[r + 1][c - 1]) == P_EE){
            board_try_insert_possible_move(cb, pm, r, c, r + 2, c - 2);
        }
    }
}

static void board_gen_possible_moves_for_advisor(struct ChessBoard* cb, struct PossibleMoves* pm, int r, int c, enum PieceSide side){
	assert(cb != NULL && pm != NULL);

    if (side == PS_UP){
        if (r + 1 <= BOARD_9_PALACE_UP_BOTTOM && c + 1 <= BOARD_9_PALACE_UP_RIGHT) {   /* walk diagonal lines. */
            board_try_insert_possible_move(cb, pm, r, c, r + 1, c + 1);
        }

        if (r + 1 <= BOARD_9_PALACE_UP_BOTTOM && c - 1 >= BOARD_9_PALACE_UP_LEFT) {
            board_try_insert_possible_move(cb, pm, r, c, r + 1, c - 1);
        }

        if (r - 1 >= BOARD_9_PALACE_UP_TOP && c + 1 <= BOARD_9_PALACE_UP_RIGHT) {
            board_try_insert_possible_move(cb, pm, r, c, r - 1, c + 1);
        }

        if (r - 1 >= BOARD_9_PALACE_UP_TOP && c - 1 >= BOARD_9_PALACE_UP_LEFT) {
            board_try_insert_possible_move(cb, pm, r, c, r - 1, c - 1);
        }
    }
    else if (side == PS_DOWN){
        if (r + 1 <= BOARD_9_PALACE_DOWN_BOTTOM && c + 1 <= BOARD_9_PALACE_DOWN_RIGHT) {
            board_try_insert_possible_move(cb, pm, r, c, r + 1, c + 1);
        }

        if (r + 1 <= BOARD_9_PALACE_DOWN_BOTTOM && c - 1 >= BOARD_9_PALACE_DOWN_LEFT) {
            board_try_insert_possible_move(cb, pm, r, c, r + 1, c - 1);
        }

        if (r - 1 >= BOARD_9_PALACE_DOWN_TOP && c + 1 <= BOARD_9_PALACE_DOWN_RIGHT) {
            board_try_insert_possible_move(cb, pm, r, c, r - 1, c + 1);
        }

        if (r - 1 >= BOARD_9_PALACE_DOWN_TOP && c - 1 >= BOARD_9_PALACE_DOWN_LEFT) {
            board_try_insert_possible_move(cb, pm, r, c, r - 1, c - 1);
        }
    }
}

static void board_gen_possible_moves_for_general(struct ChessBoard* cb, struct PossibleMoves* pm, int r, int c, enum PieceSide side){
	assert(cb != NULL && pm != NULL);
    enum Piece p;
    int row;

    if (side == PS_UP){
        if (r + 1 <= BOARD_9_PALACE_UP_BOTTOM){   /* walk horizontal or vertical. */
            board_try_insert_possible_move(cb, pm, r, c, r + 1, c);
        }

        if (r - 1 >= BOARD_9_PALACE_UP_TOP){
            board_try_insert_possible_move(cb, pm, r, c, r - 1, c);
        }

        if (c + 1 <= BOARD_9_PALACE_UP_RIGHT){
            board_try_insert_possible_move(cb, pm, r, c, r, c + 1);
        }

        if (c - 1 >= BOARD_9_PALACE_UP_LEFT){
            board_try_insert_possible_move(cb, pm, r, c, r, c - 1);
        }

        /* check if both generals faced each other directly. */
        for (row = r + 1; row < BOARD_ACTUAL_ROW_BEGIN + BOARD_ACTUAL_ROW_LEN ;++row){
            p = cb->data[row][c];

            if (p == P_EE){
                continue;
            }
            else if (p == P_DG){
                possible_move_insert(pm, r, c, row, c);
                break;
            }
            else {
                break;
            }
        }
    }
    else if (side == PS_DOWN){
        if (r + 1 <= BOARD_9_PALACE_DOWN_BOTTOM){
            board_try_insert_possible_move(cb, pm, r, c, r + 1, c);
        }

        if (r - 1 >= BOARD_9_PALACE_DOWN_TOP){
            board_try_insert_possible_move(cb, pm, r, c, r - 1, c);
        }

        if (c + 1 <= BOARD_9_PALACE_DOWN_RIGHT){
            board_try_insert_possible_move(cb, pm, r, c, r, c + 1);
        }

        if (c - 1 >= BOARD_9_PALACE_DOWN_LEFT){
            board_try_insert_possible_move(cb, pm, r, c, r, c - 1);
        }

        for (row = r - 1; row >= BOARD_ACTUAL_ROW_BEGIN ;--row){
            p = cb->data[row][c];

            if (p == P_EE){
                continue;
            }
            else if (p == P_UG){
                possible_move_insert(pm, r, c, row, c);
                break;
            }
            else {
                break;
            }
        }
    }
}

/* 
    generate possible moves for one side. 
    you should call free() on the returned value later.
*/
static struct PossibleMoves* board_gen_possible_moves(struct ChessBoard* cb, enum PieceSide side){
	assert(cb != NULL);
	
    struct PossibleMoves* pm = (struct PossibleMoves*)safe_malloc(sizeof(struct PossibleMoves));
    pm->len = 0;

    int endRow = BOARD_ACTUAL_ROW_BEGIN + BOARD_ACTUAL_ROW_LEN;
    int endCol = BOARD_ACTUAL_COL_BEGIN + BOARD_ACTUAL_COL_LEN;

    enum Piece p;
    int r, c;
    for (r = BOARD_ACTUAL_ROW_BEGIN; r < endRow; ++r) {
        for (c = BOARD_ACTUAL_COL_BEGIN; c < endCol; ++c){
            p = cb->data[r][c];

            if (piece_get_side[p] == side){
                switch (piece_get_type[p])
                {
                case PT_PAWN:
                    board_gen_possible_moves_for_pawn(cb, pm, r, c, side);
                    break;
                case PT_CANNON:
                    board_gen_possible_moves_for_cannon(cb, pm, r, c, side);
                    break;
                case PT_ROOK:
                    board_gen_possible_moves_for_rook(cb, pm, r, c, side);
                    break;
                case PT_KNIGHT:
                    board_gen_possible_moves_for_knight(cb, pm, r, c, side);
                    break;
                case PT_BISHOP:
                    board_gen_possible_moves_for_bishop(cb, pm, r, c, side);
                    break;
                case PT_ADVISOR:
                    board_gen_possible_moves_for_advisor(cb, pm, r, c, side);
                    break;
                case PT_GENERAL:
                    board_gen_possible_moves_for_general(cb, pm, r, c, side);
                    break;
                case PT_EMPTY:
                case PT_OUT:
                default:
                    break;
                }
            }
        }
    }

    return pm;
}

/* 
	calculate a chess board's score. 
	upper side value is negative, down side is positive.
*/
static long board_calc_score(const struct ChessBoard* cb){
	assert(cb != NULL);
	
    long totalScore = 0;

    int endRow = BOARD_ACTUAL_ROW_BEGIN + BOARD_ACTUAL_ROW_LEN;
    int endCol = BOARD_ACTUAL_COL_BEGIN + BOARD_ACTUAL_COL_LEN;

    enum Piece p;
    int r, c;
    for (r = BOARD_ACTUAL_ROW_BEGIN; r < endRow; ++r) {
        for (c = BOARD_ACTUAL_COL_BEGIN; c < endCol; ++c){
            p = cb->data[r][c];

			if (p != P_EE){
				totalScore += piece_get_value[p];
				totalScore += piece_get_pos_value[p][r - BOARD_ACTUAL_ROW_BEGIN][c - BOARD_ACTUAL_COL_BEGIN];	
            }
        }
    }

    return totalScore;
}

/* min-max algorithm, with alpha-beta pruning. */
static int min_max(struct ChessBoard* cb, unsigned int searchDepth, int alpha, int beta, enum PieceSide side){
    if (searchDepth == 0){
        return board_calc_score(cb);
    }

    int minMaxValue;
    if (side == PS_UP){
        int minValue = INT_MAX;
        struct MoveNode* node;
        struct PossibleMoves* possibleMoves = board_gen_possible_moves(cb, PS_UP);

        int i;
        for (i = 0;i < possibleMoves->len;++i){
            node = &(possibleMoves->data[i]);

            board_move(cb, node);
            minMaxValue = min_max(cb, searchDepth - 1, alpha, beta, PS_DOWN);
            minValue = COMPARE_MIN(minValue, minMaxValue);
            board_undo(cb);

            beta = COMPARE_MIN(beta, minValue);
            if (alpha >= beta){
                break;
            }
        }

        free(possibleMoves);
        return minValue;
    }
    else if (side == PS_DOWN){
        int maxValue = INT_MIN;
        struct MoveNode* node;
        struct PossibleMoves* possibleMoves = board_gen_possible_moves(cb, PS_DOWN);

        int i;
        for (i = 0;i < possibleMoves->len;++i){
            node = &(possibleMoves->data[i]);

            board_move(cb, node);
            minMaxValue = min_max(cb, searchDepth - 1, alpha, beta, PS_UP);
            maxValue = COMPARE_MAX(maxValue, minMaxValue);
            board_undo(cb);

            alpha = COMPARE_MAX(alpha, maxValue);
            if (alpha >= beta){
                break;
            }
        }

        free(possibleMoves);
        return maxValue;
    }
    else {   /* never need this, just for return value. */
        return 0;
    }
}

/* 
    gen best move for one side. 
    searchDepth is used as difficulty rank, the bigger it is, the more time the generation costs.
    give param enum PieceSide: PS_EXTRA to this function is meaningless, you will always get a struct MoveNode object with {0, 0, 0, 0}.
*/
static void board_gen_best_move(struct ChessBoard* cb, enum PieceSide side, unsigned int searchDepth, struct MoveNode* bestMove){
    assert(cb != NULL);

    int i, value;
    struct MoveNode* node;
    struct PossibleMoves* possibleMoves;

    if (side == PS_UP){
        int minValue = INT_MAX;
        possibleMoves = board_gen_possible_moves(cb, PS_UP);

        for (i = 0;i < possibleMoves->len;++i){
            node = &(possibleMoves->data[i]);

            board_move(cb, node);
            value = min_max(cb, searchDepth, INT_MIN, INT_MAX, PS_DOWN);
            board_undo(cb);

            if (value <= minValue){
                minValue = value;
                memcpy(bestMove, node, sizeof(struct MoveNode));
            }
        }

        free(possibleMoves);
    }
    else if (side == PS_DOWN){
        int maxValue = INT_MIN;
        possibleMoves = board_gen_possible_moves(cb, PS_DOWN);

        for (i = 0;i < possibleMoves->len;++i){
            node = &(possibleMoves->data[i]);

            board_move(cb, node);
            value = min_max(cb, searchDepth, INT_MIN, INT_MAX, PS_UP);
            board_undo(cb);

            if (value >= maxValue){
                maxValue = value;
                memcpy(bestMove, node, sizeof(struct MoveNode));
            }
        }

        free(possibleMoves);
    }
    else {
        memset(bestMove, 0, sizeof(struct MoveNode));
    }
}

/*
    get a user input line.
    if the length of the user input exceed the given param len,
    then the exceed parts will be ignored, like this:

    limits: 5

    abcdefg
    abcd
*/
static int get_line(char* buf, size_t len){
	assert(buf != NULL && len != 0);
	
    int count = 0;
    char c;

    while(1) {
        c = getchar();
        if(c == '\n' || c == EOF){
            break;
        }

        if (count < len - 1){
            buf[count] = c;
            ++count;
        }
    }

    buf[count] = '\0';
    return count;
}

/* given move is fit for rule ? return 0 if not. */
static int check_rule(struct ChessBoard* cb, const struct MoveNode* moveNode){
    assert(cb != NULL && moveNode != NULL);

    int valid = 0;
    enum Piece p = cb->data[moveNode->beginRow][moveNode->beginCol];
    struct PossibleMoves* pm = board_gen_possible_moves(cb, piece_get_side[p]);

    int i;
    struct MoveNode* cursor;
    for (i = 0;i < pm->len;++i){
        cursor = &(pm->data[i]);

        if (memcmp(cursor, moveNode, sizeof(struct MoveNode)) == 0){
            valid = 1;
            break;
        }
    }

    free(pm);
    return valid;
}

/* user input could represent a move ? return 0 if can't. */
static int check_input_is_a_move(char* input, size_t len){
    assert(input == NULL);

    if (len < 4){
        return 0;
    }

    return  (input[0] >= 'a' && input[0] <= 'i') &&
            (input[1] >= '0' && input[1] <= '9') &&
            (input[2] >= 'a' && input[2] <= 'i') &&
            (input[3] >= '0' && input[3] <= '9');
}

/*
    convert user input to a struct MoveNode object.
    you should call check_input_is_a_move() before to make sure this converting is valid.
*/
static void convert_input_to_move(char* input, struct MoveNode* move){
    assert(input != NULL && move != NULL);

    move->beginRow = 9 - ((int)input[1] - (int)'0') + BOARD_ACTUAL_ROW_BEGIN;
    move->beginCol = (int)input[0] - (int)'a' + BOARD_ACTUAL_COL_BEGIN;
    move->endRow = 9 - ((int)input[3] - (int)'0') + BOARD_ACTUAL_ROW_BEGIN;
    move->endCol = (int)input[2] - (int)'a' + BOARD_ACTUAL_COL_BEGIN;
}

/* 
    convert a move to string. 
    len must be bigger or equal to MOVE_TO_STR_BUFFER_LEN, otherwise this function returns 0.
*/
static int convert_move_to_str(const struct MoveNode* move, char* buf, size_t len){
    assert(move != NULL && buf != NULL);

    if (len < MOVE_TO_STR_BUFFER_LEN){
        return 0;
    }

    buf[0] = move->beginCol - BOARD_ACTUAL_COL_BEGIN + 'a';
    buf[1] = 9 - (move->beginRow - BOARD_ACTUAL_ROW_BEGIN) + '0';
    buf[2] = move->endCol - BOARD_ACTUAL_COL_BEGIN + 'a';
    buf[3] = 9 - (move->endRow - BOARD_ACTUAL_ROW_BEGIN) + '0';
    buf[4] = '\0';
    return 1;
}

/* every one can only move his pieces, not the enemy's. */
static int check_is_this_your_piece(const struct ChessBoard* cb, const struct MoveNode* move, enum PieceSide side){
    enum Piece p = cb->data[move->beginRow][move->beginCol];
    return piece_get_side[p] == side;
}

/* if no one wins, return PS_EXTRA. */
static enum PieceSide check_winner(const struct ChessBoard* cb){
    assert(cb != NULL);

    int upAlive = 0;
    int downAlive = 0;

    int r, c;
    for (r = BOARD_9_PALACE_UP_TOP; r <= BOARD_9_PALACE_UP_BOTTOM; ++r) {
        for (c = BOARD_9_PALACE_UP_LEFT; c <= BOARD_9_PALACE_UP_RIGHT; ++c) {
            if (cb->data[r][c] == P_UG) {
                upAlive = 1;
                break;
            }
        }
    }

    for (r = BOARD_9_PALACE_DOWN_TOP; r <= BOARD_9_PALACE_DOWN_BOTTOM; ++r) {
        for (c = BOARD_9_PALACE_DOWN_LEFT; c <= BOARD_9_PALACE_DOWN_RIGHT; ++c) {
            if (cb->data[r][c] == P_DG) {
                downAlive = 1;
                break;
            }
        }
    }

    if (upAlive && downAlive) {
        return PS_EXTRA;
    }
    else if (upAlive) {
        return PS_UP;
    }
    else {
        return PS_DOWN;
    }
}

static void print_help_page(void){
    printf("\n=======================================\n");
    printf("Help Page\n\n");
    printf("    1. help         - this page.\n");
    printf("    2. b2e2         - input like this will be parsed as a move.\n");
    printf("    3. undo         - undo the previous move.\n");
    printf("    4. exit or quit - exit the game.\n");
    printf("    5. remake       - remake the game.\n");
    printf("    6. advice       - give me a best move.\n\n");
    printf("  The characters on the board have the following relationships: \n\n");
    printf("    P -> AI side pawn.\n");
    printf("    C -> AI side cannon.\n");
    printf("    R -> AI side rook.\n");
    printf("    N -> AI side knight.\n");
    printf("    B -> AI side bishop.\n");
    printf("    A -> AI side advisor.\n");
    printf("    G -> AI side general.\n");
    printf("    p -> our pawn.\n");
    printf("    c -> our cannon.\n");
    printf("    r -> our rook.\n");
    printf("    n -> our knight.\n");
    printf("    b -> our bishop.\n");
    printf("    a -> our advisor.\n");
    printf("    g -> our general.\n");
    printf("    . -> no piece here.\n");
    printf("=======================================\n");
    printf("Press any key to continue.\n");

    (void)getchar();
}

#define CNCHESS_AI_SEARCH_DEPTH 4
#define USER_SIDE  PS_DOWN
#define AI_SIDE    PS_UP

int main(){
    struct ChessBoard* cb = board_make_new();
    char userInput[MAX_USER_INPUT_BUFFER_LEN];
    char moveStr[MOVE_TO_STR_BUFFER_LEN];
    struct MoveNode userMove, aiMove, userAdviceMove;

    board_print_to_console(cb);

    while (1){
        printf("Your move: ");
        get_line(userInput, MAX_HISOTRY_BUF_LEN);

        if (strcmp(userInput, "help") == 0){
            print_help_page();
            board_print_to_console(cb);
        }
        else if (strcmp(userInput, "undo") == 0){
            board_undo(cb);
            board_undo(cb);
            board_print_to_console(cb);
        }
        else if (strcmp(userInput, "quit") == 0){
            goto EXIT_CNCHESS;
        }
        else if (strcmp(userInput, "exit") == 0){
            goto EXIT_CNCHESS;
        }
        else if (strcmp(userInput, "remake") == 0){
            free(cb);
            cb = board_make_new();

            printf("New cnchess started.\n");
            board_print_to_console(cb);
            continue;
        }
        else if (strcmp(userInput, "advice") == 0){
            board_gen_best_move(cb, USER_SIDE, CNCHESS_AI_SEARCH_DEPTH, &userAdviceMove);
            convert_move_to_str(&userAdviceMove, moveStr, MOVE_TO_STR_BUFFER_LEN);
            printf("Maybe you can try: %s, piece is %c.\n", moveStr, piece_get_char[cb->data[userAdviceMove.beginRow][userAdviceMove.beginCol]]);
        }
        else{
            if (check_input_is_a_move(userInput, strlen(userInput))){
                convert_input_to_move(userInput, &userMove);
                
                if (!check_is_this_your_piece(cb, &userMove, USER_SIDE)){
                    printf("This piece is not yours, please choose your piece.\n");
                    continue;
                }

                if (check_rule(cb, &userMove)){
                    board_move(cb, &userMove);
                    board_print_to_console(cb);

                    if (check_winner(cb) == USER_SIDE){
                        printf("Congratulations! You win!\n");
                        goto EXIT_CNCHESS;
                    }

                    printf("AI thinking...\n");
                    board_gen_best_move(cb, AI_SIDE, CNCHESS_AI_SEARCH_DEPTH, &aiMove);
                    convert_move_to_str(&aiMove, moveStr, MOVE_TO_STR_BUFFER_LEN);
                    board_move(cb, &aiMove);
                    board_print_to_console(cb);
                    printf("AI move: %s, piece is '%c'.\n", moveStr, piece_get_char[cb->data[aiMove.endRow][aiMove.endCol]]);

                    if (check_winner(cb) == AI_SIDE){
                        printf("Game over! You lose!\n");
                        goto EXIT_CNCHESS;
                    }
                }
                else {
                    printf("Given move does't fit for rules, please re-enter.\n");
                    continue;
                }
            }
            else {
                printf("Input is not a valid move nor instruction, please re-enter(try help ?).\n");
                continue;
            }
        }
    }

EXIT_CNCHESS:
    free(cb);
    return 0;
}
