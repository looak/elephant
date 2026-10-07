import os
import argparse

def generate_square(square_char):
    """Generates the string representation of a single square on the chessboard."""
    if square_char is not None and square_char in "pbnqkPBNQKR":
        return f"[ {square_char} ]"
    else:
        return "[   ]"

def generate_row(row_string):
    resultString = ""
    for character in row_string:
        if character.isdigit():
            squareCnt = int(character)
            for _ in range(squareCnt):
                resultString += generate_square(None)
        else:
            resultString += generate_square(character)
    return resultString

def generate_header_comment(fen):
    """Generates a chessboard header comment based on the provided FEN string."""
    """example:"""
    """ // 8 [ R ][   ][   ][ k ][   ][   ][   ][ r ]"""
    """ // 7 [   ][ b ][   ][   ][   ][   ][ b ][ q ]"""
    """ // 6 [   ][   ][   ][   ][   ][   ][   ][   ]"""
    """ // 5 [   ][   ][   ][   ][   ][   ][   ][   ]"""
    """ // 4 [   ][   ][   ][   ][   ][   ][   ][   ]"""
    """ // 3 [   ][   ][   ][   ][   ][   ][   ][   ]"""
    """ // 2 [   ][   ][   ][   ][   ][   ][   ][ B ]"""
    """ // 1 [   ][   ][   ][   ][ K ][   ][   ][ R ]"""
    """ //     A    B    C    D    E    F    G    H"""
    """ // R3k2r/1b4bq/8/8/8/8/7B/4K2R w KQkq - 0 1"""
    if not fen:
        fen = "8/8/8/8/8/8/8/8 w - - 0 1"  # Default empty board FEN

    resultString = ""
    rows = []

    for row in range(8):
        row_string = "// " + str(8 - row) + " "
        row_string += generate_row(fen.split(" ")[0].split("/")[row])        
        rows.append(row_string)

    resultString = "\n".join(rows)
    resultString += "\n//     A    B    C    D    E    F    G    H"
    resultString += f"\n// {fen}"
    print(resultString)

    # Copy the resultString to clipboard
    try:
        import pyperclip
        pyperclip.copy(resultString)
        print("Chessboard header comment copied to clipboard.")
    except ImportError:
        print("pyperclip module not found. Unable to copy to clipboard.")
    except Exception as e:
        print(f"Unable to copy to clipboard: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate header comments for chessboards in various styles to clipboard.") 
    parser.add_argument("-fen", "--fen", default="", help="The FEN string for the chessboard.")
    
    args = parser.parse_args()
    
    generate_header_comment(args.fen)
