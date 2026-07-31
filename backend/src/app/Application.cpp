#include "app/Application.h"
#include "logging/Logger.h"
#include <drogon/HttpAppFramework.h>
#include <drogon/drogon.h>
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <csignal>
#include <atomic>
#include <functional>

namespace cortex::app {

using cortex::logging::Logger;

// Static application instance for handler registration
static Application* g_appInstance = nullptr;

// Handler wrapper using explicit lambda to work with Drogon's template system
// Drogon can properly introspect this since it matches the expected callback pattern
class HealthHandler {
public:
    void handle(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
        try {
            if (g_appInstance && g_appInstance->healthController_) {
                callback(g_appInstance->healthController_->handleHealth(req));
            } else {
                // Build JSON response manually (jsoncpp doesn't support initializer list syntax)
                Json::Value error;
                error["success"] = false;
                error["message"] = "Health service not available";
                
                auto response = drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k500InternalServerError);
                callback(response);
            }
        } catch (const std::exception& e) {
            cortex::logging::Logger::instance().error(std::string("Error in health handler: ") + e.what());
            
            Json::Value error;
            error["success"] = false;
            error["message"] = "Internal server error";
            
            auto response = drogon::HttpResponse::newHttpJsonResponse(error);
            response->setStatusCode(drogon::k500InternalServerError);
            callback(response);
        }
    }
};

static HealthHandler healthHandler;

// Handler wrapper for repository endpoint
class RepositoryHandler {
public:
    void submitRepository(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
        try {
            if (g_appInstance && g_appInstance->repositoryController_) {
                g_appInstance->repositoryController_->submitRepository(req, std::move(callback));
            } else {
                Json::Value error;
                error["success"] = false;
                error["message"] = "Repository service not available";
                
                auto response = drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k500InternalServerError);
                callback(response);
            }
        } catch (const std::exception& e) {
            cortex::logging::Logger::instance().error(std::string("Error in repository handler: ") + e.what());
            
            Json::Value error;
            error["success"] = false;
            error["message"] = "Internal server error";
            
            auto response = drogon::HttpResponse::newHttpJsonResponse(error);
            response->setStatusCode(drogon::k500InternalServerError);
            callback(response);
        }
    }
};

static RepositoryHandler repositoryHandler;

// Handler wrapper for job query endpoint
class JobHandler {
public:
    void getJob(const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
               const std::string& jobId) const {
        try {
            if (g_appInstance && g_appInstance->jobController_) {
                g_appInstance->jobController_->getJob(req, std::move(callback), jobId);
            } else {
                Json::Value error;
                error["success"] = false;
                error["message"] = "Job service not available";
                
                auto response = drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k500InternalServerError);
                callback(response);
            }
        } catch (const std::exception& e) {
            cortex::logging::Logger::instance().error(std::string("Error in job handler: ") + e.what());
            
            Json::Value error;
            error["success"] = false;
            error["message"] = "Internal server error";
            
            auto response = drogon::HttpResponse::newHttpJsonResponse(error);
            response->setStatusCode(drogon::k500InternalServerError);
            callback(response);
        }
    }
};

static JobHandler jobHandler;

// Handler for GET /analysis/{jobId}
class AnalysisHandler {
public:
    void getAnalysis(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                    const std::string& jobId) const {
        try {
            if (g_appInstance && g_appInstance->analysisController_) {
                g_appInstance->analysisController_->getAnalysis(req, std::move(callback), jobId);
            } else {
                Json::Value error;
                error["success"] = false;
                error["message"] = "Analysis service not available";
                auto response = drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k500InternalServerError);
                callback(response);
            }
        } catch (const std::exception& e) {
            cortex::logging::Logger::instance().error(
                std::string("Error in analysis handler: ") + e.what());
            Json::Value error;
            error["success"] = false;
            error["message"] = "Internal server error";
            auto response = drogon::HttpResponse::newHttpJsonResponse(error);
            response->setStatusCode(drogon::k500InternalServerError);
            callback(response);
        }
    }
};

static AnalysisHandler analysisHandler;
static std::atomic<bool> shutdown_requested(false);

