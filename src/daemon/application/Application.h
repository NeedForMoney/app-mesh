#pragma once
#include <atomic>
#include <memory>
#include <string>
#include <map>
#include <mutex>
#include <chrono>
#include <cpprest/json.h>
#include "../TimerHandler.h"

class User;
class CounterPtr;
class GaugePtr;
class PrometheusRest;
class AppProcess;
class DailyLimitation;
class ResourceLimitation;
//////////////////////////////////////////////////////////////////////////
/// An Application is used to define and manage a process job.
//////////////////////////////////////////////////////////////////////////
class Application : public TimerHandler
{
	struct ShellAppFileGen
	{
		explicit ShellAppFileGen(const std::string& name, const std::string& cmd, const std::string& workDir);
		virtual ~ShellAppFileGen();
		const std::string& getShellStartCmd() const { return m_shellCmd; };
		const std::string& getShellFileName() const { return m_fileName; };
	private:
		std::string m_cmd;
		std::string m_shellCmd;
		std::string m_fileName;
	};
public:
	enum class STATUS : int
	{
		DISABLED,
		ENABLED,
		NOTAVIALABLE,	// used for temp app from RestHandler::apiRunParseApp and destroyed app
		INITIALIZING,
		UNINITIALIZING
	};
	enum class PERMISSION : int
	{
		GROUP_DENY = 1,
		GROUP_READ = 2,
		GROUP_WRITE = 3,
		OTHER_DENY = 10,
		OTHER_READ = 20,
		OTHER_WRITE = 30
	};
	Application();
	virtual ~Application();
	virtual bool operator==(const std::shared_ptr<Application>& app);

	virtual bool avialable();
	const std::string getName() const;
	bool isEnabled() const;
	bool isWorkingState() const;
	bool attach(int pid);

	static void FromJson(std::shared_ptr<Application>& app, const web::json::value& obj) noexcept(false);
	virtual web::json::value AsJson(bool returnRuntimeInfo);
	virtual void dump();

	// Invoke by scheduler
	virtual void invoke();
	virtual void disable();
	virtual void enable();
	void destroy();
	void onSuicideEvent(int timerId = 0);
	void onFinishEvent(int timerId = 0);
	void onEndEvent(int timerId = 0);

	std::string runAsyncrize(int timeoutSeconds) noexcept(false);
	std::string runSyncrize(int timeoutSeconds, void* asyncHttpRequest) noexcept(false);
	std::string getAsyncRunOutput(const std::string& processUuid, int& exitCode, bool& finished) noexcept(false);

	// health: 0-health, 1-unhealth
	void setHealth(bool health) { m_health = health; }
	const std::string& getHealthCheck() { return m_healthCheckCmd; }
	int getHealth() { return 1 - m_health; }
	pid_t getpid() const;

	// get normal stdout for running app
	std::string getOutput(bool keepHistory);

	void initMetrics(std::shared_ptr<PrometheusRest> prom);
	int getVersion();
	void setVersion(int version);
	const std::string& getMetadata() const { return m_metadata; }
	const std::string& getInitCmd() const { return m_commandLineInit; }
	const std::shared_ptr<User>& getOwner() const { return m_owner; }
	int getOwnerPermission() const { return m_ownerPermission; }
	bool isCloudApp() const;

protected:
	// Invoke immediately
	virtual void invokeNow(int timerId);
	virtual void refreshPid();
	std::shared_ptr<AppProcess> allocProcess(int cacheOutputLines, const std::string& dockerImage, const std::string& appName);
	bool isInDailyTimeRange();
	virtual void checkAndUpdateHealth();
	std::string runApp(int timeoutSeconds) noexcept(false);
	void handleEndTimer();
	const std::string getExecUser() const;
	const std::string& getCmdLine() const;

protected:
	STATUS m_status;
	std::string m_name;
	std::string m_commandLine;
	std::string m_commandLineInit;
	std::string m_commandLineFini;
	/// @brief TODO: when user is removed, need remove associated app, otherwise, app invoke will fail
	std::shared_ptr<User> m_owner;
	int m_ownerPermission;
	std::string m_workdir;
	std::string m_stdoutFile;
	std::string m_metadata;
	bool m_shellApp;
	std::shared_ptr<ShellAppFileGen> m_shellAppFile;
	//the exit code of last instance
	std::shared_ptr<int> m_return;
	std::string m_posixTimeZone;
	std::chrono::system_clock::time_point m_startTime;
	std::chrono::system_clock::time_point m_endTime;
	std::chrono::system_clock::time_point m_regTime;
	int m_endTimerId;
	bool m_health;
	std::string m_healthCheckCmd;
	const std::string m_appId;
	unsigned int m_version;
	std::shared_ptr<AppProcess> m_process;
	int m_pid;
	std::shared_ptr<DailyLimitation> m_dailyLimit;
	std::shared_ptr<ResourceLimitation> m_resourceLimit;
	std::map<std::string, std::string> m_envMap;
	std::string m_dockerImage;
	std::chrono::system_clock::time_point m_procStartTime;

	// Prometheus
	std::shared_ptr<CounterPtr> m_metricStartCount;
	std::shared_ptr<GaugePtr> m_metricMemory;
	std::atomic<int> m_continueFails;
};
