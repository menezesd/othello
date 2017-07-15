DECLARE FUNCTION AlphaBeta% (player%, board%(), achievable%, cutoff%, ply%)
DECLARE FUNCTION FinalValue% (player%, board%())
DECLARE FUNCTION WeightedSquares% (player%, board%())
DECLARE FUNCTION MaximizeDifference% (player%, board%())
DECLARE FUNCTION CountDifference% (player%, board%())
DECLARE SUB MakeMove (move%, player%, board%())
DECLARE SUB InitBoard ()
DECLARE FUNCTION LegalP% (move%, player%, board%())
DECLARE FUNCTION WouldFlip% (move%, player%, board%(), dir%)
DECLARE FUNCTION FindBracketingPiece% (square%, player%, board%(), dir%)
DECLARE FUNCTION NextToPlay% (board%(), PreviousPlayer%)
DECLARE FUNCTION AnyLegalMove% (player%, board%())
DECLARE FUNCTION Opponent% (player%)
DECLARE SUB MakeFlips (move%, player%, board%(), dir%)
DEFINT A-Z
DECLARE SUB Mouse (ax AS INTEGER)
DECLARE FUNCTION GetMove% ()
DECLARE FUNCTION Colour (i)
DECLARE SUB ShowBd ()
DIM SHARED board(100) AS INTEGER
DIM SHARED AllDirections(8) AS INTEGER
DIM SHARED weights(100) AS INTEGER
CLEAR , , 9999
FOR i = 1 TO 8
  READ AllDirections(i)
NEXT
DATA -11, -10, -9, -1, 1, 9, 10, 11

FOR i = 0 TO 99
  READ weights(i)
NEXT
DATA 0,0,0,0,0,0,0,0,0,0
DATA 0,120,-20,20,5,5,20,-20,120,0
DATA 0,-20,-0,-5,-5,-5,-5,-40,-20,0
DATA 0,20,-5,15,3,3,15,-5,20,0
DATA 0,5,-5,3,3,3,3,-5,5,0
DATA 0,5,-5,3,3,3,3,-5,5,0
DATA 0,20,-5,15,3,3,15,-5,20,0
DATA 0,-20,-0,-5,-5,-5,-5,-40,-20,0
DATA 0,120,-20,20,5,5,20,-20,120,0
DATA 0,0,0,0,0,0,0,0,0,0

CONST ScreenWidth = 640
CONST ScreenHeight = 480
CONST SquareWidth = ScreenHeight / 8
CONST tlx = (ScreenWidth - ScreenHeight) / 2
CONST tly = 0
CONST brx = ScreenWidth - (ScreenWidth - ScreenHeight) / 2
CONST bry = ScreenHeight
CONST PieceRadius = SquareWidth / 2 - 5
CONST EMPTY = 0
CONST BLACK = 1
CONST WHITE = 2
CONST OUTER = 3
CONST WinningValue = 32767
CONST LosingValue = -32767
CONST nply = 5

DIM SHARED bestm(nply) AS INTEGER
DIM SHARED ax AS INTEGER
ax = 0
DIM SHARED bx AS INTEGER
bx = 0
DIM SHARED cx AS INTEGER
cx = 0
DIM SHARED dx AS INTEGER
dx = 0



SCREEN 12
CALL InitBoard
CALL Mouse(0)
CALL Mouse(1)
player = BLACK
human = BLACK
computer = Opponent(human)
DO
  CALL ShowBd
  IF player = human THEN
    n = GetMove
    IF LegalP(n, player, board()) THEN
      CALL MakeMove(n, player, board())
      player = NextToPlay(board(), player)
    END IF
  END IF
  CALL ShowBd
  IF player = computer THEN
    n = AlphaBeta(player, board(), LosingValue, WinningValue, nply)
    move = bestm(nply)
    CALL MakeMove(move, player, board())
    player = NextToPlay(board(), player)
  END IF
LOOP UNTIL player = 0

