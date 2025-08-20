#pragma once
#include <iostream>
#include <list>
#include <unordered_map>

#include <core/game_context.hpp>

class UCI
{
public:
    UCI();
    ~UCI();

    void Enable();
    bool Enabled();
    void Disable();

    /**
    * position [fen  | startpos ]  moves  ....
    * set up the position described in fenstring on the internal board and play the
    * moves on the internal chess board. If the game was played from the start
    * position the string "startpos" will be sent   */
    bool Position(std::list<std::string>& args);

    /**
     * Synchronizes the engine with the GUI by ensuring that the engine is ready
     * or still alive after processing time-consuming commands.
     * This function must be called once before initiating any search to allow
     * the engine to complete its initialization.
     * The engine will always respond with "readyok" when this command is called,
     * even during an ongoing search, without interrupting the search process.  */
    bool IsReady();

    /**
     * option name [name] value [value]
     * sets a option on the engine */
    bool SetOption(const std::list<std::string>& args);

    /**
     * Sets up the engine for a new game. The engine will respond "isready"    */
    bool NewGame();

    /**
     * Starts calculating the best move for the current position. Number of
     * options are available according to the UCI standard and described in
     * the documentation.     */
    bool Go(std::list<std::string>& args);

    /**
     * Stops calculating the best move for the current position. If the engine
     * was  calculating a move, it will respond with "bestmove"     */
    bool Stop();

    /**
     * Non standard UCI, used for testing.   */
    bool Perft(std::list<std::string>& args);

    const GameContext& readGameContext() { return m_context; }
private:
    /**
     * initialize the engines options with default values    */
    void InitializeOptions();

    bool m_enabled;
    GameContext m_context;
    std::ostream& m_stream;
    std::unordered_map<std::string, std::string> m_options;
};