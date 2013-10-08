'
' FILE: QUEENS.BAS
'
' This program places n queens on a chess table of size n so that no
' queen pairs attack each other.
'
' In this sample the size of the chess table is given by the variable n
' Altering that variable you can calculate the number of solutions for any
' table size. Experiment with it.
'

' the size of the table
' try to change it and experience the result
' be careful setting it too large
n = 8

' this variable holds the actual number of solutions found
SolutionNumber = 1

' this variable holds the actual row in which we try to place a queen
ActualRow = 1

' initially we set the queen before the table and the program
' starts putting it one step forward (onto the table)
QueenPosition[1] = 0

' here we start a loop to find all solutions not only one
repeat

  ' do this loop until all queens are positioned
  while ActualRow <= n

    ' step the queen one step further
    QueenPosition[ActualRow] += 1

    ' if we stepped off the table
    IF QueenPosition[ActualRow] > n THEN

      ' step one row back
      ActualRow -= 1
      ' if this was the first row then we are finished
      if ActualRow < 1 then
        goto PrintAllSolutionsFound
      end if

    ELSE
      'if we are still on the table
      ' check that the actual position is OK or not
      PositionIsOk = TRUE
      for i=1 to ActualRow-1
        if QueenPosition[i] = QueenPosition[ActualRow] or _
           abs(QueenPosition[i]-QueenPosition[ActualRow]) = ActualRow - i then
          PositionIsOk = FALSE
        end if
      next i
      if PositionIsOk then
        ActualRow += 1
        if ActualRow <= n then QueenPosition[ActualRow] = 0
      end if

    END IF
  wend

  '
  ' Check if the actual solution is the same as a previous one
  '
  NewSolutionIsFound = TRUE
  for j=1 to SolutionNumber-1

  HorizontalMirrorCheck:
    for i=1 to n
      if n - QueenPosition[i] <> SavedQueenPosition[j,i] then goto VerticalMirrorCheck
    next i
    NewSolutionIsFound = FALSE
    goto EndCheckLoop

  VerticalMirrorCheck:
    for i=1 to n
      if QueenPosition[n-i+1] <> SavedQueenPosition[j,i] then goto RotateCheck
    next i
    NewSolutionIsFound = FALSE
    goto EndCheckLoop

  RotateCheck:
    for i=1 to n
      RotPos[i] = QueenPosition[i]
    next i
    for k=1 to 3
      for i=1 to n
        TmpPos[n-RotPos[i]+1] = i
      next i
      for i=1 to n
        RotPos[i] = TmpPos[i]
      next i
      for i=1 to n
        if RotPos[i] <> SavedQueenPosition[j,i] then goto NextRotate
      next i
      NewSolutionIsFound = FALSE
      goto EndCheckLoop
    NextRotate:
    next k

  next j
  EndCheckLoop:

  if NewSolutionIsFound then
    for i=1 to n
      SavedQueenPosition[SolutionNumber,i] = QueenPosition[i]
    next i
    print SolutionNumber,". saved\n"
    SolutionNumber += 1
  endif

  ' step back after the solution was found
  ActualRow -= 1

until false

PrintAllSolutionsFound:

for j=1 to SolutionNumber - 1 

  print "Solution #",j,"\n"
  for i=1 to n
    print STRING(SavedQueenPosition[j,i]-1,"."),"*",STRING(n-SavedQueenPosition[j,i],"."),"\n"
  next i
  print

next j
