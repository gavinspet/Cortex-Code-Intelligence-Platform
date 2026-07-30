/**
 * @file Application.cpp
 * @brief Wires the dependency graph, initializes Drogon, registers HTTP routes, and manages application lifecycle
 *
 * @project Cortex Code Intelligence Platform
 *
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 *
 * @copyright Copyright (c) 2026 Kartick Kumar Ghosh
 * @license MIT
 */

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

        // 9. Create JobWorker (background processing)
        jobWorker_ = std::make_shared<JobWorker>(jobRepository_, analysisRepository_);
        Logger::instance().info("Registered JobWorker");
        
        // 10. Create WorkerService (worker lifecycle management)
        workerService_ = std::make_shared<WorkerService>(jobWorker_);
        Logger::instance().info("Registered WorkerService");

        // 11. Create AnalysisService and AnalysisController
        analysisService_ = std::make_shared<cortex::analysis::AnalysisService>(analysisRepository_);
        analysisController_ = std::make_shared<cortex::analysis::AnalysisController>(analysisService_);
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


static const std::string kLandingPageHtml = R"HTMLPAGE(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1.0"/>
<title>Cortex Code Intelligence Platform</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#0f172a;color:#e2e8f0;font-family:'Segoe UI',system-ui,sans-serif;min-height:100vh}
a{color:#6366f1;text-decoration:none}a:hover{text-decoration:underline}
.container{max-width:1100px;margin:0 auto;padding:0 24px}
header{background:linear-gradient(135deg,#1e293b,#0f172a);border-bottom:1px solid #1e293b;padding:48px 0 40px}
.hdr{display:flex;flex-direction:column;align-items:center;text-align:center;gap:12px}
.badge{background:#1e293b;border:1px solid #334155;border-radius:999px;padding:4px 14px;font-size:12px;color:#94a3b8;display:inline-flex;align-items:center;gap:6px;margin-bottom:8px}
.dot{width:8px;height:8px;border-radius:50%;background:#22c55e;box-shadow:0 0 6px #22c55e}
h1{font-size:clamp(1.8rem,5vw,3rem);font-weight:800;background:linear-gradient(135deg,#6366f1,#8b5cf6);-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text;line-height:1.2}
.sub{color:#94a3b8;font-size:clamp(.9rem,2vw,1.1rem);max-width:600px;line-height:1.6}
main{padding:48px 0}
.g2{display:grid;grid-template-columns:1fr 1fr;gap:24px;margin-bottom:24px}
@media(max-width:700px){.g2{grid-template-columns:1fr}}
.card{background:#1e293b;border:1px solid #334155;border-radius:16px;padding:28px;transition:border-color .2s}
.card:hover{border-color:#6366f1}
.ct{font-size:1rem;font-weight:700;color:#f1f5f9;margin-bottom:20px;text-transform:uppercase;letter-spacing:.05em}
.sg{display:grid;grid-template-columns:repeat(auto-fit,minmax(130px,1fr));gap:14px}
.st{background:#0f172a;border:1px solid #334155;border-radius:10px;padding:16px}
.sl{font-size:11px;text-transform:uppercase;letter-spacing:.08em;color:#64748b;margin-bottom:6px}
.sv{font-size:.95rem;font-weight:600;color:#f1f5f9}
.sr{color:#22c55e;display:flex;align-items:center;gap:6px}
.pulse{width:8px;height:8px;border-radius:50%;background:#22c55e;animation:p 2s infinite}
@keyframes p{0%,100%{box-shadow:0 0 0 0 rgba(34,197,94,.4)}50%{box-shadow:0 0 0 6px rgba(34,197,94,0)}}
.at{color:#94a3b8;line-height:1.7;font-size:.95rem;margin-bottom:16px}
.fl{list-style:none;display:grid;grid-template-columns:1fr 1fr;gap:6px 20px;margin-bottom:16px}
.fl li{color:#94a3b8;font-size:.9rem;display:flex;align-items:center;gap:8px}
.fl li::before{content:'';width:6px;height:6px;border-radius:50%;background:#6366f1;flex-shrink:0}
.fut{padding:14px;background:#0f172a;border-radius:8px;border-left:3px solid #6366f1}
.fl2{font-size:11px;text-transform:uppercase;letter-spacing:.08em;color:#6366f1;margin-bottom:8px;font-weight:600}
.fc{list-style:none;display:flex;flex-wrap:wrap;gap:8px}
.fc li{background:#1e293b;border:1px solid #334155;border-radius:999px;padding:3px 12px;font-size:12px;color:#94a3b8}
table{width:100%;border-collapse:collapse}
th{text-align:left;padding:10px 14px;font-size:11px;text-transform:uppercase;letter-spacing:.08em;color:#64748b;border-bottom:1px solid #334155}
td{padding:12px 14px;font-size:.9rem;border-bottom:1px solid #1a2744;vertical-align:middle}
tr:last-child td{border-bottom:none}
tr:hover td{background:#243148}
.mg{display:inline-block;padding:2px 10px;border-radius:999px;font-size:11px;font-weight:700}
.get{background:#1a3a2a;color:#22c55e;border:1px solid #22c55e40}
.post{background:#2a1a3a;color:#8b5cf6;border:1px solid #8b5cf640}
.ep{font-family:'Courier New',monospace;color:#e2e8f0}
.ed{color:#64748b;font-size:.85rem}
.dn{font-size:1.25rem;font-weight:700;color:#f1f5f9;margin-bottom:6px}
.dr{color:#6366f1;font-size:.9rem;margin-bottom:16px}
.sk{list-style:none;display:flex;flex-direction:column;gap:6px;margin-bottom:20px}
.sk li{color:#94a3b8;font-size:.9rem;display:flex;align-items:center;gap:8px}
.sk li::before{content:'';width:6px;height:6px;border-radius:50%;background:#22c55e;flex-shrink:0}
.bio{color:#64748b;font-size:.88rem;line-height:1.6;margin-bottom:20px}
.bg{display:flex;flex-wrap:wrap;gap:10px}
.btn{display:inline-flex;align-items:center;gap:6px;padding:8px 18px;border-radius:8px;font-size:.85rem;font-weight:600;text-decoration:none;transition:all .2s}
.b1{background:#6366f1;color:#fff;border:1px solid #6366f1}.b1:hover{background:#4f46e5;text-decoration:none}
.b2{background:transparent;color:#94a3b8;border:1px solid #334155}.b2:hover{border-color:#6366f1;color:#6366f1;text-decoration:none}
.lg{display:flex;flex-direction:column;gap:12px}
.li{display:flex;align-items:center;gap:12px;padding:14px;background:#0f172a;border:1px solid #334155;border-radius:10px;transition:border-color .2s}
.li:hover{border-color:#6366f1}
.ll{font-size:11px;text-transform:uppercase;letter-spacing:.08em;color:#64748b;margin-bottom:4px}
.lu{font-size:.9rem;color:#6366f1}
.chips{display:flex;flex-wrap:wrap;gap:10px}
.chip{background:#0f172a;border:1px solid #334155;border-radius:8px;padding:8px 14px;font-size:.85rem;color:#94a3b8;display:flex;align-items:center;gap:6px}
.cd{width:6px;height:6px;border-radius:50%;background:#6366f1}
footer{border-top:1px solid #1e293b;padding:28px 0;text-align:center;color:#475569;font-size:.85rem}
footer span{color:#64748b}
</style>
</head>
<body>
<header>
  <div class="container">
    <div class="hdr">
      <div class="badge"><span class="dot"></span>Live Production Service</div>
      <h1>&#x1F680; Cortex Code Intelligence Platform</h1>
      <p class="sub">Production-grade C++20 backend for AI-powered repository intelligence. Analyze GitHub repositories with high-performance async processing.</p>
    </div>
  </div>
</header>
<main><div class="container">
  <div class="g2">
    <div class="card">
      <div class="ct">&#x2705; Service Status</div>
      <div class="sg">
        <div class="st"><div class="sl">Status</div><div class="sv sr"><span class="pulse"></span>Running</div></div>
        <div class="st"><div class="sl">Environment</div><div class="sv">Production</div></div>
        <div class="st"><div class="sl">Version</div><div class="sv">v1.0.0</div></div>
        <div class="st"><div class="sl">Framework</div><div class="sv">Drogon</div></div>
        <div class="st"><div class="sl">Language</div><div class="sv">C++20</div></div>
        <div class="st"><div class="sl">Architecture</div><div class="sv">Clean Arch</div></div>
      </div>
    </div>
    <div class="card">
      <div class="ct">&#x2699;&#xFE0F; Tech Stack</div>
      <div class="chips">
        <div class="chip"><span class="cd"></span>C++20</div>
        <div class="chip"><span class="cd"></span>Drogon 1.9</div>
        <div class="chip"><span class="cd"></span>spdlog</div>
        <div class="chip"><span class="cd"></span>MySQL 8</div>
        <div class="chip"><span class="cd"></span>CMake</div>
        <div class="chip"><span class="cd"></span>Docker</div>
        <div class="chip"><span class="cd"></span>React 18</div>
        <div class="chip"><span class="cd"></span>Vite 6</div>
      </div>
    </div>
  </div>
  <div class="card" style="margin-bottom:24px">
    <div class="ct">&#x1F9E0; About the Project</div>
    <p class="at">Cortex Code Intelligence Platform accepts GitHub repository URLs, clones them asynchronously, and performs deep structural analysis returning language distribution, file counts, line counts, and directory maps via REST API. Built on Clean Architecture with Dependency Injection, Repository Pattern, and async background processing.</p>
    <ul class="fl">
      <li>Repository analysis</li><li>Language detection</li>
      <li>File and directory counting</li><li>Lines of code analysis</li>
      <li>Background job processing</li><li>REST API with polling</li>
    </ul>
    <div class="fut">
      <div class="fl2">&#x1F52D; Coming in v1.1+</div>
      <ul class="fc"><li>AI summaries</li><li>Dependency graphs</li><li>Static analysis</li><li>Architecture detection</li></ul>
    </div>
  </div>
  <div class="card" style="margin-bottom:24px">
    <div class="ct">&#x1F4E1; Available Endpoints</div>
    <table>
      <thead><tr><th>Method</th><th>Path</th><th>Description</th></tr></thead>
      <tbody>
        <tr><td><span class="mg get">GET</span></td><td><span class="ep">/health</span></td><td class="ed">Service health check with uptime and version</td></tr>
        <tr><td><span class="mg post">POST</span></td><td><span class="ep">/repositories</span></td><td class="ed">Submit a GitHub repo URL for analysis</td></tr>
        <tr><td><span class="mg get">GET</span></td><td><span class="ep">/jobs/{jobId}</span></td><td class="ed">Poll job status: QUEUED &#x2192; PROCESSING &#x2192; COMPLETED</td></tr>
        <tr><td><span class="mg get">GET</span></td><td><span class="ep">/analysis/{jobId}</span></td><td class="ed">Retrieve full analysis results</td></tr>
      </tbody>
    </table>
  </div>
  <div class="g2">
    <div class="card">
      <div class="ct">&#x1F468;&#x200D;&#x1F4BB; About the Developer</div>
      <div class="dn">Kartick Kumar Ghosh</div>
      <div class="dr">Software Engineer</div>
      <ul class="sk">
        <li>Modern C++ and Backend Engineering</li>
        <li>Distributed Systems</li>
        <li>Clean Architecture and System Design</li>
      </ul>
      <p class="bio">Passionate about building scalable backend systems and high-performance software. This project demonstrates production-grade C++20 design patterns, async processing, and Clean Architecture principles.</p>
      <div class="bg">
        <a class="btn b1" href="https://github.com/gavinspet" target="_blank">&#x2B50; GitHub</a>
        <a class="btn b2" href="https://linkedin.com" target="_blank">LinkedIn</a>
        <a class="btn b2" href="mailto:kartick.ghosh.dev@gmail.com">&#x2709; Email</a>
      </div>
    </div>
    <div class="card">
      <div class="ct">&#x1F517; Project Links</div>
      <div class="lg">
        <a class="li" href="https://github.com/gavinspet/Cortex-Code-Intelligence-Platform" target="_blank">
          <div><div class="ll">Repository</div><div class="lu">github.com/gavinspet/Cortex-Code-Intelligence-Platform</div></div>
        </a>
        <a class="li" href="https://github.com/gavinspet/Cortex-Code-Intelligence-Platform/blob/main/LICENSE" target="_blank">
          <div><div class="ll">License</div><div class="lu">MIT License</div></div>
        </a>
        <a class="li" href="/health">
          <div><div class="ll">Health Check</div><div class="lu">/health &#x2192; Live JSON status</div></div>
        </a>
      </div>
    </div>
  </div>
</div></main>
<footer><div class="container"><p>Built with &#x2764;&#xFE0F; using <span>C++20</span>, <span>Drogon</span> and <span>React</span> &nbsp;&middot;&nbsp; Copyright &copy; 2026 Kartick Kumar Ghosh</p></div></footer>
</body></html>)HTMLPAGE";

void Application::registerRoutes() {
    try {
        Logger::instance().info("Registering HTTP routes...");


        // Register GET / - HTML landing page
        drogon::app().registerHandler(
            "/",
            [](const drogon::HttpRequestPtr& /*req*/,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeCode(drogon::CT_TEXT_HTML);
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
                healthHandler.handle(req, std::move(callback));
            },
            {drogon::HttpMethod::Get}
        );
        
        Logger::instance().info("Registered route: GET /health");

        // Register POST /repositories endpoint using handler member function
        drogon::app().registerHandler(
            "/repositories",
            [](const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
                repositoryHandler.submitRepository(req, std::move(callback));
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
                jobHandler.getJob(req, std::move(callback), jobId);
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
                analysisHandler.getAnalysis(req, std::move(callback), jobId);
            },
            {drogon::HttpMethod::Get}
        );

        Logger::instance().info("Registered route: GET /analysis/{jobId}");
        Logger::instance().info("HTTP routes registered successfully");

    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Failed to register routes: ") + e.what());
    }
}

} // namespace cortex::app
