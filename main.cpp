#include <crow.h>
#include <nlohmann/json.hpp>
#include <boost/filesystem.hpp>
#include <random>
#include <iostream>
#include <regex>
#include <fstream>

using namespace crow;
using namespace boost::filesystem;
using nlohmann::json;

class CustomLogger : public crow::ILogHandler {
public:
    CustomLogger()= default;
    void log(std::string message, crow::LogLevel level) override{
        auto now = std::chrono::system_clock::now();
        // UTC + 8
        now += std::chrono::hours(8);
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::string loglevel_name = (level == crow::LogLevel::Info) ? "INFO" : ((level == crow::LogLevel::Warning) ? "WARNING" : "ERROR");
        // [loglevel] [time] message
        std::cout << "[" << loglevel_name << "] [" << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X") << "] " << message << std::endl;
    }
};

nlohmann::json readConfig(const std::string &configFile) {
    // Check if the config file exists
    if (!exists(configFile)) {
        std::cout << "Config file not found. Creating a new one." << std::endl;
        std::ofstream file(configFile);
        nlohmann::json config;
        config["host"] = "0.0.0.0";
        config["port"] = 8080;
        config["image_folder"] = "./images";
        file << config.dump(4);
        return config;
    }
    std::ifstream file(configFile);
    nlohmann::json config;
    file >> config;
    std::cout << "Config: " << config.dump(4) << std::endl;
    return config;
}

std::vector<std::string> indexImages(const path &folder) {
    // Check if the folder exists
    if (!exists(folder)) {
        std::cout << "Image folder not found. Creating a new one." << std::endl;
        create_directory(folder);
        // Create subfolders
        create_directory(folder / "pc");
        create_directory(folder / "mp");
        return std::vector<std::string>();
    }
    std::vector<std::string> images;
    recursive_directory_iterator it(folder), end;

    for (; it != end; ++it) {
        if (is_regular_file(it->path()) && it->path().extension() == ".jpg") {
            images.push_back(it->path().string());
        }
    }
    return images;
}

int main(int argc, char *argv[]) {
    CustomLogger logger;
    crow::logger::setHandler(&logger);
    std::string folderPath;
    nlohmann::json config=readConfig("config.json");
    if (argc > 1) {
        folderPath = argv[1];
        // Write the folder path to the config file
        config["image_folder"] = folderPath;
        std::ofstream file("config.json");
        file << config.dump(4);
    } else {
        folderPath = config["image_folder"];
    }

    std::vector<std::string> images = indexImages(folderPath);
    std::cout << "Found " << images.size() << " images." << std::endl;
    SimpleApp app;
    CROW_ROUTE(app,"/")
            .methods("GET"_method)
                    ([]() {
                        return response(200, "Hello, world!");
                    });
    CROW_ROUTE(app, "/api/images/<string>")
            .methods("GET"_method)
                    ([&images](const request &req, const std::string &subfolder) {
                        std::vector<std::string> filteredImages;
                        for (const auto &image : images) {
                            if (image.find(subfolder) != std::string::npos) {
                                filteredImages.push_back(image);
                            }else if (subfolder == "all"){
                                filteredImages.push_back(image);
                            }
                        }

                        if (filteredImages.empty()) {
                            nlohmann::json body;
                            body["error"] = "No images found in the specified type.";
                            return response(404, body.dump());
                        }

                        std::random_device rd;
                        std::mt19937 gen(rd());
                        std::uniform_int_distribution<size_t> dist(0, filteredImages.size() - 1);
                        size_t randomIndex = dist(gen);
                        auto ip_address = req.get_header_value("X-Real-IP");
                        auto log_message = "Serving image: " + filteredImages[randomIndex] + " to " + ip_address;
                        CROW_LOG_INFO << log_message;
                        response res;
                        res.set_header("Content-Type", "image/jpeg");
                        std::ifstream file(filteredImages[randomIndex], std::ios::binary);
                        res.body = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
                        return res;
                    });

    // app.bindaddr(config["host"]).port(config["port"]).multithreaded().run_async();
    try{
        app.bindaddr(config["host"]).port(config["port"]).multithreaded().run();
    }catch (const std::exception &e){
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
