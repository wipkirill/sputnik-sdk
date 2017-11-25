#pragma once

#include <memory>
#include <UrbanLabs/Sdk/Utils/URL.h>
#include <UrbanLabs/Sdk/Utils/Properties.h>
#include <UrbanLabs/Sdk/Storage/SqliteConnection.h>
#include <UrbanLabs/Sdk/Storage/MySqlConnection.h>

class ConnectionsManager {
public:
    static std::unique_ptr<DbConn> getConnection(const URL &url, const Properties &props);
};