// Signal handler
static void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        shutdown_requested.store(true);
        drogon::app().quit();  // Gracefully stop the Drogon server
    }
}

Application::Application(ConfigurationPtr config)
    : config_(config) {
    if (!config_) {
        throw std::invalid_argument("Configuration cannot be null");
    }
    
    // Set static instance for handler registration
    g_appInstance = this;
}

Application::~Application() = default;

void Application::buildDependencyGraph() noexcept {
    try {
        Logger::instance().info("Building dependency graph...");
        
        // Health services
        // 1. Create HealthService (demonstrates service layer pattern)
        healthService_ = std::make_shared<HealthService>();
        Logger::instance().info("Registered HealthService");
        
        // 2. Create HealthController (demonstrates controller layer pattern)
        healthController_ = std::make_shared<HealthController>(healthService_);
        Logger::instance().info("Registered HealthController");

        // Repository services
        // 3. Create job repository — try MySQL, fall back to InMemory
        bool mysqlOk = cortex::database::Database::instance().initialize();
        if (mysqlOk) {
            jobRepository_ = std::make_shared<cortex::infrastructure::MySQLJobRepository>();
            Logger::instance().info("Using MySQLJobRepository");
        } else {
            Logger::instance().warn("MySQL unavailable — falling back to InMemoryJobRepository");
            jobRepository_ = std::make_shared<cortex::infrastructure::InMemoryJobRepository>();
        }
        
        // 4. Create RepositoryService (business logic layer)
        repositoryService_ = std::make_shared<RepositoryService>(jobRepository_, 
                                                                  nullptr);  // Will set workerService after creating it
        Logger::instance().info("Registered RepositoryService");
        
        // 5. Create RepositoryController (HTTP handler layer)
        repositoryController_ = std::make_shared<RepositoryController>(repositoryService_);
        Logger::instance().info("Registered RepositoryController");

        // Job query services
        // 6. Create JobService (query business logic layer)
        jobService_ = std::make_shared<JobService>(jobRepository_);
        Logger::instance().info("Registered JobService");
        
        // 7. Create JobController (HTTP handler layer for job queries)
        jobController_ = std::make_shared<JobController>(jobService_);
        Logger::instance().info("Registered JobController");

        // Background worker services
        // 8. Create analysis repository (in-memory)
        analysisRepository_ = std::make_shared<cortex::analysis::InMemoryAnalysisRepository>();
        Logger::instance().info("Registered InMemoryAnalysisRepository");

        // 9. GitHub metadata components (needed before JobWorker)
        metadataRepository_ = std::make_shared<cortex::github::InMemoryGitHubMetadataRepository>();
        gitHubClient_ = std::make_shared<cortex::github::GitHubClient>();
        gitHubMetadataService_ = std::make_shared<cortex::github::GitHubMetadataService>(
            gitHubClient_, metadataRepository_);
        Logger::instance().info("Registered GitHubMetadataService");

        // 10. Technology detection service
        technologyRepository_ = std::make_shared<cortex::technology::InMemoryTechnologyRepository>();
        technologyService_ = std::make_shared<cortex::technology::TechnologyService>(
            technologyRepository_);
        Logger::instance().info("Registered TechnologyService");

        // 11. Create JobWorker with all services injected
        jobWorker_ = std::make_shared<JobWorker>(
            jobRepository_, analysisRepository_,
            gitHubMetadataService_, technologyService_);
        Logger::instance().info("Registered JobWorker (with GitHubMetadataService + TechnologyService)");

        // 11. Create WorkerService (worker lifecycle management)
        workerService_ = std::make_shared<WorkerService>(jobWorker_);
        Logger::instance().info("Registered WorkerService");

        // 12. Create AnalysisService and AnalysisController (with metadata + technology services)
        analysisService_ = std::make_shared<cortex::analysis::AnalysisService>(analysisRepository_);
        analysisController_ = std::make_shared<cortex::analysis::AnalysisController>(
            analysisService_, gitHubMetadataService_, technologyService_);
        Logger::instance().info("Registered AnalysisService and AnalysisController");
        
        // Now set the workerService in repositoryService for notifications
        if (repositoryService_) {
            repositoryService_->setWorkerService(workerService_);
            Logger::instance().info("Wired WorkerService into RepositoryService");
        }
        
        Logger::instance().info("Dependency graph built successfully");
    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Failed to build dependency graph: ") + e.what());
    }
}

