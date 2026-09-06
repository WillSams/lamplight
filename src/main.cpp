#include "game.h"

int main(int argc, char *argv[]) {
    NetMode mode = NetMode::Solo;
    std::string ip = "127.0.0.1";
    uint16_t port = 5000;
    uint32_t seed = 12345;
    // argv: `host <port> [seed]` / `join <ip> <port>`
    if (argc >= 2) {
        std::string a = argv[1];
        if (a == "host") {
            mode = NetMode::Host;
            if (argc >= 3) port = (uint16_t)atoi(argv[2]);
            if (argc >= 4) seed = (uint32_t)atoi(argv[3]);
        } else if (a == "join") {
            mode = NetMode::Join;
            if (argc >= 3) ip = argv[2];
            if (argc >= 4) port = (uint16_t)atoi(argv[3]);
        }
    }
    Game game(mode, ip, port, seed);
    game.Run();
    game.Destroy();
    return 0;
}
