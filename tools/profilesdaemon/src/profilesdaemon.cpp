/**
 * \file fbitexpire.cpp
 * \author Michal Kozubik <kozubik@cesnet.cz>
 * \brief Main body of the fbitexpire
 *
 * Copyright (C) 2015 CESNET, z.s.p.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is, and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */

#include <cstdlib>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Profiles.h"
#include "SocketController.h"
#include "verbose.h"

/** Acceptable command-line parameters (normal) */
#define OPTSTRING "hVp:s:v:x:X:"

/** Acceptable command-line parameters (long) */
struct option long_opts[] = {
{ "help",		no_argument,		NULL, 'h' },
{ "version",	no_argument,		NULL, 'V' },
{ "daemonize",  no_argument,		NULL, 'd' },
{ "verbose",	required_argument,	NULL, 'v' },
{ "port",		required_argument,	NULL, 'p' },
{ "config",		required_argument,	NULL, 'c' },
{ "socket",		required_argument,	NULL, 's' },
{ 0, 0, 0, 0 }
};

void print_help()
{
	std::cout << "Usage: profilesdaemon [-hV] -p port -c config -s socket\n\n";
	std::cout << "Options:\n";
	std::cout << "  -h, --help                 Show this help and exit\n";
	std::cout << "  -V, --version              Show version and exit\n";
	std::cout << "  -p, --port=PORT            Port number for collectors\n";
	std::cout << "  -c, --config=PROFILES      Path to the profiles.xml configuration\n";
	std::cout << "  -s, --socket=SOCKET        Path to the control socket for adding/removing profiles etc.\n";
	std::cout << "  -v, --verbose=LEVEL        Set verbosity level\n";
	std::cout << "  -d, --daemonize            Run as a daemon\n";
	std::cout << "\n";
}

void print_version()
{
	std::cout << "profilesdaemon v0.1" << "\n";
}

int testCase(std::string path, std::string channel, std::string profile)
{
	try {

		Profiles *profiles = new Profiles(path);

		if (!channel.empty()) {
			profiles->addChannel(channel);
			profiles->saveChanges();
		}

		if (!profile.empty()) {
			profiles->addProfile(profile);
			profiles->saveChanges();
		}

		if (channel.empty() && profile.empty()) {
			profiles->print();
		}

	} catch (std::exception &e) {

		MSG_ERROR(e.what());
		return 1;

	}

	return 0;

}

int main(int argc, char *argv[])
{
	std::string profilesConfig{}, port{}, controlSocket{};
	std::string channel{}, profile{};
	int c;
	bool daemonize{false};
	while ((c = getopt_long(argc, argv, OPTSTRING, long_opts, NULL)) != -1) {
		switch (c) {
		case 'h':
			print_help();
			return 0;
		case 'V':
			print_version();
			return 0;
		case 'p':
			port = std::string(optarg);
			break;
		case 'c':
			profilesConfig = std::string(optarg);
			break;
		case 's':
			controlSocket = std::string(optarg);
			break;
		case 'v':
			MSG_SET_VERBOSE(std::atoi(optarg));
			break;
		case 'x':
			channel = std::string(optarg);
			break;
		case 'X':
			profile = std::string(optarg);
			break;
		case 'd':
			daemonize = true;
			break;
		default:
			print_help();
			return 1;
		}
	}

	openlog(0, LOG_CONS | LOG_PID, LOG_USER);

	if (daemonize) {
		// TODO: daemonize
	}

	if (profilesConfig.empty()) {
		MSG_ERROR("Missing path to the profiles configuration (-p)!");
		return 1;
	}

	return testCase(profilesConfig, channel, profile);

	if (controlSocket.empty()) {
		MSG_ERROR("Missing path to the control socket (-s)!");
		return 1;
	}

	if (port.empty()) {
		MSG_ERROR("Missing path to the collectors socket (-c)");
		return 1;
	}

	Profiles *profiles = new Profiles(profilesConfig);
	SocketController *sockets = new SocketController(controlSocket, port);

	sockets->setProfiles(profiles);
	sockets->run();

	closelog();
}