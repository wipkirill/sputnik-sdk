#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Storage/ConnectionsManager.h>

using std::unique_ptr;

unique_ptr<DbConn> ConnectionsManager::getConnection(const URL &/*url*/, const Properties &props) {
    if(props.get("type") == "sqlite") {
        return unique_ptr<DbConn>(new SqliteConn());
    } else if(props.get("type") == "mysql") {
        LOGG(Logger::ERROR) << "Not implemented yet" << Logger::FLUSH;
        exit(EXIT_FAILURE);
        return 0;//return unique_ptr<DbConn>(new MySqlConn(url, props));
    } else {
        LOGG(Logger::ERROR) << "Unknown driver type" << Logger::FLUSH;
        return 0;
    }
}
