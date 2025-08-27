TEST_F(ChessboardFixture, ZorbistHashing)
{
    // board should start out empty, so hashing these two boards should result in the same value.
    Chessboard boardOne;
    Chessboard boardTwo;

    u64 oneHash = ZorbistHash::Instance().HashBoard(boardOne);
    u64 twoHash = ZorbistHash::Instance().HashBoard(boardTwo);

    EXPECT_EQ(oneHash, twoHash);

    auto k = BLACKKING;
    auto q = BLACKQUEEN;
    auto b = BLACKBISHOP;
    auto n = BLACKKNIGHT;
    auto r = BLACKROOK;
    auto p = BLACKPAWN;
    auto R = WHITEROOK;

    // board one
    boardOne.PlacePiece(r, a8);
    boardTwo.PlacePiece(R, a8);

    oneHash = ZorbistHash::Instance().HashBoard(boardOne);
    twoHash = ZorbistHash::Instance().HashBoard(boardTwo);

    EXPECT_NE(oneHash, twoHash);
    EXPECT_EQ(boardTwo.readHash(), twoHash);
    EXPECT_EQ(boardOne.readHash(), oneHash);
    EXPECT_NE(boardOne.readHash(), boardTwo.readHash());

    // board one
    boardOne.PlacePiece(n, b8);
    EXPECT_EQ(n, boardOne.readPieceAt(Square::B8));
    boardOne.PlacePiece(b, c8);
    EXPECT_EQ(b, boardOne.readPieceAt(Square::C8));
    boardOne.PlacePiece(q, d8);
    EXPECT_EQ(q, boardOne.readPieceAt(Square::D8));
    boardOne.PlacePiece(k, e8);
    EXPECT_EQ(k, boardOne.readPieceAt(Square::E8));
    boardOne.PlacePiece(b, f8);
    EXPECT_EQ(b, boardOne.readPieceAt(Square::F8));
    boardOne.PlacePiece(n, g8);
    EXPECT_EQ(n, boardOne.readPieceAt(Square::G8));

    boardOne.PlacePiece(p, a7);
    EXPECT_EQ(p, boardOne.readPieceAt(Square::A7));
    boardOne.PlacePiece(p, b7);
    EXPECT_EQ(p, boardOne.readPieceAt(Square::B7));
    boardOne.PlacePiece(p, c7);
    EXPECT_EQ(p, boardOne.readPieceAt(Square::C7));
    boardOne.PlacePiece(p, d7);
    EXPECT_EQ(p, boardOne.readPieceAt(Square::D7));
    boardOne.PlacePiece(p, e7);
    EXPECT_EQ(p, boardOne.readPieceAt(Square::E7));
    boardOne.PlacePiece(p, f7);
    EXPECT_EQ(p, boardOne.readPieceAt(Square::F7));
    boardOne.PlacePiece(p, g7);
    EXPECT_EQ(p, boardOne.readPieceAt(Square::G7));
    boardOne.PlacePiece(p, h7);
    EXPECT_EQ(p, boardOne.readPieceAt(Square::H7));

    // board Two
    boardTwo.PlacePiece(r, a8, true);
    EXPECT_EQ(r, boardTwo.readPieceAt(Square::A8));
    boardTwo.PlacePiece(n, b8);
    EXPECT_EQ(n, boardTwo.readPieceAt(Square::B8));
    boardTwo.PlacePiece(b, c8);
    EXPECT_EQ(b, boardTwo.readPieceAt(Square::C8));
    boardTwo.PlacePiece(q, d8);
    EXPECT_EQ(q, boardTwo.readPieceAt(Square::D8));
    boardTwo.PlacePiece(k, e8);
    EXPECT_EQ(k, boardTwo.readPieceAt(Square::E8));
    boardTwo.PlacePiece(b, f8);
    EXPECT_EQ(b, boardTwo.readPieceAt(Square::F8));
    boardTwo.PlacePiece(n, g8);
    EXPECT_EQ(n, boardTwo.readPieceAt(Square::G8));
    boardTwo.PlacePiece(r, h8);
    EXPECT_EQ(r, boardTwo.readPieceAt(Square::H8));

    boardTwo.PlacePiece(p, h7);
    EXPECT_EQ(p, boardTwo.readPieceAt(Square::H7));
    boardTwo.PlacePiece(p, g7);
    EXPECT_EQ(p, boardTwo.readPieceAt(Square::G7));
    boardTwo.PlacePiece(p, f7);
    EXPECT_EQ(p, boardTwo.readPieceAt(Square::F7));
    boardTwo.PlacePiece(p, e7);
    EXPECT_EQ(p, boardTwo.readPieceAt(Square::E7));
    boardTwo.PlacePiece(p, d7);
    EXPECT_EQ(p, boardTwo.readPieceAt(Square::D7));
    boardTwo.PlacePiece(p, c7);
    EXPECT_EQ(p, boardTwo.readPieceAt(Square::C7));
    boardTwo.PlacePiece(p, b7);
    EXPECT_EQ(p, boardTwo.readPieceAt(Square::B7));
    boardTwo.PlacePiece(p, a7);
    EXPECT_EQ(p, boardTwo.readPieceAt(Square::A7));

    oneHash = ZorbistHash::Instance().HashBoard(boardOne);
    twoHash = ZorbistHash::Instance().HashBoard(boardTwo);

    EXPECT_NE(oneHash, twoHash);
    EXPECT_EQ(boardTwo.readHash(), twoHash);
    EXPECT_EQ(boardOne.readHash(), oneHash);
    EXPECT_NE(boardOne.readHash(), boardTwo.readHash());

    boardOne.PlacePiece(r, h8);

    oneHash = ZorbistHash::Instance().HashBoard(boardOne);
    twoHash = ZorbistHash::Instance().HashBoard(boardTwo);

    EXPECT_EQ(oneHash, twoHash);
    EXPECT_EQ(boardTwo.readHash(), twoHash);
    EXPECT_EQ(boardOne.readHash(), oneHash);
    EXPECT_EQ(boardOne.readHash(), boardTwo.readHash());

    boardTwo.setCastlingState(12);

    oneHash = ZorbistHash::Instance().HashBoard(boardOne);
    twoHash = ZorbistHash::Instance().HashBoard(boardTwo);

    EXPECT_NE(oneHash, twoHash);
    EXPECT_EQ(boardTwo.readHash(), twoHash);
    EXPECT_EQ(boardOne.readHash(), oneHash);
    EXPECT_NE(boardOne.readHash(), boardTwo.readHash());

    boardOne.setCastlingState(12);

    oneHash = ZorbistHash::Instance().HashBoard(boardOne);
    twoHash = ZorbistHash::Instance().HashBoard(boardTwo);

    EXPECT_EQ(oneHash, twoHash);
    EXPECT_EQ(boardTwo.readHash(), twoHash);
    EXPECT_EQ(boardOne.readHash(), oneHash);
    EXPECT_EQ(boardOne.readHash(), boardTwo.readHash());

    boardOne.setEnPassant(c7);

    oneHash = ZorbistHash::Instance().HashBoard(boardOne);
    twoHash = ZorbistHash::Instance().HashBoard(boardTwo);

    EXPECT_NE(oneHash, twoHash);
    EXPECT_EQ(boardTwo.readHash(), twoHash);
    EXPECT_EQ(boardOne.readHash(), oneHash);
    EXPECT_NE(boardOne.readHash(), boardTwo.readHash());

    boardTwo.setEnPassant(c7);

    oneHash = ZorbistHash::Instance().HashBoard(boardOne);
    twoHash = ZorbistHash::Instance().HashBoard(boardTwo);

    EXPECT_EQ(oneHash, twoHash);
    EXPECT_EQ(boardTwo.readHash(), twoHash);
    EXPECT_EQ(boardOne.readHash(), oneHash);
    EXPECT_EQ(boardOne.readHash(), boardTwo.readHash());

    boardOne.setEnPassant(e4);

    oneHash = ZorbistHash::Instance().HashBoard(boardOne);

    EXPECT_NE(oneHash, twoHash);
    EXPECT_EQ(boardOne.readHash(), oneHash);
    EXPECT_NE(boardOne.readHash(), boardTwo.readHash());
}