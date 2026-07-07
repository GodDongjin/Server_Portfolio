#include "GlobalData.h"

ClientMode GClientMode = ClientMode::LOAD_TEST;
ServerStats GServerStats;
atomic<int32> GExpectedRoomRecvCount = 10;

