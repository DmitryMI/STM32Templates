#include <boost/program_options.hpp>
#include "RF24GatewayHandler.h"

#ifdef CM_UNIX
#include <pwd.h>
#endif

#include <filesystem>
#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <iostream>

namespace po = boost::program_options;

uint32_t mesh_timer = 0;

po::options_description create_options_description()
{
    po::options_description desc("Allowed options");

    desc.add_options()
        ("help", "produce help message")
        ("service-install", "install .service file. Use --config to specify path to config file.")
        ("service-uninstall", "uninstall .service file")
        ("config", po::value<std::string>(), "path to config file")
        ("log-dir", po::value<std::string>()->default_value("logs"), "logging directory")
        ("log-level", po::value<std::string>()->default_value("INFO"), "logging level")
        ("rf24-ce-pin", po::value<uint16_t>()->default_value(23), "CE pin number")
        ("rf24-csn-pin", po::value<uint16_t>()->default_value(0), "CSN pin number (00 for /dev/spidev0.0, 11 for /dev/spidev1.1, etc.)")
		("rf24-channel", po::value<uint8_t>()->default_value(97), "RF24 channel")
		("rf24-data-rate", po::value<uint8_t>()->default_value(0), "RF24 data rate (0 - RF24_1MBPS, 1 - RF24_2MBPS, 2 - RF24_250KBPS)")
		("force-mkdirs", po::value<bool>()->default_value(false), "create missing directories")
		("gateway-ip", po::value<std::string>()->default_value("10.10.2.0"), "gateway ip address")
		("gateway-subnet", po::value<std::string>()->default_value("255.255.255.0"), "gateway subnet")
		("gateway-node-id", po::value<uint8_t>()->default_value(0), "gateway node id")
		
        ;

    return desc;
}

struct Configuration
{
	uint16_t Rf24CePin = 0;
	uint16_t Rf24CsnPin = 0;
	uint8_t Rf24Channel = 0;
	uint8_t Rf24DataRate = 0;
	std::string GatewayIp;
	std::string GatewaySubnet;
	uint8_t GatewayNodeId = 0;
};

std::filesystem::path GetHomeDir()
{
#ifdef CM_UNIX
	char* homedir;
	if ((homedir = getenv("HOME")) == NULL)
	{
		homedir = getpwuid(getuid())->pw_dir;
	}

	return std::filesystem::path(homedir);
#elif CM_WIN32
	char* userprofile = getenv("USERPROFILE");
	if (userprofile == NULL)
	{
		return "C:/";
	}

	return std::filesystem::path(userprofile);
#endif
}

std::string FixPath(std::string_view path_str)
{
	if (path_str.starts_with("~/"))
	{
		path_str.remove_prefix(2);

		auto home = GetHomeDir();
		return (home / path_str).string();
	}

	if (path_str.starts_with("./"))
	{
		path_str.remove_prefix(2);
	}

	std::filesystem::path path(path_str);
	if (path.is_relative())
	{
		return std::filesystem::absolute(path).string();
	}

	return std::string(path_str);
}

bool GetLogFullname(std::string dir_name, std::string filename, bool force_mkdirs, std::string& out_filename)
{
	std::string logging_dir = "";

	if (!dir_name.empty())
	{
		if (!std::filesystem::exists(dir_name))
		{
			if (!force_mkdirs)
			{
				std::cerr << "Directory " << dir_name << " set as logging dir does not exist!" << std::endl;
				return false;
			}
			else
			{
				std::cerr << "Directory " << dir_name << " set as logging dir does not exist. Will be created." << std::endl;
				std::filesystem::create_directories(dir_name);
				logging_dir = dir_name;
			}
		}
		else
		{
			logging_dir = dir_name;
		}
	}

	std::filesystem::path logging_dir_path(logging_dir);
	std::filesystem::path logging_file(filename);
	std::filesystem::path logging_full_path = logging_dir_path / logging_file;
	out_filename = logging_full_path.string();

	return true;
}

std::shared_ptr<spdlog::sinks::rotating_file_sink_mt> CreateRotatingLogSink(std::string dir_name, std::string file_name, bool force_mkdirs, int size = 1048576 * 5, int files_num = 3)
{
	std::string filepath;
	if (!GetLogFullname(dir_name, file_name, force_mkdirs, filepath))
	{
		std::cerr << "Failed to create rotating_file_sink_mt for dir_name " << dir_name << " and file_name " << file_name << std::endl;
		return nullptr;
	}

	std::cout << "Created rotating_file_sink_mt with path " << filepath << std::endl;
	auto rotating_file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(filepath, size, files_num);
	return rotating_file_sink;
}

