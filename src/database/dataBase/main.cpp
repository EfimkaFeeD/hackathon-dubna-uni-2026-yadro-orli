#include <csignal>
#include <atomic>
#include <chrono>
#include <thread>
#include "./src/validator.h"
#include "./src/dataBase.h"
#include "./src/readerJSON.h"

std::atomic<bool> running(true);

void signalHandler(int signal) {
    std::cout << "\nGet signal " << signal << ". Finish work..." << std::endl;
    running = false;
}


int main() {
    std::signal(SIGINT, signalHandler);  // Ctrl+C
    std::signal(SIGTERM, signalHandler); // Docker stop
    readerJSON reader;
    reader.read("confCreateDB.json");
    std::vector<table> tables = reader.getTables();
    validator val;
    if (val.validateSchema(tables)) {
        dataBase db;
        db.createAllTables(tables);
        std::cout << "Server DB was started" << std::endl;
        while (running) {
            // logic of hand queries
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } else {
        std::cout << "not validate schema" << std::endl;
        return 1;
    }
}