int Application::run() noexcept {
    try {
        logStartupInfo();

        // Build dependency graph (DB init attempted inside)
        buildDependencyGraph();

        initializeDrogon();
        registerRoutes();

        // Start background worker before HTTP server
        if (workerService_) {
            workerService_->start();
            Logger::instance().info("Background job worker started");
        }

        // Register signal handlers for graceful shutdown
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        auto& app = drogon::HttpAppFramework::instance();

        // Start the framework (blocking)
        Logger::instance().info("Starting HTTP server...");
        app.run();

        // Stop background worker after HTTP server stops
        if (workerService_) {
            workerService_->stop();
            Logger::instance().info("Background job worker stopped");
        }

        // Shutdown database gracefully
        cortex::database::Database::instance().shutdown();

        logShutdownInfo();
        return 0;
    } catch (const std::exception& e) {
        Logger::instance().critical(std::string("Application error: ") + e.what());
        cortex::database::Database::instance().shutdown();
        return 1;
    } catch (...) {
        Logger::instance().critical("Unknown application error");
        cortex::database::Database::instance().shutdown();
        return 1;
    }
}

void Application::initializeDrogon() {
    auto& app = drogon::HttpAppFramework::instance();

    // Read configuration
    auto host = config_->getString("server.host").value_or("127.0.0.1");
    auto port = config_->getUInt("server.port").value_or(8080);
    auto threads = config_->getUInt("server.threads").value_or(4);

    // Configure Drogon
    app.addListener(host, port);
    app.setThreadNum(threads);

    Logger::instance().info("Drogon configured: host=" + host + ", port=" + std::to_string(port) + 
             ", threads=" + std::to_string(threads));
}

void Application::logStartupInfo() {
    Logger::instance().info("========================================");
    Logger::instance().info("Cortex Code Intelligence Platform");
    Logger::instance().info("Production Foundation");
    Logger::instance().info("========================================");
    Logger::instance().info("Loading configuration...");
}

void Application::logShutdownInfo() {
    Logger::instance().info("========================================");
    Logger::instance().info("Server shutting down gracefully...");
    Logger::instance().info("========================================");
}