bool SetupLogging(std::string dir_name, std::string level, bool force_mkdirs)
{
	auto general_console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	auto general_filesink = CreateRotatingLogSink(dir_name, "RF24GatewayController.log", force_mkdirs);
	if (!general_filesink)
	{
		return false;
	}

	spdlog::logger general_logger("general", { general_console_sink, general_filesink });
	general_logger.set_pattern("[general] [%^%l%$] %v");

	auto general_logger_ptr = std::make_shared<spdlog::logger>(general_logger);
	spdlog::set_default_logger(general_logger_ptr);

	if (!level.empty())
	{
		std::transform(level.begin(), level.end(), level.begin(),
			[](unsigned char c) { return std::tolower(c); }
		);

		spdlog::level::level_enum level_value = spdlog::level::from_str(level);
		spdlog::set_level(level_value);
	}
	else
	{
		// Use SPDLOG_LEVEL
		spdlog::cfg::load_env_levels();
	}

	return true;
}


bool ReadConfiguration(int argc, char** argv, const po::options_description& desc, Configuration& config, po::variables_map& vm)
{
	try
	{
		if (vm.count("config"))
		{
			std::string config_path_str = FixPath(vm["config"].as<std::string>());
			std::filesystem::path config_path(config_path_str);
			if (!std::filesystem::exists(config_path))
			{
				std::cerr << "Config file " << config_path << " not found." << std::endl;
			}
			else
			{
				std::cout << "Using config file " << config_path << std::endl;

				po::store(po::parse_config_file(config_path.string().c_str(), desc), vm);

				std::filesystem::path working_dir = config_path.parent_path();
				std::cout << "Changing working directory to " << working_dir << std::endl;
				std::filesystem::current_path(working_dir);
			}
		}

		po::notify(vm);
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
		return false;
	}

	bool force_mkdirs = vm["force-mkdirs"].as<bool>();
	std::string log_level = vm["log-level"].as<std::string>();
	if (!SetupLogging(FixPath(vm["log-dir"].as<std::string>()), log_level, force_mkdirs))
	{
		return false;
	}
	spdlog::info("Logging set up.");

	config.Rf24CePin = vm["rf24-ce-pin"].as<uint16_t>();
	config.Rf24CsnPin = vm["rf24-csn-pin"].as<uint16_t>();
	config.Rf24DataRate = vm["rf24-data-rate"].as<uint8_t>();
	config.Rf24Channel = vm["rf24-channel"].as<uint8_t>();
	config.GatewayIp = vm["gateway-ip"].as<std::string>();
	config.GatewaySubnet = vm["gateway-subnet"].as<std::string>();
	config.GatewayNodeId = vm["gateway-node-id"].as<uint8_t>();

	return true;
}

int main(int argc, char** argv)
{
	Configuration configuration;

	po::options_description po_desc = create_options_description();
	po::variables_map vm;

	po::store(po::parse_command_line(argc, argv, po_desc), vm);
	po::notify(vm);

	if (vm.count("help"))
	{
		std::cout << po_desc << "\n";
		return 0;
	}

	/*
	if (vm.count("service-install"))
	{
		std::cout << "Installing service..." << "\n";
		if (vm.count("config"))
		{
			return install_service(std::string_view(argv[0]), vm["config"].as<std::string>());
		}
		return install_service(std::string_view(argv[0]));
	}
	else if (vm.count("service-uninstall"))
	{
		std::cout << "Uninstalling PiTV service..." << "\n";
		return uninstall_service();
	}
	*/

	if (!ReadConfiguration(argc, argv, po_desc, configuration, vm))
	{
		return 1;
	}

    RF24GatewayHandler GatewayHandler(configuration.Rf24CePin, configuration.Rf24CsnPin);
	GatewayHandler.Begin(configuration.GatewayIp, configuration.GatewaySubnet, configuration.Rf24Channel, configuration.Rf24DataRate, configuration.GatewayNodeId);

	while (true)
	{
#ifdef CM_WIN32
		_sleep(10000);
#endif

#ifdef CM_UNIX
		usleep(1 * 1000000);
#endif
	}

	spdlog::info("main() exiting");

    return 0;
}