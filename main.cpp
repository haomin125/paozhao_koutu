#include "version.h"
#include "web_server.hpp"
#include "xj_app_server.h"
#include "xj_app_io_manager.h"
#include "database.h"
#include "logger.h"
#include "customized_json_config.h"
#include "apb_web_server.hpp"
#include "xj_app_web_server.h"

#include <csignal>
#include <thread>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>

#ifdef __linux__
#include <unistd.h>
#elif _WIN64
#include "XGetopt.h"
#endif

using namespace std;

namespace
{
volatile sig_atomic_t signalStatus;
}

void signalHandler(int signal)
{
	signalStatus = signal;
	LogINFO << "signalHandler(): catch signal: " << signalStatus;

	XJAppServer::stopDetectors();
}

int main(int argc, char const *argv[])
{

	
	pid_t pid = getpid();
	struct sched_param param;
 	param.sched_priority = sched_get_priority_max(SCHED_FIFO);   // 也可用SCHED_RR
	sched_setscheduler(pid, SCHED_RR, &param);                   // 设置当前进程
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);   // 设置当前线程

	string usage = "Usage: xj_server -a address -p port -d plc_distance -c camera_type -l log_level -o logoutput -f mock_file_path \r\n \
   address     : IP address, eg.  127.0.0.1\r\n \
   port        : eg. 8080\r\n \
   plc_distance: integer\r\n \
   log_level   : integer (logCRITICAL = 0, logERROR = 1, logWARNING = 2, logDEBUG = 3, logINFO = 4)\r\n \
   camera_type : mock  (mock camera, default using real camera)\r\n \
   logoutput   : 0  (0: to terminal, default to log file)\r\n \
   file_path   : mock file path (only valid if camera type is mock)\r\n \
                   ";

	// parse the command line
	string webAddress = "localhost";
	int webPort = 8080;
	int plcDistance = 0;
	string cameraType = "mock";
#ifdef __linux__
	string mockFilePath = "/opt/videos/";
	string logFile = "/opt/xj_server.log";
#elif _WIN64
	string mockFilePath = "C:\\opt\\";
	string logFile = "C:\\opt\\xj_server.log";
#endif
	LogLevel logLevel = LogLevel::logDEBUG;
	int opt;

#ifdef __linux__
	while ((opt = getopt(argc, (char **)argv, "a:p:d:c:l:o:f:")) != -1)
	{
#elif _WIN64
	while ((opt = getopt(argc, (TCHAR **)argv, "a:p:d:c:l:o:f:")) != -1)
	{
#endif
		switch (opt)
		{
		case 'a':
			webAddress = optarg;
			break;
		case 'p':
			webPort = atoi(optarg);
			break;
		case 'd':
			plcDistance = atoi(optarg);
			break;
		case 'c':
			cameraType = optarg;
			break;
		case 'f':
			mockFilePath = optarg;
			break;
		case 'o':
			if (atoi(optarg) == 0)
				logFile = "";
			break;
		case 'l':
			logLevel = (LogLevel)atoi(optarg);
			break;
		case '?':
			cout << usage << endl;
			return 0;
		default:
			return 0;
		}
	}

	Logger::instance().setFileName(logFile);
	Logger::instance().setLoggerLevel(logLevel);
	LogINFO << "XJ SERVER " << PROJECT_NAME << "(" << PROJECT_VERSION << ") START...";

	// signal handling
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);
	signal(SIGABRT, signalHandler);
	signal(SIGSEGV, signalHandler);
#ifdef __linux__
	signal(SIGQUIT, signalHandler);
#endif

	// initial camera manager
	shared_ptr<CameraManager> pCameraManager = make_shared<CameraManager>();

	// initial IoManager
	int signalPulseWidth = CustomizedJsonConfig::instance().get<int>("SIGNAL_PULSE_WIDTH");
#ifdef _WIN64
	shared_ptr<BaseIoManager> pIoManager = make_shared<AppIoManager>("PCI-1730", signalPulseWidth);
#else
	shared_ptr<BaseIoManager> pIoManager = make_shared<AppIoManager>("PCI-1730", signalPulseWidth);
#endif

	// initial web server
	AppWebServer webServer(pCameraManager, webAddress, webPort);
	// AppWebServer webServer(pCameraManager, webAddress, webPort);
	shared_ptr<BaseIoManager> pPlcManager = make_shared<AppPlcManager>("192.168.3.1", 502, 2);
	webServer.setIoManager(pPlcManager);
	// webServer.setIoManager(pIoManager);

	HttpServer &httpServer = webServer.getServer();

	// initial xj server
	XJAppServer xjServer(pCameraManager, plcDistance);
	if (!xjServer.initXJApp(pIoManager))
	{
		LogCRITICAL << "Initial XJ app failed, exit...";
		RunningInfo::instance().GetRunningData().setCustomerDataByName("critical", "initial-XJ-server-failed");
		exit(-1);
	}
	webServer.setTotalBoards(xjServer.totalDetectors());

	string host = httpServer.config.address.empty() ? "localhost" : httpServer.config.address;
	cout << "starting web server at " << host << ":" << httpServer.config.port << "..." << endl;
	LogDEBUG << "Starting web server at " << host << ":" << httpServer.config.port << "...";

	// start web server
	thread webServerThread([&httpServer]() {
		try
		{
			httpServer.start();
		}
		catch (...)
		{
			LogCRITICAL << "Start web server failed, exit...";
			RunningInfo::instance().GetRunningData().setCustomerDataByName("critical", "start-web-server-failed");
			exit(-1);
		}
	});

	// wait for camera starts
	LogINFO << "Detectors have been initialized, and wait for cameras...";
	if (webServerThread.joinable() && xjServer.initCameraManager(cameraType, mockFilePath))
	{
		LogINFO << "Camera manager has been initialized";

		// start detector threads
		vector<thread> detectorThreads;
		for (int detectorIdx = 0; detectorIdx < xjServer.totalDetectors(); detectorIdx++)
		{
			LogINFO << "Detector thread " << detectorIdx << " started...";
			detectorThreads.emplace_back(&XJAppServer::startDetector, &xjServer, detectorIdx);
		}

		// detector thread join, main thread is waiting for detector thread to finish
		for (auto &itr : detectorThreads)
		{
			itr.join();
		}
	}

	// when we reach this point, the detector thread either exit or never start
	LogINFO << "All detector threads exited, we stop web server as well";
	httpServer.stop();

	// web server thread join, main thread is waiting for web thread to finish
	webServerThread.join();

	LogDEBUG << "XJ SERVER STOPPED";

	return 0;
}