#include "Server.hpp"
#include "../Log.hpp"

using namespace log_es;

void ServerStart()
{
    vod::Server server(8899);
    LOG(INFO, "SERVER START\n");
    server.RunModule();
}
int main()
{
    ServerStart();
    return 0;
}