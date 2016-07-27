/******************************************************************************
    Copyright (C) 2016 by Jo�o Portela <email@joaoportela.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include<iostream>
#include<string>
#include<memory>

#include<boost/program_options.hpp>

#include<obs.h>
#include<obs.hpp>
#include<util/platform.h>

#include"enum_types.hpp"
#include"setup_obs.hpp"

namespace {
	namespace Ret {
		enum ENUM : int {
			success = 0,
			print_help = 1,
			error_in_command_line = 2,
			error_unhandled_exception = 3,
			error_obs = 4
		};
	}

// cli options with default values.
int monitor_to_record = 0;
std::string encoder_selected = "obs_x264";
std::string output_filepath = "default.mp4";

std::string output_filepath2 = "default2.mp4";
int video_bitrate = 2500;

} // namespace

/**
* Resets/Initializes video settings.
*
*   Calls obs_reset_video internally. Assumes some video options.
*/
void reset_video() {
	struct obs_video_info ovi;

	ovi.fps_num = 60;
	ovi.fps_den = 1;

	ovi.graphics_module = "libobs-d3d11.dll"; // DL_D3D11
	ovi.base_width = 1920;
	ovi.base_height = 1080;
	ovi.output_width = 1920;
	ovi.output_height = 1080;
	ovi.output_format = VIDEO_FORMAT_NV12;
	ovi.colorspace = VIDEO_CS_601;
	ovi.range = VIDEO_RANGE_PARTIAL;
	ovi.adapter = 0;
	ovi.gpu_conversion = true;
	ovi.scale_type = OBS_SCALE_BICUBIC;

	int ret = obs_reset_video(&ovi);
	if (ret != OBS_VIDEO_SUCCESS) {
		std::cout << "reset_video failed!" << std::endl;
	}
}

/**
* Resets/Initializes video settings.
*
*   Calls obs_reset_audio internally. Assumes some audio options.
*/
void reset_audio() {
	struct obs_audio_info ai;
	ai.samples_per_sec = 44100;
	ai.speakers = SPEAKERS_STEREO;

	bool success = obs_reset_audio(&ai);
	if (!success)
		std::cout << "Audio reset failed!" << std::endl;
}

void start_recording(std::vector<OBSOutput> outputs) {
	int outputs_started = 0;
	for (auto output : outputs) {
		bool success = obs_output_start(output);
		outputs_started += success ? 1 : 0;
	}

	if (outputs_started == outputs.size()) {
		std::cout << "Recording started for all outputs!" << std::endl;
	}
	else {
		int outputs_failed = outputs.size() - outputs_started;
		std::cerr << outputs_failed << "/" << outputs.size() << " file outputs are not recording." << std::endl;
	}
}

int parse_args(int argc, char **argv) {
	namespace po = boost::program_options;

	// Declare the supported options.
	po::options_description desc("\n****************************************************************\nAllowed options");
	desc.add_options()
		("help,h", "produce help message")
		("monitor,m", po::value<int>(), "set monitor to be recorded")
		("encoder,e", po::value<std::string>(), "set encoder")
		("vbitrate,v", po::value<int>(), "set video bitrate. suggested values: 1200 for low, 2500 for medium, 5000 for high")
		("output,o", po::value<std::string>(), "set file destination")
		("secondary_output,s", po::value<std::string>(), "set file destination")
		;

	try {
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			std::cout << desc << "\n";
			//print_obs_enum_input_types();
			print_obs_enum_encoder_types();
			//print_obs_enum_output_types();
			return Ret::print_help;
		}

		if (vm.count("monitor")) {
			std::cout << "Monitor was set to "
				<< vm["monitor"].as<int>() << ".\n";
			monitor_to_record = vm["monitor"].as<int>();
		}
		else {
			std::cout << "Monitor not set.\n";
			return Ret::error_in_command_line;
		}

		if (vm.count("encoder")) {
			std::cout << "Encoder was set to "
				<< vm["encoder"].as<std::string>() << ".\n";
			encoder_selected = vm["encoder"].as<std::string>();
		}
		else {
			std::cout << "Encoder not set.\n";
			return Ret::error_in_command_line;
		}

		if (vm.count("vbitrate")) {
			std::cout << "Video bitrate was set to "
				<< vm["vbitrate"].as<int>() << ".\n";
			video_bitrate = vm["vbitrate"].as<int>();
		}
		else {
			std::cout << "Bitrate not set.\n";
			return Ret::error_in_command_line;
		}

		if (vm.count("output")) {
			std::cout << "Output was set to "
				<< vm["output"].as<std::string>() << ".\n";
			output_filepath = vm["output"].as<std::string>();
		}
		else {
			std::cout << "Output not set.\n";
			return Ret::error_in_command_line;
		}
		if (vm.count("secondary_output")) {
			std::cout << "Secondary output was set to "
				<< vm["secondary_output"].as<std::string>() << ".\n";
			output_filepath2 = vm["secondary_output"].as<std::string>();
		}
		else {
			std::cout << "Secondary output not set.\n";
		}

	}
	catch (po::error& e) {
		std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
		std::cerr << desc << std::endl;
		return Ret::error_in_command_line;
	}

	return Ret::success;
}

int main(int argc, char **argv) {
	try {
		// manages object context so that we don't have to call
		// obs_startup and obs_shutdown.
		OBSContext ObsScope("en-US", nullptr, nullptr);

		if (!obs_initialized()) {
			std::cerr << "Obs initialization failed." << std::endl;
			return Ret::error_obs;
		}

		reset_video();
		reset_audio();
		obs_load_all_modules();

		//Only parse after Initializing OBS so that the help can print available encoders, etc.
		int ret = parse_args(argc, argv);
		if (ret != Ret::success)
			return ret;

		setup_input(monitor_to_record);

		// While the outputs are kept in scope, we will continue recording.
		std::vector<OBSOutput> outputs = setup_outputs(encoder_selected, video_bitrate, { output_filepath, output_filepath2 });

		start_recording(outputs);

		// wait for user input to stop recording.
		std::cout << "press any key to stop recording." << std::endl;
		std::string str;
		std::getline(std::cin, str);

		return Ret::success;
	}
	catch (std::exception& e) {
		std::cerr << "Unhandled Exception reached the top of main: "
			<< e.what() << ", application will now exit" << std::endl;
		return Ret::error_unhandled_exception;
	}
}
