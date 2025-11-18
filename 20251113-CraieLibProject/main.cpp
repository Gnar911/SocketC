// craie_test_client.c
// C
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

// C++
#include <fstream>
#include <sstream>
#include <filesystem>
#include <thread>
#include "CraieLib.h"  // provided by your library headers
#include "fwlog.h"
#include "json.hpp"
#include "vehicleinfo.h"

// Qt
#include <QCoreApplication>

using json = nlohmann::json;
static volatile int g_running = 1;
static json previousState;
static bool hasPrevious = false;

static bool json_changed(const json& j, const char* key) {
    if (!hasPrevious) return false;               // first load
    if (!previousState.contains(key)) return true;
    if (!j.contains(key)) return false;          // key removed → ignore
    return j[key] != previousState[key];
}

static void process_json_commands(const json& j) {

    // -------- driving state --------
    if (json_changed(j, "driving_state")) {
        if (j.contains("driving_state")) {
            int v = j["driving_state"].get<int>();
            SetDrivingState((DrivingCourse)v);
            syslog(LOG_INFO, "SetDrivingState(%d)", v);
        }
    }

    // -------- POI list --------
    //if (json_changed(j, "poi_list")) {
        if (j.contains("poi_list")) {
            syslog(LOG_INFO, "Preparing POI list JSON to send...");

            // Extract the "poi_list" block
            json poiJson = j["poi_list"];

            // Convert back to raw JSON string
            std::string raw = poiJson.dump();

            // Send JSON to CraieLib
            CraieResult res = request_push_json((void*)raw.c_str(), RequestPushJsonType::Poi);
            if (res != CraieResult::Ok) {
                syslog(LOG_ERR, "Failed to send POI JSON (code %d)", (int)res);
            }
        }
   //  }

    // -------- reset configuration --------
    if (json_changed(j, "reset_craie_config")) {
        if (j.contains("reset_craie_config")) {
            if (j["reset_craie_config"].get<int>() != 0) {
                CraieResult r = ResetCraieConfig();
                syslog(LOG_INFO, "ResetCraieConfig() -> %d", (int)r);
            }
        }
    }

    // -------- start pairing tablet --------
    if (json_changed(j, "start_pairing_tablet")) {
        if (j.contains("start_pairing_tablet")) {
            if (j["start_pairing_tablet"].get<int>() != 0) {
                CraieResult r = StartPairingTablet();
                syslog(LOG_INFO, "StartPairingTablet() -> %d", (int)r);
            }
        }
    }

    // -------- tablet display mode --------
    if (json_changed(j, "tablet_display_mode")) {
        if (j.contains("tablet_display_mode")) {
            int v = j["tablet_display_mode"].get<int>();
            CraieResult r = request_event((TabletDisplayMode)v);
            syslog(LOG_INFO, "request_event(TabletDisplayMode=%d) -> %d", v, (int)r);
        }
    }

    // -------- text summary sending --------
    if (json_changed(j, "text_summary")) {
        if (j.contains("text_summary")) {

            auto s = j["text_summary"];

            if (!s.contains("text") || !s.contains("seat_id")) {
                syslog(LOG_ERR, "text_summary missing fields {text, seat_id}");
            } else {
                std::string summary = s["text"].get<std::string>();
                int seatId = s["seat_id"].get<int>();

                CraieResult r = request_push_summary(summary.c_str(), (SeatId)seatId);
                syslog(LOG_INFO, "request_push_summary(text=\"%s\", seat=%d) -> %d",
                       summary.c_str(), seatId, (int)r);
            }
        }
    }


    // -------- Push image --------
    if (json_changed(j, "image")) {
        if (j.contains("image") && j["image"].is_string()) {

            std::string imagePath = j["image"];
            std::string imageName;

            // extract file name from the path
            auto pos = imagePath.find_last_of("/\\");
            if (pos != std::string::npos)
                imageName = imagePath.substr(pos + 1);
            else
                imageName = imagePath;

            CraieResult res = request_push_image(
                (void*)imageName.c_str(),
                (void*)imagePath.c_str(),
                RequestPushImageType::Selfie
            );

            if (res != CraieResult::Ok) {
                syslog(LOG_ERR, "Failed to send image (code %d)", (int)res);
            }
        }
    }

    // -------- Push LLM --------
    if (json_changed(j, "llm")) {
        if (j.contains("llm")) {
            json poiJson = j["llm"];

            // Convert back to raw JSON string
            std::string raw = poiJson.dump();   // <----- KEY LINE

            // Send JSON to CraieLib
            CraieResult res = request_push_json((void*)raw.c_str(), RequestPushJsonType::LLM);
            if (res != CraieResult::Ok) {
                syslog(LOG_ERR, "Failed to send LLM (code %d)", (int)res);
            }
        }
    }
    previousState = j;
    hasPrevious = true;
}

