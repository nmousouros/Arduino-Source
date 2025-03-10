/*  Raid Lobby Reader
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 *
 *      Determine if a raid is full and ready to start early.
 *
 */

#ifndef PokemonAutomation_PokemonSwSh_RaidLobbyReader_H
#define PokemonAutomation_PokemonSwSh_RaidLobbyReader_H

#include "CommonFramework/Tools/VideoFeed.h"
#include "CommonFramework/Logging/Logger.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{


enum class RaidLobbySlot{
    EMPTY,
    NOT_READY,
    READY,
};

struct RaidLobbyState{
    bool valid = false;
    RaidLobbySlot player0 = RaidLobbySlot::EMPTY;
    RaidLobbySlot player1 = RaidLobbySlot::EMPTY;
    RaidLobbySlot player2 = RaidLobbySlot::EMPTY;
    RaidLobbySlot player3 = RaidLobbySlot::EMPTY;

    bool raid_is_full() const{
        return
            player0 != RaidLobbySlot::EMPTY &&
            player1 != RaidLobbySlot::EMPTY &&
            player2 != RaidLobbySlot::EMPTY &&
            player3 != RaidLobbySlot::EMPTY;
    }
    bool raiders_are_ready() const{
        return
            player1 != RaidLobbySlot::NOT_READY &&
            player2 != RaidLobbySlot::NOT_READY &&
            player3 != RaidLobbySlot::NOT_READY;
    }

    size_t raiders() const{
        size_t count = 0;
        if (player1 != RaidLobbySlot::EMPTY) count++;
        if (player2 != RaidLobbySlot::EMPTY) count++;
        if (player3 != RaidLobbySlot::EMPTY) count++;
        return count;
    }
};


class RaidLobbyReader{
public:
    RaidLobbyReader(Logger& logger, VideoOverlay& overlay);

    RaidLobbyState read(const QImage& screen);

private:
    Logger& m_logger;
    InferenceBoxScope m_checkbox0;
    InferenceBoxScope m_checkbox1;
    InferenceBoxScope m_checkbox2;
    InferenceBoxScope m_checkbox3;
    InferenceBoxScope m_spritebox0;
    InferenceBoxScope m_spritebox1;
    InferenceBoxScope m_spritebox2;
    InferenceBoxScope m_spritebox3;
};


}
}
}
#endif