FUNCTION AlphaBeta (player, board(), achievable, cutoff, ply)
  DIM board2(100)
  FOR i = 0 TO 99
    board2(i) = board(i)
  NEXT
  IF ply = 0 THEN
    AlphaBeta = WeightedSquares(player, board())
    EXIT FUNCTION
  END IF
  nlegal = 0
  FOR move = 0 TO 99
    IF LegalP(move, player, board()) THEN
      nlegal = nlegal + 1
      CALL MakeMove(move, player, board2())
      value = -AlphaBeta(Opponent(player), board2(), -cutoff, -achievable, ply - 1)
      IF value > achievable THEN
        achievable = value
        bestmove = move
      END IF
    END IF
  NEXT
  IF nlegal = 0 THEN
    IF AnyLegalMove(Opponent(player), board()) THEN
      AlphaBeta = -AlphaBeta(Opponent(player), board(), -cutoff, -achievable, ply - 1)
    ELSE
      AlphaBeta = FinalValue(player, board())
    END IF
  END IF
  bestm(ply) = bestmove
END FUNCTION

FUNCTION AnyLegalMove (player, board())
  FOR i = 0 TO 99
    IF LegalP(i, player, board()) THEN AnyLegalMove = -1: EXIT FUNCTION
  NEXT
END FUNCTION

FUNCTION Colour (i)
  IF i = BLACK THEN
    Colour = 0
  ELSE
    Colour = 15
  END IF
END FUNCTION

FUNCTION CountDifference (player, board())
  c = 0
  FOR y = 1 TO 8
    FOR x = 1 TO 8
      IF board(10 * y + x) = player THEN c = c + 1
      IF board(10 * y + x) = Opponent(player) THEN c = c - 1
    NEXT
  NEXT
  CountDifference = c
END FUNCTION

FUNCTION FinalValue (player, board())
  SELECT CASE SGN(CountDifference(player, board()))
    CASE -1
      FinalValue = LosingValue
    CASE 0
      FinalValue = 0
    CASE 1
      FinalValue = WinningValue
  END SELECT
END FUNCTION

FUNCTION FindBracketingPiece (square, player, board(), dir)
  IF board(square) = player THEN
    FindBracketingPiece = square
  ELSEIF board(square) = Opponent(player) THEN
    FindBracketingPiece = FindBracketingPiece(square + dir, player, board(), dir)
  END IF
END FUNCTION

FUNCTION GetMove
  DO
    CALL Mouse(3)
    IF bx AND 1 THEN
      y = (dx - tly) \ SquareWidth + 1
      x = (cx - tlx) \ SquareWidth + 1
    END IF
    IF 1 <= y AND y <= 8 AND 1 <= x AND x <= 8 THEN EXIT DO
  LOOP
  GetMove = 10 * y + x
END FUNCTION

SUB InitBoard
  FOR i = 0 TO 9
    board(i) = OUTER
    board(90 + i) = OUTER
    board(i * 10) = OUTER
    board(i * 10 + 9) = OUTER
  NEXT
  board(44) = 1
  board(45) = 2
  board(54) = 2
  board(55) = 1
END SUB

FUNCTION LegalP (move, player, board())
  IF board(move) <> EMPTY THEN LegalP = 0: EXIT FUNCTION
  FOR i = 1 TO 8
    x = WouldFlip(move, player, board(), AllDirections(i))
    IF x THEN LegalP = -1: EXIT FUNCTION
  NEXT
END FUNCTION

SUB MakeFlips (move, player, board(), dir)
  bracketer = WouldFlip(move, player, board(), dir)
  IF bracketer THEN
    FOR c = move + dir TO bracketer STEP dir
      board(c) = player
    NEXT
  END IF
END SUB

SUB MakeMove (move, player, board())
  board(move) = player
  FOR i = 1 TO 8
    CALL MakeFlips(move, player, board(), AllDirections(i))
  NEXT
END SUB

FUNCTION MaximizeDifference (player, board())
  DIM board2(100)
  best = -9999
  FOR y = 1 TO 8
    FOR x = 1 TO 8
      move = 10 * y + x
      IF LegalP(move, player, board()) THEN
        FOR i = 0 TO 99
          board2(i) = board(i)
        NEXT
        CALL MakeMove(move, player, board2())
        score = WeightedSquares(player, board2())
        IF score > best THEN best = score: bestmove = move
      END IF
    NEXT
  NEXT
  MaximizeDifference = bestmove
END FUNCTION