static void watch_json_command_file(const std::string& filePath) {
    namespace fs = std::filesystem;

    fs::file_time_type lastWriteTime{};
    bool firstRun = true;

    while (g_running) {
        if (fs::exists(filePath)) {
            auto writeTime = fs::last_write_time(filePath);

            if (firstRun || writeTime != lastWriteTime) {
                firstRun = false;
                lastWriteTime = writeTime;

                syslog(LOG_INFO, "Detected JSON file update, reloading: %s", filePath.c_str());

                std::ifstream f(filePath);
                if (!f.is_open()) {
                    syslog(LOG_ERR, "Could not open JSON file: %s", filePath.c_str());
                    continue;
                }

                try {
                    json j = json::parse(f);
                    process_json_commands(j);
                } catch (std::exception& e) {
                    syslog(LOG_ERR, "JSON parse error: %s", e.what());
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

// Handle Ctrl+C to cleanly stop the test client
static void handle_sigint(int sig) {
    syslog(LOG_INFO, "Caught signal %d, stopping...", sig);
    g_running = 0;
}

static void setup_syslog() {
    openlog("CraieTestClient", LOG_CONS | LOG_PID | LOG_NDELAY | LOG_PERROR, LOG_USER);
    syslog(LOG_INFO, "Syslog initialized");
}

// Initialize CraieLib server context
static int start_server() {
    CraieResult res = init_context(CRAIE_SYSTEM_VERSION);
    if (res != CraieResult::Ok) {
        syslog(LOG_ERR, "init_context failed with code: %d", res);
        return -1;
    }
    syslog(LOG_INFO, "CraieLib initialized successfully");
    return 0;
}

// Shutdown CraieLib
static void stop_server() {
    deinit_context();
    syslog(LOG_INFO, "CraieLib deinitialized");
}

static void send_json_file(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        syslog(LOG_ERR, "Cannot open JSON file: %s", filePath.c_str());
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();  // read entire file into string
    std::string jsonStr = buffer.str();
    file.close();

    if (jsonStr.empty()) {
        syslog(LOG_WARNING, "JSON file %s is empty", filePath.c_str());
        return;
    }

    CraieResult res = request_push_json((void*)jsonStr.c_str(), RequestPushJsonType::Poi);
    if (res == CraieResult::Ok)
        syslog(LOG_INFO, "Successfully sent JSON from file: %s", filePath.c_str());
    else
        syslog(LOG_ERR, "Failed to send JSON from file %s (code %d)", filePath.c_str(), (int)res);
}

//static void watch_json_file(const std::string& filePath) {
//    namespace fs = std::filesystem;

//    fs::file_time_type lastWriteTime{};
//    bool firstRead = true;

//    while (g_running) {
//        try {
//            if (fs::exists(filePath)) {
//                auto currentWriteTime = fs::last_write_time(filePath);

//                if (firstRead || currentWriteTime != lastWriteTime) {
//                    firstRead = false;
//                    lastWriteTime = currentWriteTime;

//                    std::ifstream file(filePath);
//                    if (!file.is_open()) {
//                        syslog(LOG_ERR, "Cannot open JSON file: %s", filePath.c_str());
//                        std::this_thread::sleep_for(std::chrono::seconds(1));
//                        continue;
//                    }

//                    std::stringstream buffer;
//                    buffer << file.rdbuf();
//                    std::string jsonStr = buffer.str();
//                    file.close();

//                    if (jsonStr.empty()) {
//                        syslog(LOG_WARNING, "File %s is empty", filePath.c_str());
//                        continue;
//                    }

//                    CraieResult res = request_push_json((void*)jsonStr.c_str(), RequestPushJsonType::Poi);
//                    if (res == CraieResult::Ok)
//                        syslog(LOG_INFO, "Sent updated JSON to server from file: %s", filePath.c_str());
//                    else
//                        syslog(LOG_ERR, "request_push_json failed (%d) for file %s", (int)res, filePath.c_str());
//                }
//            } else {
//                syslog(LOG_WARNING, "File %s not found, waiting...", filePath.c_str());
//            }
//        } catch (const std::exception& e) {
//            syslog(LOG_ERR, "Exception watching file: %s", e.what());
//        }

//        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // check twice per second
//    }
//}

// Periodically read CraieLib state and log key fields
static void poll_server_state() {
    static int frameCount = 0;
    AllCraieConfig cfg;
    memset(&cfg, 0, sizeof(cfg));

    CraieResult ret = get_all_craie_config(&cfg);
    if (ret != CraieResult::Ok) {
        syslog(LOG_WARNING, "get_all_craie_config returned %d", ret);
        return;
    }

    if (++frameCount < 60) return;
        frameCount = 0;

        syslog(LOG_INFO, "---- CraieConfig Snapshot ----");
//        syslog(LOG_INFO, "Active content: %d", (int)cfg.active_content);
//        syslog(LOG_INFO, "VoiceAssistant active: %d", cfg.voice_assistant.active);
//        //syslog(LOG_INFO, "Nanoe active: %d", cfg.typer.nanoe);
//        syslog(LOG_INFO, "Sound normal speed: %d", !cfg.typer.is_speed_up_sound_mode);

//        syslog(LOG_INFO, "LED #1 (SoundControl): %d", (int)cfg.typer.led_sound_control);
//        syslog(LOG_INFO, "LED #2 (CurrentInfo): %d", (int)cfg.typer.led_current_info);
//        syslog(LOG_INFO, "LED #3 (DIS): %d", (int)cfg.typer.led_dis);

//        syslog(LOG_INFO, "Massage Active: %d", cfg.typer.massage_active);
//        syslog(LOG_INFO, "Massage Seats: driver=%d, frontLeft=%d, rearLeft=%d, rearRight=%d",
//               cfg.typer.massage_driver, cfg.typer.massage_front_left,
//               cfg.typer.massage_rear_left, cfg.typer.massage_rear_right);

//        syslog(LOG_INFO, "Massage Strength: driver=%d, frontLeft=%d, rearLeft=%d, rearRight=%d",
//               (int)cfg.typer.massage_seat_config.driver.strength,
//               (int)cfg.typer.massage_seat_config.front_left.strength,
//               (int)cfg.typer.massage_seat_config.rear_left.strength,
//               (int)cfg.typer.massage_seat_config.rear_right.strength);

//        syslog(LOG_INFO, "Massage Pattern: driver=%d, frontLeft=%d, rearLeft=%d, rearRight=%d",
//               (int)cfg.typer.massage_seat_config.driver.pattern,
//               (int)cfg.typer.massage_seat_config.front_left.pattern,
//               (int)cfg.typer.massage_seat_config.rear_left.pattern,
//               (int)cfg.typer.massage_seat_config.rear_right.pattern);

//        syslog(LOG_INFO, "Gesture Detection: active=%d, seat=%d",
//               cfg.gesture_detection.active,
//               (int)cfg.gesture_detection.gesture_detection_seat);

        //syslog(LOG_INFO, "Volume Entire Vehicle: %f", cfg.volume_adjustment.entire_vehicle);
//        syslog(LOG_INFO, "Selfie Active: %d", cfg.selfie.active);
//        syslog(LOG_INFO, "Selfie Auto Mode: %d", !cfg.selfie.is_manual_mode);

        syslog(LOG_INFO, "Music share state: driver=%d, frontLeft=%d, rearLeft=%d, rearRight=%d",
               (int)cfg.typer.sound_seat_config.driver_source,
               (int)cfg.typer.sound_seat_config.front_left_source,
               (int)cfg.typer.sound_seat_config.rear_right_source,
               (int)cfg.typer.sound_seat_config.rear_left_source);


        syslog(LOG_INFO, "------------------------------");

}

int main(int argc, char* argv[]) {
    setup_syslog();
    syslog(LOG_INFO, "Starting CraieLib test client...");

    // Register Ctrl+C handler for clean shutdown
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    QCoreApplication app(argc, argv);
    VehicleInfo server;

    if (start_server() != 0) {
        syslog(LOG_ERR, "Failed to initialize CraieLib, exiting.");
        closelog();
        return EXIT_FAILURE;
    }

    std::string filePath = (argc > 1) ? argv[1] : "commands.json";
    std::thread watchThread(watch_json_command_file, filePath);

    const int interval_ms = 1000 / 60;

    //if (init_context(CRAIE_SYSTEM_VERSION) == CraieResult::Ok)
    while (g_running) {
        poll_server_state();
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        usleep(interval_ms * 1000);
    }

    // Cleanup
    stop_server();
    syslog(LOG_INFO, "CraieLib test client stopped.");
    closelog();

    return EXIT_SUCCESS;
}