static const std::string kLandingPageHtml = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1.0"/>
<title>Cortex — Code Intelligence Platform</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#0f172a;color:#e2e8f0;font-family:'Segoe UI',system-ui,sans-serif;min-height:100vh}
a{color:#6366f1;text-decoration:none}
a:hover{text-decoration:underline}
.container{max-width:1100px;margin:0 auto;padding:0 24px}
/* HEADER */
header{background:linear-gradient(135deg,#1e293b 0%,#0f172a 100%);border-bottom:1px solid #1e293b;padding:48px 0 40px}
.header-inner{display:flex;flex-direction:column;align-items:center;text-align:center;gap:12px}
.badge{display:inline-flex;align-items:center;gap:6px;background:#1e293b;border:1px solid #334155;border-radius:999px;padding:4px 14px;font-size:12px;color:#94a3b8;margin-bottom:8px}
.badge-dot{width:8px;height:8px;border-radius:50%;background:#22c55e;box-shadow:0 0 6px #22c55e}
h1{font-size:clamp(1.8rem,5vw,3rem);font-weight:800;background:linear-gradient(135deg,#6366f1,#8b5cf6);-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text;line-height:1.2}
.subtitle{color:#94a3b8;font-size:clamp(0.9rem,2vw,1.1rem);max-width:600px;line-height:1.6}
/* MAIN */
main{padding:48px 0}
.grid{display:grid;gap:24px}
.grid-2{grid-template-columns:1fr 1fr}
@media(max-width:700px){.grid-2{grid-template-columns:1fr}}
/* CARD */
.card{background:#1e293b;border:1px solid #334155;border-radius:16px;padding:28px;transition:border-color .2s}
.card:hover{border-color:#6366f1}
.card-title{display:flex;align-items:center;gap:10px;font-size:1rem;font-weight:700;color:#f1f5f9;margin-bottom:20px;text-transform:uppercase;letter-spacing:.05em}
.card-icon{font-size:1.2rem}
/* STATUS */
.status-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:14px}
.stat{background:#0f172a;border:1px solid #334155;border-radius:10px;padding:16px}
.stat-label{font-size:11px;text-transform:uppercase;letter-spacing:.08em;color:#64748b;margin-bottom:6px}
.stat-value{font-size:0.95rem;font-weight:600;color:#f1f5f9}
.status-running{color:#22c55e;display:flex;align-items:center;gap:6px}
.pulse{width:8px;height:8px;border-radius:50%;background:#22c55e;animation:pulse 2s infinite}
@keyframes pulse{0%,100%{box-shadow:0 0 0 0 rgba(34,197,94,.4)}50%{box-shadow:0 0 0 6px rgba(34,197,94,0)}}
/* ABOUT */
.about-text{color:#94a3b8;line-height:1.7;font-size:0.95rem;margin-bottom:16px}
.feature-list{list-style:none;display:grid;grid-template-columns:1fr 1fr;gap:6px 20px}
.feature-list li{color:#94a3b8;font-size:0.9rem;display:flex;align-items:center;gap:8px}
.feature-list li::before{content:'';width:6px;height:6px;border-radius:50%;background:#6366f1;flex-shrink:0}
.future{margin-top:16px;padding:14px;background:#0f172a;border-radius:8px;border-left:3px solid #6366f1}
.future-label{font-size:11px;text-transform:uppercase;letter-spacing:.08em;color:#6366f1;margin-bottom:8px;font-weight:600}
.future-list{list-style:none;display:flex;flex-wrap:wrap;gap:8px}
.future-list li{background:#1e293b;border:1px solid #334155;border-radius:999px;padding:3px 12px;font-size:12px;color:#94a3b8}
/* ENDPOINTS TABLE */
.endpoints-table{width:100%;border-collapse:collapse}
.endpoints-table th{text-align:left;padding:10px 14px;font-size:11px;text-transform:uppercase;letter-spacing:.08em;color:#64748b;border-bottom:1px solid #334155}
.endpoints-table td{padding:12px 14px;font-size:0.9rem;border-bottom:1px solid #1a2744;vertical-align:middle}
.endpoints-table tr:last-child td{border-bottom:none}
.endpoints-table tr:hover td{background:#243148}
.method{display:inline-block;padding:2px 10px;border-radius:999px;font-size:11px;font-weight:700;letter-spacing:.04em}
.method-get{background:#1a3a2a;color:#22c55e;border:1px solid #22c55e40}
.method-post{background:#2a1a3a;color:#8b5cf6;border:1px solid #8b5cf640}
.endpoint-path{font-family:'Courier New',monospace;color:#e2e8f0;font-size:0.9rem}
.endpoint-desc{color:#64748b;font-size:0.85rem}
/* DEVELOPER */
.dev-name{font-size:1.25rem;font-weight:700;color:#f1f5f9;margin-bottom:6px}
.dev-role{color:#6366f1;font-size:0.9rem;margin-bottom:16px}
.skills{list-style:none;display:flex;flex-direction:column;gap:6px;margin-bottom:20px}
.skills li{color:#94a3b8;font-size:0.9rem;display:flex;align-items:center;gap:8px}
.skills li::before{content:'';width:6px;height:6px;border-radius:50%;background:#22c55e;flex-shrink:0}
.dev-bio{color:#64748b;font-size:0.88rem;line-height:1.6;margin-bottom:20px}
.btn-group{display:flex;flex-wrap:wrap;gap:10px}
.btn{display:inline-flex;align-items:center;gap:6px;padding:8px 18px;border-radius:8px;font-size:0.85rem;font-weight:600;text-decoration:none;transition:all .2s}
.btn-primary{background:#6366f1;color:#fff;border:1px solid #6366f1}
.btn-primary:hover{background:#4f46e5;text-decoration:none}
.btn-outline{background:transparent;color:#94a3b8;border:1px solid #334155}
.btn-outline:hover{border-color:#6366f1;color:#6366f1;text-decoration:none}
/* LINKS */
.links-grid{display:flex;flex-direction:column;gap:12px}
.link-item{display:flex;align-items:center;gap:12px;padding:14px;background:#0f172a;border:1px solid #334155;border-radius:10px;transition:border-color .2s}
.link-item:hover{border-color:#6366f1}
.link-label{font-size:11px;text-transform:uppercase;letter-spacing:.08em;color:#64748b;margin-bottom:4px}
.link-url{font-size:0.9rem;color:#6366f1}
/* TECH STACK */
.stack-chips{display:flex;flex-wrap:wrap;gap:10px}
.chip{background:#0f172a;border:1px solid #334155;border-radius:8px;padding:8px 14px;font-size:0.85rem;color:#94a3b8;display:flex;align-items:center;gap:6px}
.chip-dot{width:6px;height:6px;border-radius:50%;background:#6366f1}
/* FOOTER */
footer{border-top:1px solid #1e293b;padding:28px 0;text-align:center;color:#475569;font-size:0.85rem}
footer span{color:#64748b}
</style>
</head>
<body>
<header>
  <div class="container">
    <div class="header-inner">
      <div class="badge"><span class="badge-dot"></span>Live Production Service</div>
      <h1>🚀 Cortex Code Intelligence Platform</h1>
      <p class="subtitle">Production-grade C++20 backend for AI-powered repository intelligence. Analyze GitHub repositories with high-performance async processing.</p>
    </div>
  </div>
</header>

<main>
  <div class="container">

    <!-- Status + About -->
    <div class="grid grid-2" style="margin-bottom:24px">
      <div class="card">
        <div class="card-title"><span class="card-icon">✅</span>Service Status</div>
        <div class="status-grid">
          <div class="stat">
            <div class="stat-label">Status</div>
            <div class="stat-value status-running"><span class="pulse"></span>Running</div>
          </div>
          <div class="stat">
            <div class="stat-label">Environment</div>
            <div class="stat-value">Production</div>
          </div>
          <div class="stat">
            <div class="stat-label">Version</div>
            <div class="stat-value">v1.0.0</div>
          </div>
          <div class="stat">
            <div class="stat-label">Framework</div>
            <div class="stat-value">Drogon</div>
          </div>
          <div class="stat">
            <div class="stat-label">Language</div>
            <div class="stat-value">C++20</div>
          </div>
          <div class="stat">
            <div class="stat-label">Architecture</div>
            <div class="stat-value">Clean Arch</div>
          </div>
        </div>
      </div>

      <div class="card">
        <div class="card-title"><span class="card-icon">⚙️</span>Tech Stack</div>
        <div class="stack-chips">
          <div class="chip"><span class="chip-dot"></span>C++20</div>
          <div class="chip"><span class="chip-dot"></span>Drogon 1.9</div>
          <div class="chip"><span class="chip-dot"></span>spdlog</div>
          <div class="chip"><span class="chip-dot"></span>MySQL 8</div>
          <div class="chip"><span class="chip-dot"></span>CMake</div>
          <div class="chip"><span class="chip-dot"></span>Docker</div>
          <div class="chip"><span class="chip-dot"></span>React 18</div>
          <div class="chip"><span class="chip-dot"></span>Vite 6</div>
        </div>
      </div>
    </div>

    <!-- About -->
    <div class="card" style="margin-bottom:24px">
      <div class="card-title"><span class="card-icon">🧠</span>About the Project</div>
      <p class="about-text">Cortex Code Intelligence Platform accepts GitHub repository URLs, clones them asynchronously, and performs deep structural analysis — returning language distribution, file counts, line counts, and directory maps via REST API. Built on a layered Clean Architecture with Dependency Injection, Repository Pattern, and async background processing.</p>
      <ul class="feature-list">
        <li>Repository analysis</li>
        <li>Language detection</li>
        <li>File &amp; directory counting</li>
        <li>Lines of code analysis</li>
        <li>Background job processing</li>
        <li>REST API (202 Accepted + polling)</li>
      </ul>
      <div class="future">
        <div class="future-label">🔭 Coming in v1.1+</div>
        <ul class="future-list">
          <li>AI summaries</li>
          <li>Dependency graphs</li>
          <li>Static analysis</li>
          <li>Architecture detection</li>
        </ul>
      </div>
    </div>

    <!-- Endpoints -->
    <div class="card" style="margin-bottom:24px">
      <div class="card-title"><span class="card-icon">📡</span>Available Endpoints</div>
      <table class="endpoints-table">
        <thead>
          <tr>
            <th>Method</th>
            <th>Path</th>
            <th>Description</th>
          </tr>
        </thead>
        <tbody>
          <tr>
            <td><span class="method method-get">GET</span></td>
            <td><span class="endpoint-path">/health</span></td>
            <td class="endpoint-desc">Service health check with uptime and version</td>
          </tr>
          <tr>
            <td><span class="method method-post">POST</span></td>
            <td><span class="endpoint-path">/repositories</span></td>
            <td class="endpoint-desc">Submit a GitHub repo URL for analysis (returns jobId)</td>
          </tr>
          <tr>
            <td><span class="method method-get">GET</span></td>
            <td><span class="endpoint-path">/jobs/{jobId}</span></td>
            <td class="endpoint-desc">Poll job status (QUEUED → PROCESSING → COMPLETED)</td>
          </tr>
          <tr>
            <td><span class="method method-get">GET</span></td>
            <td><span class="endpoint-path">/analysis/{jobId}</span></td>
            <td class="endpoint-desc">Retrieve full analysis results for a completed job</td>
          </tr>
        </tbody>
      </table>
    </div>

    <!-- Developer + Project Links -->
    <div class="grid grid-2">
      <div class="card">
        <div class="card-title"><span class="card-icon">👨‍💻</span>About the Developer</div>
        <div class="dev-name">Kartick Kumar Ghosh</div>
        <div class="dev-role">Software Engineer</div>
        <ul class="skills">
          <li>Modern C++ &amp; Backend Engineering</li>
          <li>Distributed Systems</li>
          <li>Clean Architecture</li>
          <li>System Design</li>
        </ul>
        <p class="dev-bio">Passionate about building scalable backend systems and high-performance software. This project demonstrates production-grade C++20 design patterns, async processing, and Clean Architecture principles.</p>
        <div class="btn-group">
          <a class="btn btn-primary" href="https://github.com/gavinspet" target="_blank">⭐ GitHub</a>
          <a class="btn btn-outline" href="https://linkedin.com" target="_blank">LinkedIn</a>
          <a class="btn btn-outline" href="mailto:kartick.ghosh.dev@gmail.com">✉ Email</a>
        </div>
      </div>

      <div class="card">
        <div class="card-title"><span class="card-icon">🔗</span>Project Links</div>
        <div class="links-grid">
          <a class="link-item" href="https://github.com/gavinspet/Cortex-Code-Intelligence-Platform" target="_blank">
            <div>
              <div class="link-label">Repository</div>
              <div class="link-url">github.com/gavinspet/Cortex-Code-Intelligence-Platform</div>
            </div>
          </a>
          <a class="link-item" href="https://github.com/gavinspet/Cortex-Code-Intelligence-Platform/blob/main/LICENSE" target="_blank">
            <div>
              <div class="link-label">License</div>
              <div class="link-url">MIT License — Free to use and modify</div>
            </div>
          </a>
          <a class="link-item" href="/health" target="_blank">
            <div>
              <div class="link-label">Health Check</div>
              <div class="link-url">/health → Live service status JSON</div>
            </div>
          </a>
        </div>
      </div>
    </div>

  </div>
</main>

<footer>
  <div class="container">
    <p>Built with ❤️ using <span>C++20</span>, <span>Drogon</span> and <span>React</span> &nbsp;·&nbsp; Copyright © 2026 Kartick Kumar Ghosh</p>
  </div>
</footer>
</body>
</html>)HTML";

void Application::registerRoutes() {
    try {
        Logger::instance().info("Registering HTTP routes...");

        // Register GET / — landing page
        drogon::app().registerHandler(
            "/",
            [](const drogon::HttpRequestPtr& /*req*/,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeCode(drogon::CT_TEXT_HTML);
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
                resp->setBody(kLandingPageHtml);
                callback(resp);
            },
            {drogon::HttpMethod::Get}
        );

        Logger::instance().info("Registered route: GET /");

        // Register GET /health endpoint using handler member function
        // Bind to the handler instance's method
        drogon::app().registerHandler(
            "/health",
            [](const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
                auto corsCallback = [cb = std::move(callback)](const drogon::HttpResponsePtr& resp) {
                    resp->addHeader("Access-Control-Allow-Origin", "*");
                    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
                    resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
                    cb(resp);
                };
                healthHandler.handle(req, std::move(corsCallback));
            },
            {drogon::HttpMethod::Get}
        );
        
        Logger::instance().info("Registered route: GET /health");

        // Register POST /repositories endpoint using handler member function
        drogon::app().registerHandler(
            "/repositories",
            [](const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
                auto corsCallback = [cb = std::move(callback)](const drogon::HttpResponsePtr& resp) {
                    resp->addHeader("Access-Control-Allow-Origin", "*");
                    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
                    resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
                    cb(resp);
                };
                repositoryHandler.submitRepository(req, std::move(corsCallback));
            },
            {drogon::HttpMethod::Post}
        );
        
        Logger::instance().info("Registered route: POST /repositories");
        
        // Register GET /jobs/{jobId} endpoint using handler member function
        drogon::app().registerHandler(
            "/jobs/{jobId}",
            [](const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
               const std::string& jobId) {
                auto corsCallback = [cb = std::move(callback)](const drogon::HttpResponsePtr& resp) {
                    resp->addHeader("Access-Control-Allow-Origin", "*");
                    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
                    resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
                    cb(resp);
                };
                jobHandler.getJob(req, std::move(corsCallback), jobId);
            },
            {drogon::HttpMethod::Get}
        );
        
        Logger::instance().info("Registered route: GET /jobs/{jobId}");

        // Register GET /analysis/{jobId} endpoint
        drogon::app().registerHandler(
            "/analysis/{jobId}",
            [](const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
               const std::string& jobId) {
                auto corsCallback = [cb = std::move(callback)](const drogon::HttpResponsePtr& resp) {
                    resp->addHeader("Access-Control-Allow-Origin", "*");
                    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
                    resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
                    cb(resp);
                };
                analysisHandler.getAnalysis(req, std::move(corsCallback), jobId);
            },
            {drogon::HttpMethod::Get}
        );

        Logger::instance().info("Registered route: GET /analysis/{jobId}");

        // Register OPTIONS handlers for CORS preflight requests on each endpoint
        auto corsOptionsHandler = [](const drogon::HttpRequestPtr& req,
                                     std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k200OK);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "86400");
            callback(resp);
        };

        // OPTIONS for /repositories
        drogon::app().registerHandler("/repositories", corsOptionsHandler, {drogon::HttpMethod::Options});
        Logger::instance().info("Registered route: OPTIONS /repositories");

        // OPTIONS for /jobs/{jobId}
        drogon::app().registerHandler("/jobs/{jobId}", corsOptionsHandler, {drogon::HttpMethod::Options});
        Logger::instance().info("Registered route: OPTIONS /jobs/{jobId}");

        // OPTIONS for /analysis/{jobId}
        drogon::app().registerHandler("/analysis/{jobId}", corsOptionsHandler, {drogon::HttpMethod::Options});
        Logger::instance().info("Registered route: OPTIONS /analysis/{jobId}");

        // OPTIONS for /health
        drogon::app().registerHandler("/health", corsOptionsHandler, {drogon::HttpMethod::Options});
        Logger::instance().info("Registered route: OPTIONS /health");

        Logger::instance().info("HTTP routes registered successfully");

    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Failed to register routes: ") + e.what());
    }
}

} // namespace cortex::app