SUB Mouse (ax AS INTEGER)
  
  ml$ = "" ' -=<( Mouse Code )>=-
  ml$ = ml$ + CHR$(&H55) ' push bp               ; preserve BP register
  ml$ = ml$ + CHR$(&H89) + CHR$(&HE5) ' mov  bp, sp           ; copy SP to BP
  ml$ = ml$ + CHR$(&HB8) + CHR$(ax) + CHR$(&H0) '   mov  ax, #          ;   copy SUBFUNCTION to AX
  ml$ = ml$ + CHR$(&HCD) + CHR$(&H33) '   int  33             ;   call mouse interrupt
  ml$ = ml$ + CHR$(&H53) '   push bx             ;   preserve BX (again)
  ml$ = ml$ + CHR$(&H8B) + CHR$(&H5E) + CHR$(&H6) '   mov  bx, [bp+6]     ;   copy location of dx (last variable) to BX
  ml$ = ml$ + CHR$(&H89) + CHR$(&H17) '   mov  [bx], dx       ;   copy DX to dx location in BX
  ml$ = ml$ + CHR$(&H8B) + CHR$(&H5E) + CHR$(&H8) '   mov  bx, [bp+8]     ;   copy location of cx to BX
  ml$ = ml$ + CHR$(&H89) + CHR$(&HF) '   mov  [bx], cx       ;   copy CX to cx location in BX
  ml$ = ml$ + CHR$(&H8B) + CHR$(&H5E) + CHR$(&HC) '   mov  bx, [bp+C]     ;   copy location of ax to BX
  ml$ = ml$ + CHR$(&H89) + CHR$(&HF7) '   mov  [bx], ax       ;   copy AX to ax location in BX
  ml$ = ml$ + CHR$(&H8B) + CHR$(&H5E) + CHR$(&HA) '   mov  bx, [bp+A]     ;   copy location of bx to BX
  ml$ = ml$ + CHR$(&H58) '   pop  ax             ;   restore int 33's BX value to AX
  ml$ = ml$ + CHR$(&H89) + CHR$(&H7) '   mov  [bx], ax       ;   copy AX to bx location in BX
  ml$ = ml$ + CHR$(&H5D) ' pop  bp               ; restore BP
  ml$ = ml$ + CHR$(&HCA) + CHR$(&H8) + CHR$(&H0) ' retf 8                ; Return Far and skip 8 bytes of variables
  DEF SEG = SSEG(ml$) ' Set current segment this machine code segment
  offset% = SADD(ml$) ' Set offset to this machine code location
  CALL absolute(ax, bx, cx, dx, offset%) ' The actual call to this machine code
  DEF SEG ' Restore the default segment
  
END SUB

FUNCTION NextToPlay (board(), PreviousPlayer)
  opp = Opponent(PreviousPlayer)
  IF AnyLegalMove(opp, board()) THEN NextToPlay = opp: EXIT FUNCTION
  IF AnyLegalMove(PreviousPlayer, board()) THEN NextToPlay = PreviousPlayer
END FUNCTION

FUNCTION Opponent (player)
  IF player = WHITE THEN Opponent = BLACK
  IF player = BLACK THEN Opponent = WHITE
END FUNCTION

SUB ShowBd
  LINE (tlx, tly)-(brx, bry), 8, BF
  FOR i = 0 TO 8
    LINE (tlx + SquareWidth * i, tly)-(tlx + SquareWidth * i, bry)
    LINE (tlx, tly + SquareWidth * i)-(brx, tly + SquareWidth * i)
  NEXT
  FOR y = 1 TO 8
    FOR x = 1 TO 8
      IF board(10 * y + x) <> EMPTY THEN
        CIRCLE (tlx + SquareWidth * x - SquareWidth / 2, tly + SquareWidth * y - SquareWidth / 2), PieceRadius, Colour(board(10 * y + x))
        PAINT (tlx + SquareWidth * x - SquareWidth / 2, tly + SquareWidth * y - SquareWidth / 2), Colour(board(10 * y + x))
      END IF
    NEXT
  NEXT
END SUB

FUNCTION WeightedSquares (player, board())
  opp = Opponent(player)
  sum = 0
  FOR i = 0 TO 99
    IF board(i) = player THEN sum = sum + weights(i)
    IF board(i) = opp THEN sum = sum - weights(i)
  NEXT
  WeightedSquares = sum
END FUNCTION

FUNCTION WouldFlip (move, player, board(), dir)
  c = move + dir
  IF board(c) <> Opponent(player) THEN WouldFlip = 0: EXIT FUNCTION
  WouldFlip = FindBracketingPiece(c + dir, player, board(), dir)
END FUNCTION

