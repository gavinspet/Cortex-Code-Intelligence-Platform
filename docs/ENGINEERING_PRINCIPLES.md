# Cortex Code Intelligence Platform

## Engineering Principles & Standards

**Version:** 1.0  
**Last Updated:** 2026-07-28  
**Audience:** Cortex Engineering Team

---

## 1. Vision

Cortex Code Intelligence Platform is a **production-grade, distributed C++20 backend system** built to demonstrate and exemplify senior-level software engineering practices. 

Our commitment is not just to build software that works, but to build software that:

- Is **maintainable** by multiple engineers over years
- Is **testable** at every layer without compromise
- Is **scalable** from single-server to distributed architecture
- Is **secure** from inception through deployment
- Is **performant** where it matters, simple where it's sufficient
- Is **documented** such that future engineers understand not just *what* but *why*
- Serves as a **reference implementation** of enterprise-grade backend architecture

Every line of code in Cortex is written with the understanding that this project may be examined by senior engineers from Google, Microsoft, Meta, or Uber. We build not to pass interviews—we build to define the standard.

---

## 2. Engineering Philosophy

Our philosophy guides every decision, from architecture to code review.

### 2.1 Core Principles

**Build for Maintainability First**

- Code is read 10x more than it is written
- The primary consumer of our code is the next engineer (often yourself in 6 months)
- Clarity and simplicity beat cleverness every time
- A 10% performance gain that reduces code clarity is not worth it

**Readability Over Cleverness**

- Avoid unnecessary C++ template metaprogramming
- Avoid overly compressed code
- Explicit is better than implicit
- If you use advanced C++ features, document *why* they were chosen

**Performance Where It Matters**

- Profile before optimizing
- Focus optimization effort on the critical path (typically <10% of code)
- Never sacrifice correctness for premature optimization
- Measure impact of optimizations with benchmarks

**Testability as a First-Class Concern**

- Architecture should enable testing without excessive mocking
- Dependency Injection is non-negotiable
- A function that can't be tested easily is a sign of poor design
- Unit tests should be fast enough to run on every keystroke

**Scalability Through Design**

- Write code assuming it will scale from single-node to distributed
- Use patterns (async, queueing, eventual consistency) from day one
- Document limitations and scaling boundaries
- Plan for distributed tracing and observability

**Extensibility Without Modification**

- Follow Open/Closed Principle
- Use Interfaces for extension points
- New features should rarely require modifying existing classes
- Plugin architecture where appropriate

**Simplicity**

- Favor fewer dependencies
- Favor standard patterns over novel approaches
- Favor established libraries over homegrown solutions
- Complexity should be justified and documented

**Explicitness Over Magic**

- Avoid framework magic that hides behavior
- Configuration should be explicit, not discovered through conventions
- Dependency graphs should be traceable
- Dependency Injection container behavior should be obvious

**Production-First Mindset**

- Write code assuming it will run in production at scale
- Plan for failure modes before they occur
- Monitor-ability is not an afterthought
- Security is built in, not bolted on

---

## 3. SOLID Principles

SOLID principles form the foundation of our architecture. Every engineer must understand and apply them.

### 3.1 Single Responsibility Principle (SRP)

**Definition:**  
A class should have one, and only one, reason to change.

**Why It Matters:**

- Classes with multiple responsibilities become harder to understand
- Changes to one responsibility force changes to an unrelated class
- Testing becomes difficult when classes mix concerns
- Reusability decreases as responsibilities entangle

**Good Example:**

```cpp
// Good: Logger has one responsibility - to log
class Logger {
public:
    void info(const std::string& message);
    void error(const std::string& message);
};

// Good: Configuration has one responsibility - to provide config
class Configuration {
public:
    std::optional<std::string> getString(const std::string& key);
    std::optional<int> getInt(const std::string& key);
};
```

**Bad Example:**

```cpp
// Bad: Application does everything
class Application {
public:
    void loadConfig();              // Configuration concern
    void initializeDatabase();      // Database concern
    void setupLogging();            // Logging concern
    void startHttpServer();         // HTTP concern
    void handleRequest();           // Business logic concern
    void parseJSON();               // JSON parsing concern
    // ... 20 more responsibilities
};
```

**How Cortex Applies It:**

- **Application**: Only orchestrates startup/shutdown
- **Configuration**: Only manages configuration access
- **Logger**: Only responsible for logging
- **Controllers**: Only handle HTTP routing, delegate to services
- **Services**: Only contain business logic
- **Repositories**: Only handle persistence

Each class changes for exactly one business reason.

---

### 3.2 Open/Closed Principle (OCP)

**Definition:**  
Software entities should be open for extension but closed for modification.

**Why It Matters:**

- Adding new features shouldn't require modifying existing code
- Existing code is already tested and in production
- Risk of breaking existing functionality decreases
- Codebase scales as new features are added

**Good Example:**

```cpp
// Good: Use interfaces for extension
class DatabaseConnection {
    virtual ~DatabaseConnection() = default;
    virtual bool execute(const std::string& query) = 0;
};

class PostgreSQLConnection : public DatabaseConnection {
    bool execute(const std::string& query) override { /* ... */ }
};

class MySQLConnection : public DatabaseConnection {
    bool execute(const std::string& query) override { /* ... */ }
};

// New database type? Just add new class, don't modify existing code
```

**Bad Example:**

```cpp
// Bad: Must modify existing code to add new database type
class DatabaseConnection {
public:
    bool execute(const std::string& query, const std::string& type) {
        if (type == "postgres") { /* ... */ }
        else if (type == "mysql") { /* ... */ }
        else if (type == "mongodb") { /* ... */ }  // Must modify this!
    }
};
```

**How Cortex Applies It:**

- **Configuration Sources**: New sources added via new implementations of `Configuration` interface
- **Logging Backends**: New backends added via new implementations of `Logger` interface
- **Request Handlers**: New endpoints added via new controllers, not modifying existing ones
- **Data Access**: New repository types don't modify existing repositories

---

### 3.3 Liskov Substitution Principle (LSP)

**Definition:**  
Objects of a superclass should be replaceable with objects of a subclass without breaking the application.

**Why It Matters:**

- Inheritance hierarchies must be semantically correct
- Subclasses must honor the contract of their parent
- Violating LSP leads to runtime errors and surprising behavior
- Code relying on base class can't make assumptions about subtypes

**Good Example:**

```cpp
// Good: Subclass honors the base class contract
class Logger {
public:
    virtual ~Logger() = default;
    virtual void info(const std::string& msg) noexcept = 0;
};

class FileLogger : public Logger {
    void info(const std::string& msg) noexcept override {
        // Honors contract: noexcept, takes string, logs info level
    }
};

class ConsoleLogger : public Logger {
    void info(const std::string& msg) noexcept override {
        // Honors contract: noexcept, takes string, logs info level
    }
};

// Can be used interchangeably
void startServer(std::shared_ptr<Logger> logger) {
    logger->info("Server starting");  // Works with any Logger implementation
}
```

**Bad Example:**

```cpp
// Bad: Subclass violates the contract
class Logger {
public:
    virtual void log(const std::string& msg) = 0;
};

class FileLogger : public Logger {
    void log(const std::string& msg) override {
        if (msg.empty()) throw std::invalid_argument("Empty message");
        // Violates contract! Base class didn't say this could throw
    }
};
```

**How Cortex Applies It:**

- All interface implementations honor the base class contract
- Exceptions only thrown if base class specifies they can be
- Return values have consistent semantics across implementations
- Preconditions and postconditions don't change in subclasses

---

### 3.4 Interface Segregation Principle (ISP)

**Definition:**  
Clients should not be forced to depend on interfaces they don't use.

**Why It Matters:**

- Fat interfaces create tight coupling
- Classes implementing fat interfaces must implement methods they don't need
- Changes to one part of an interface affect unrelated clients
- Smaller interfaces are easier to implement and test

**Good Example:**

```cpp
// Good: Segregated interfaces
class Logger {
public:
    virtual ~Logger() = default;
    virtual void info(const std::string& msg) noexcept = 0;
    virtual void error(const std::string& msg) noexcept = 0;
};

class LoggerConfig {
public:
    virtual ~LoggerConfig() = default;
    virtual void setLevel(LogLevel level) noexcept = 0;
};

// Clients only depend on what they need
class Application {
public:
    Application(std::shared_ptr<Logger> logger) : logger_(logger) {}
private:
    std::shared_ptr<Logger> logger_;  // Doesn't care about config
};
```

**Bad Example:**

```cpp
// Bad: Fat interface
class Logger {
public:
    virtual void log(LogLevel level, const std::string& msg) = 0;
    virtual void setLevel(LogLevel level) = 0;
    virtual void flush() = 0;
    virtual void setOutputFile(const std::string& path) = 0;
    virtual void rotate() = 0;
    virtual std::vector<std::string> getLogFiles() const = 0;
    virtual void deleteOldLogs() = 0;
    // ... 10 more methods
};
// Clients forced to know about all this even if they only log
```

**How Cortex Applies It:**

- **Logger**: Interface only has logging methods, not configuration
- **Configuration**: Interface only has read methods
- **Repository**: Interface has only data access methods
- **Service**: Interfaces are role-based, not implementation-based

---

### 3.5 Dependency Inversion Principle (DIP)

**Definition:**  
High-level modules should not depend on low-level modules. Both should depend on abstractions. Abstractions should not depend on details. Details should depend on abstractions.

**Why It Matters:**

- Decouples high-level business logic from low-level infrastructure
- Makes testing possible through mock implementations
- Allows swapping implementations without changing high-level code
- Enables parallel development of different layers

**Good Example:**

```cpp
// Good: Both depend on abstraction
class Configuration {
public:
    virtual ~Configuration() = default;
    virtual std::optional<std::string> getString(const std::string& key) = 0;
};

class Application {
public:
    Application(std::shared_ptr<Configuration> config) : config_(config) {}
private:
    std::shared_ptr<Configuration> config_;  // Depends on abstraction
};

// Low-level implementation
class FileConfiguration : public Configuration {
    std::optional<std::string> getString(const std::string& key) override { /* ... */ }
};

// Application doesn't know about FileConfiguration, only Configuration
```

**Bad Example:**

```cpp
// Bad: High-level depends on low-level
class Application {
public:
    Application() {
        config_ = std::make_unique<FileConfiguration>("config.json");
    }
private:
    std::unique_ptr<FileConfiguration> config_;  // Tight coupling!
};

// Now we can't test with mock config, can't swap implementations
```

**How Cortex Applies It:**

- **Application** depends on `Configuration` interface, not implementations
- **Application** depends on `Logger` interface, not spdlog directly
- **Services** depend on `Repository` interfaces, not concrete repositories
- **Controllers** depend on `Service` interfaces, not implementations
- All dependencies are injected, not created internally

---

## 4. Architectural Principles

### 4.1 Clean Architecture

Clean Architecture separates the application into concentric layers, each with clear responsibilities:

```
┌─────────────────────────────────────┐
│      User Interface/API              │  (Controllers)
├─────────────────────────────────────┤
│      Use Cases/Business Logic        │  (Services)
├─────────────────────────────────────┤
│      Interface Adapters             │  (Repositories, Mappers)
├─────────────────────────────────────┤
│      Frameworks & Drivers           │  (Database, HTTP, Config)
└─────────────────────────────────────┘

Dependencies flow INWARD only.
Outer layers know about inner layers.
Inner layers know NOTHING about outer layers.
```

**In Cortex:**

- **Controllers** (outermost): HTTP routing, request/response
- **Services** (middle): Business logic, orchestration
- **Repositories** (middle): Data access abstraction
- **Entities** (innermost): Core business objects, no dependencies

### 4.2 Layered Architecture

Cortex follows strict layered architecture:

```
Request → Controller → Service → Repository → Database
  ↓          ↓           ↓           ↓
Response← Logger      Logger     Logger
         (Config)    (Config)    (Config)
```

**Rules:**

- Controllers never call Repositories (skip Service layer → violates architecture)
- Services never know HTTP details (no HttpRequest/HttpResponse)
- Repositories never contain business logic
- Each layer communicates through interfaces

### 4.3 Dependency Injection (DI)

All dependencies are injected, never created internally.

**Pattern:**

```cpp
class Service {
public:
    Service(std::shared_ptr<Repository> repo, std::shared_ptr<Logger> logger)
        : repo_(repo), logger_(logger) {}
private:
    std::shared_ptr<Repository> repo_;
    std::shared_ptr<Logger> logger_;
};

// Created by factory, not by Service
auto service = ApplicationFactory::createService(repo, logger);
```

**Benefits:**

- Services are testable (inject mocks)
- Dependencies are visible in constructor
- No static state or singletons
- Easy to replace implementations

### 4.4 High Cohesion

Cohesion measures how closely methods in a class relate to each other.

**High Cohesion (Good):**

```cpp
class UserService {
public:
    User create(const std::string& name);
    std::optional<User> findById(int id);
    bool update(const User& user);
    bool delete(int id);
};
// All methods are related to user operations
```

**Low Cohesion (Bad):**

```cpp
class UserService {
public:
    User createUser(const std::string& name);           // User management
    bool sendEmail(const std::string& to);              // Emails
    double calculateShippingCost(double weight);        // Shipping
    void logAnalytics(const std::string& event);        // Analytics
};
// Unrelated responsibilities
```

### 4.5 Low Coupling

Coupling measures how much classes depend on each other.

**Low Coupling (Good):**

```cpp
class Controller {
    Controller(std::shared_ptr<Service> service) : service_(service) {}
    // Only knows about Service interface
private:
    std::shared_ptr<Service> service_;
};
```

**High Coupling (Bad):**

```cpp
class Controller {
    Controller() {
        service_ = std::make_unique<UserServiceImpl>();
        repo_ = std::make_unique<PostgreSQLRepository>();
        db_ = std::make_unique<Database>();
        config_ = std::make_unique<FileConfiguration>();
        logger_ = std::make_unique<SpdlogLogger>();
    }
    // Tightly coupled to all implementations
};
```

### 4.6 Separation of Concerns

Each component has exactly one area of concern.

| Component | Concern |
|-----------|---------|
| Controller | HTTP routing and request/response |
| Service | Business logic and orchestration |
| Repository | Data persistence |
| Configuration | Configuration values |
| Logger | Structured logging |
| Mapper | Object translation |

### 4.7 Composition Over Inheritance

Favor object composition (has-a) over class inheritance (is-a).

**Composition (Good):**

```cpp
class User {
private:
    std::unique_ptr<EmailValidator> email_validator_;
    std::unique_ptr<PasswordValidator> pwd_validator_;
public:
    bool isValid() const {
        return email_validator_->validate(email_) &&
               pwd_validator_->validate(password_);
    }
};
```

**Inheritance (Bad):**

```cpp
class BaseValidator { /* ... */ };
class EmailValidator : public BaseValidator { /* ... */ };
class PasswordValidator : public BaseValidator { /* ... */ };
class User : public EmailValidator, public PasswordValidator { /* ... */ };
// Multiple inheritance, brittle hierarchy
```

**Why Composition:**

- More flexible
- Avoids inheritance hierarchies
- Avoids diamond problem
- Easier to understand and modify
- Better supports SOLID principles

### 4.8 Program to Interfaces

Always depend on interfaces, never on concrete implementations.

**Good:**

```cpp
class Application {
public:
    Application(std::shared_ptr<Configuration> config,
                std::shared_ptr<Logger> logger)
        : config_(config), logger_(logger) {}
};
```

**Bad:**

```cpp
class Application {
public:
    Application() : config_(std::make_unique<FileConfiguration>()),
                    logger_(std::make_unique<SpdlogLogger>()) {}
};
```

### 4.9 Immutable Configuration

Configuration is loaded once at startup and never modified.

**Benefits:**

- No race conditions
- Easier to reason about
- Safe to share across threads
- Prevents runtime configuration errors

### 4.10 Fail Fast

Detect problems early and terminate rather than continuing with corrupted state.

```cpp
// Good: Fail fast at construction
class Application {
public:
    explicit Application(ConfigPtr config, LoggerPtr logger) {
        if (!config) throw std::invalid_argument("Config required");
        if (!logger) throw std::invalid_argument("Logger required");
        if (config->getInt("port") < 0) {
            throw std::invalid_argument("Invalid port in config");
        }
        // All preconditions met, safe to proceed
    }
};
```

### 4.11 Defensive Programming

Assume inputs are wrong and verify everything.

```cpp
// Good: Defensive
bool UserService::update(const User& user) {
    if (user.id <= 0) return false;  // Invalid ID
    if (user.name.empty()) return false;  // Empty name
    if (user.email.empty()) return false;  // Empty email
    // Proceed with validation-passed user
}
```

---

## 5. Design Patterns

Cortex uses established design patterns to solve recurring problems. This is not novel architecture—it's proven patterns applied correctly.

| Pattern | Purpose | Where in Cortex | Rationale |
|---------|---------|-----------------|-----------|
| **Repository** | Abstract data access layer | Services use Repository interface for persistence | Decouples business logic from database implementation; enables testing with in-memory repos |
| **Factory** | Create complex objects | ApplicationFactory, LoggerFactory, RepositoryFactory | Centralizes object creation; simplifies dependency injection; one place to add logging/validation |
| **Strategy** | Encapsulate interchangeable algorithms | Configuration sources (JSON, env vars, YAML) | Allows runtime selection of implementation; satisfies OCP |
| **Builder** | Construct complex objects step-by-step | Query builders, request builders | Improves readability; validates state at construction; separates construction from representation |
| **Adapter** | Convert interface to expected type | SpdlogLogger adapts spdlog to Logger interface | Decouples from third-party libraries; allows swapping implementations |
| **Facade** | Provide simplified interface to complex subsystem | Application orchestrates multiple subsystems | Hides complexity; keeps main() small; single entry point |
| **Dependency Injection** | Provide object dependencies externally | All services receive dependencies via constructor | Enables testing; decouples components; makes dependencies visible |
| **Observer** | Notify multiple objects of state changes | Logging subscribers, event handlers | Loosely couples components; enables extensibility |
| **Command** | Encapsulate requests as objects | API request handlers, async task queues | Enables queuing, logging, undo, retry semantics |
| **Decorator** | Add behavior to objects dynamically | Caching layer around repositories | Extends behavior without modifying original; satisfies OCP |
| **Template Method** | Define algorithm skeleton in base class | Base controller template, base service template | Enforces consistency; reduces duplication |

### 5.1 Pattern Application Rules

1. **Use patterns, don't force them.** If a simpler solution exists, use it.
2. **Document why you chose a pattern.** Future engineers should understand the reasoning.
3. **Avoid pattern fever.** Not every class needs a pattern.
4. **Combine patterns thoughtfully.** Strategy + Factory + DI is common; don't combine unnecessarily.

---

## 6. Project Structure Standards

Every folder and file has a clear purpose and defined responsibilities.

```
cortex-backend/
├── CMakeLists.txt              Root CMake configuration
├── docker-compose.yml          Local development environment
│
├── include/                    PUBLIC headers (only if meant for outside use)
│   ├── app/
│   ├── config/
│   ├── logging/
│   └── utils/
│
├── src/                        PRIVATE implementation
│   ├── main.cpp               Application entry point (< 40 lines)
│   ├── app/                   Application orchestrator
│   │   ├── Application.h
│   │   ├── Application.cpp
│   │   ├── ApplicationFactory.h
│   │   └── ApplicationFactory.cpp
│   ├── config/                Configuration management
│   │   ├── Configuration.h    (interface)
│   │   ├── FileConfiguration.h
│   │   └── FileConfiguration.cpp
│   ├── logging/               Logging system
│   │   ├── Logger.h          (interface)
│   │   ├── SpdlogLogger.h
│   │   └── SpdlogLogger.cpp
│   ├── http/                 (future) HTTP endpoints
│   │   ├── controllers/      Request handlers
│   │   └── middleware/       HTTP middleware
│   ├── services/             (future) Business logic
│   │   ├── UserService.h
│   │   └── UserService.cpp
│   ├── repositories/         (future) Data access
│   │   ├── Repository.h      (interface)
│   │   ├── UserRepository.h
│   │   └── UserRepository.cpp
│   ├── models/               (future) Domain entities
│   │   ├── User.h
│   │   └── User.cpp
│   ├── database/             (future) Database connections
│   ├── utils/                Utilities
│   │   ├── Result.h         Result/Optional types
│   │   ├── Mapper.h         Object mappers
│   │   └── Validators.h     Input validators
│   └── exception/            Custom exceptions
│
├── tests/                     Unit and integration tests
│   ├── unit/
│   ├── integration/
│   └── CMakeLists.txt
│
├── config/                    Configuration files
│   ├── config.json           Main configuration
│   ├── config.dev.json       Development overrides
│   ├── config.prod.json      Production overrides
│   └── database/             Database migrations
│
├── docs/                      Documentation
│   ├── ENGINEERING_PRINCIPLES.md  (this file)
│   ├── ARCHITECTURE.md
│   ├── API.md
│   ├── SETUP.md
│   └── DECISIONS.md
│
├── scripts/                   Build and utility scripts
│   ├── build.sh
│   ├── clean.sh
│   └── test.sh
│
├── docker/                    Docker configurations
│   ├── Dockerfile.backend
│   └── Dockerfile.postgres
│
└── logs/                      (runtime) Log files
    └── cortex.log
```

### 6.1 Folder Responsibilities

| Folder | Responsibility | Dependencies Allowed |
|--------|-----------------|----------------------|
| **include/** | Public headers for external use | (Usually empty in closed systems) |
| **src/app/** | Application startup and orchestration | Everything (all layers) |
| **src/config/** | Configuration reading and access | Logging only |
| **src/logging/** | Structured logging system | Nothing (no dependencies) |
| **src/http/controllers/** | HTTP request routing and responses | Services, Utils, Logging |
| **src/services/** | Business logic and orchestration | Repositories, Utils, Logging |
| **src/repositories/** | Data persistence abstraction | Database, Models, Utils, Logging |
| **src/models/** | Domain entities and business objects | Utils only |
| **src/database/** | Database connections and drivers | Logging only |
| **src/utils/** | Utilities, helpers, validators | Nothing (no dependencies) |
| **src/exception/** | Custom exception types | Nothing |
| **tests/** | Unit and integration tests | All (can mock anything) |
| **config/** | Configuration and secrets | Nothing (static files) |
| **docs/** | Documentation and decisions | Nothing (reference materials) |

### 6.2 Dependency Flow Diagram

```
        main.cpp
            ↓
    ┌───────────────────────┐
    │   Application         │ (Facade, Orchestrator)
    │   (Startup & Config)  │
    └───────────────────────┘
            ↓
    ┌───────────────────────┐
    │  Controllers          │ (HTTP Routing)
    │  (HTTP Layer)         │
    └───────────────────────┘
            ↓
    ┌───────────────────────┐
    │  Services             │ (Business Logic)
    │  (Use Cases)          │
    └───────────────────────┘
            ↓
    ┌───────────────────────┐
    │  Repositories         │ (Data Abstraction)
    │  (Interfaces)         │
    └───────────────────────┘
            ↓
    ┌───────────────────────┐
    │  Database Driver      │ (Persistence)
    │  (SQL, Adapters)      │
    └───────────────────────┘

RULE: Arrows point DOWNWARD only.
      Upper layers know lower layers.
      Lower layers know NOTHING about upper layers.
```

### 6.3 Cross-Cutting Concerns

Some components are accessed throughout:

```
        Configuration
            (Immutable)
                ↓
    ┌──────────────────────────┐
    │ All Layers               │
    │ (Global access, read-only)
    └──────────────────────────┘

        Logger
       (Injected)
            ↓
    ┌──────────────────────────┐
    │ All Layers               │
    │ (Via dependency injection)
    └──────────────────────────┘
```

---

## 7. Coding Standards

### 7.1 Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| **Classes** | PascalCase | `UserService`, `FileConfiguration` |
| **Functions** | camelCase | `getUserById()`, `validateEmail()` |
| **Variables** | snake_case | `user_count`, `is_valid` |
| **Constants** | UPPER_SNAKE_CASE | `MAX_RETRIES`, `DEFAULT_PORT` |
| **Member variables** | snake_case with trailing `_` | `user_`, `config_` |
| **Enums** | PascalCase values | `enum class LogLevel { Info, Error }` |
| **File names** | match class name | `UserService.h`, `UserService.cpp` |
| **Namespaces** | snake_case, relate to module | `namespace cortex::services { }` |

**Rationale:**

- PascalCase for types (classes, structs)—visually distinct from variables
- camelCase for functions—emphasizes behavior
- snake_case for variables—matches standard C++ (std::string)
- Trailing `_` for members—immediately visible that it's a member
- UPPER_SNAKE_CASE for constants—immediately obvious they're immutable

### 7.2 Namespaces

**Use namespaces to prevent naming collisions and organize code logically:**

```cpp
namespace cortex {
    namespace app {
        class Application { };
    }
    namespace config {
        class Configuration { };
    }
    namespace services {
        class UserService { };
    }
}
```

**Using declarations at file scope (not in headers):**

```cpp
// In .cpp file only
using cortex::app::Application;
using cortex::services::UserService;
```

**Never in headers:**

```cpp
// NEVER in header files
using namespace cortex;  // BAD: pollutes namespace
using cortex::*;         // NOT VALID: can't use wildcards
```

### 7.3 Header Organization

**Order of includes in .h files:**

```cpp
#pragma once  // or #ifndef guard

// 1. Standard library headers
#include <string>
#include <vector>
#include <memory>
#include <optional>

// 2. Third-party headers
#include <spdlog/spdlog.h>

// 3. Local project headers
#include "config/Configuration.h"
#include "logging/Logger.h"

namespace cortex {
    class MyClass { /* ... */ };
}
```

**Header guards (prefer `#pragma once`):**

```cpp
#pragma once  // Modern, cleaner, compiler-optimized
```

**Order of class members:**

```cpp
class MyClass {
public:
    // Public methods
    MyClass(Dependencies) noexcept;
    ~MyClass();
    
    // Delete copy operations if not needed
    MyClass(const MyClass&) = delete;
    MyClass& operator=(const MyClass&) = delete;
    
    // Allow move
    MyClass(MyClass&&) = default;
    MyClass& operator=(MyClass&&) = default;
    
    // Public interface
    void doSomething() noexcept;
    std::optional<int> getData() const noexcept;

private:
    // Private implementation
    void internalHelper();
    
    // Member variables
    std::shared_ptr<Dependency> dependency_;
    std::string state_;
};
```

### 7.4 Include Ordering

Always include in this order:

1. Self header (.h file)
2. Standard library
3. Third-party libraries
4. Project headers

```cpp
// MyClass.cpp
#include "MyClass.h"              // 1. Self

#include <vector>                 // 2. Standard library
#include <memory>

#include <spdlog/spdlog.h>        // 3. Third-party

#include "config/Configuration.h" // 4. Project
#include "logging/Logger.h"
```

**Rationale:** Catches missing includes in headers; prevents include-order dependencies.

### 7.5 Const Correctness

**Mark everything const that doesn't modify state:**

```cpp
class User {
public:
    // Mark methods const if they don't modify state
    std::string getName() const { return name_; }
    int getId() const { return id_; }
    
    // Mark parameters const if not modified
    void update(const User& other) {
        name_ = other.name_;
        // ...
    }
    
    // Mark return values const if returning internal state
    const std::string& getEmailUnsafe() const {
        return email_;  // Caller can't modify returned reference
    }

private:
    std::string name_;
    std::string email_;
    int id_;
};
```

**Why:**

- Documents intent: "this doesn't modify state"
- Compiler catches mistakes at compile time
- Enables safe shared access
- Helps multithreading safety

### 7.6 constexpr

Use `constexpr` for compile-time constants:

```cpp
// Evaluated at compile time
constexpr int MAX_USERS = 1000;
constexpr std::string_view APP_NAME = "Cortex";

// Evaluated at compile time when possible
constexpr int factorial(int n) {
    return n <= 1 ? 1 : n * factorial(n - 1);
}

// Usage
std::array<User, MAX_USERS> users;
```

### 7.7 enum class

Always use `enum class`, never plain `enum`:

```cpp
// Good: Scoped, prevents implicit conversion
enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Critical
};

// Usage
if (level == LogLevel::Error) { /* ... */ }

// Bad: Plain enum
enum LogLevel {
    TRACE,
    DEBUG,
    INFO
};
// Pollutes namespace, implicit conversions
```

### 7.8 RAII (Resource Acquisition Is Initialization)

**Every resource must be acquired in constructor and released in destructor:**

```cpp
// Good: RAII
class DatabaseConnection {
public:
    explicit DatabaseConnection(const std::string& url) {
        connection_ = PQconnectdb(url.c_str());  // Acquire
        if (!connection_) throw std::runtime_error("Connection failed");
    }
    
    ~DatabaseConnection() {
        if (connection_) PQfinish(connection_);  // Release
    }
    
private:
    PGconn* connection_;
};

// Usage - exception-safe
{
    DatabaseConnection db("postgresql://localhost/cortex");
    db.execute("SELECT * FROM users");
}  // Destructor called, connection cleaned up even if exception

// Bad: Manual resource management
class BadConnection {
public:
    PGconn* connect(const std::string& url) {
        return PQconnectdb(url.c_str());  // Caller must remember to close
    }
};
```

### 7.9 Move Semantics

**Support move when object is expensive to copy:**

```cpp
class LargeBuffer {
public:
    // Copy: explicit, indicates expensive operation
    LargeBuffer(const LargeBuffer& other) { /* deep copy */ }
    LargeBuffer& operator=(const LargeBuffer& other) { /* deep copy */ }
    
    // Move: transfer ownership
    LargeBuffer(LargeBuffer&& other) noexcept : data_(std::move(other.data_)) {}
    LargeBuffer& operator=(LargeBuffer&& other) noexcept {
        data_ = std::move(other.data_);
        return *this;
    }
    
private:
    std::vector<uint8_t> data_;
};

// Usage
LargeBuffer buf1 = /* ... */;
LargeBuffer buf2 = std::move(buf1);  // Move, not copy
```

### 7.10 Rule of Zero

**Prefer not implementing constructors/destructors/assignment:**

```cpp
// Good: Rule of Zero
class User {
public:
    // No explicit constructor, destructor, or assignment
    // Compiler generates optimal implementations
    
private:
    std::string name_;              // std::string handles destruction
    std::vector<std::string> tags_; // std::vector handles destruction
    std::shared_ptr<Profile> profile_;  // shared_ptr handles destruction
};
```

**Use Rule of Zero unless:**

- You need custom destruction (rare with modern C++)
- You need to customize copy/move behavior (document why)
- You're implementing RAII for external resources

### 7.11 Rule of Five

**If you implement any special member function, implement all five:**

```cpp
// If you override destructor...
class CustomBuffer {
public:
    // Destructor
    ~CustomBuffer() { delete[] buffer_; }
    
    // Copy constructor
    CustomBuffer(const CustomBuffer& other) : size_(other.size_) {
        buffer_ = new char[size_];
        std::copy(other.buffer_, other.buffer_ + size_, buffer_);
    }
    
    // Copy assignment
    CustomBuffer& operator=(const CustomBuffer& other) {
        if (this == &other) return *this;
        delete[] buffer_;
        size_ = other.size_;
        buffer_ = new char[size_];
        std::copy(other.buffer_, other.buffer_ + size_, buffer_);
        return *this;
    }
    
    // Move constructor
    CustomBuffer(CustomBuffer&& other) noexcept
        : buffer_(other.buffer_), size_(other.size_) {
        other.buffer_ = nullptr;
        other.size_ = 0;
    }
    
    // Move assignment
    CustomBuffer& operator=(CustomBuffer&& other) noexcept {
        delete[] buffer_;
        buffer_ = other.buffer_;
        size_ = other.size_;
        other.buffer_ = nullptr;
        other.size_ = 0;
        return *this;
    }
    
private:
    char* buffer_ = nullptr;
    size_t size_ = 0;
};
```

**But honestly, use `std::vector` or smart pointers instead.**

### 7.12 Smart Pointers

**Usage rules:**

```cpp
// Use std::unique_ptr for exclusive ownership
std::unique_ptr<Service> service = std::make_unique<UserService>();

// Use std::shared_ptr only when ownership is truly shared
std::shared_ptr<Logger> logger = std::make_shared<SpdlogLogger>("App");

// Move unique_ptr to transfer ownership
class Application {
    std::unique_ptr<Service> service_;
public:
    void setService(std::unique_ptr<Service> svc) {
        service_ = std::move(svc);  // Transfer ownership
    }
};

// Pass by reference when not transferring ownership
void processUser(const User& user) { /* ... */ }
void modify(Service& service) { /* ... */ }

// Avoid raw pointers (use unique_ptr or references)
class Good {
    std::unique_ptr<Resource> resource_;  // or reference
};
class Bad {
    Resource* resource_;  // When should you delete this?
};
```

### 7.13 Exception Safety

**Provide strong exception guarantee:**

```cpp
class UserRepository {
public:
    // Strong guarantee: all-or-nothing
    bool add(const User& user) noexcept {
        try {
            // Prepare (might throw)
            std::string query = buildInsertQuery(user);
            
            // Execute (might throw)
            bool result = connection_->execute(query);
            
            // If we get here, no exception occurred
            return result;
        } catch (...) {
            // On any exception, state unchanged (strong guarantee)
            logger_->error("Failed to add user");
            return false;
        }
    }
};
```

**Levels of exception safety:**

1. **No guarantee:** Exception might corrupt state
2. **Basic guarantee:** Exception won't corrupt state, but may leave it changed
3. **Strong guarantee:** Exception has no effect (transaction-like)
4. **No-throw guarantee:** Never throws (mark with `noexcept`)

Cortex targets **strong guarantee** by default.

### 7.14 Error Handling

**Prefer returning `std::optional` or `std::variant` over exceptions for expected errors:**

```cpp
// Good: Expected error - use optional
std::optional<User> UserRepository::findById(int id) const {
    auto result = database_.query("SELECT * FROM users WHERE id = ?", id);
    if (!result) return std::nullopt;
    return parseUser(result);
}

// Usage
if (auto user = repository.findById(123)) {
    process(*user);
} else {
    // User not found - expected case
}

// Good: Multiple possible results - use variant
enum class ValidationError {
    InvalidEmail,
    PasswordTooShort,
    NameEmpty
};

std::variant<User, ValidationError> validateUser(const UserInput& input) {
    if (input.email.empty()) return ValidationError::InvalidEmail;
    if (input.password.length() < 8) return ValidationError::PasswordTooShort;
    return User(input);
}

// Usage
auto result = validateUser(input);
if (std::holds_alternative<User>(result)) {
    User user = std::get<User>(result);
} else {
    ValidationError error = std::get<ValidationError>(result);
    log_error(error);
}

// Exceptions: Only for truly exceptional conditions
class FileConfiguration : public Configuration {
public:
    explicit FileConfiguration(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            // This is exceptional - config file should always exist
            throw std::runtime_error("Cannot open config file: " + path);
        }
        // ... parse
    }
};
```

### 7.15 Logging

**Structured logging guidelines:**

```cpp
#include "logging/Logger.h"

void ProcessRequest(const HttpRequest& request) {
    logger_->info("Processing request: {} {}", request.method, request.path);
    
    try {
        // Process request
        auto user = userService_->findUser(userId);
        if (!user) {
            logger_->warn("User not found: {}", userId);
            return;
        }
        
        logger_->info("User found: name={}, email={}", user->name, user->email);
    } catch (const std::exception& e) {
        logger_->error("Request failed: {}", e.what());
        throw;
    }
}
```

**Logging levels:**

| Level | When to Use | Example |
|-------|------------|---------|
| **TRACE** | Very detailed debugging | Variable values, loop iterations |
| **DEBUG** | Detailed info for developers | Function entry/exit, decisions |
| **INFO** | Important state changes | Server started, user logged in |
| **WARN** | Unexpected but recoverable | Retry attempt, missing optional config |
| **ERROR** | Recoverable errors | User not found, validation failed |
| **CRITICAL** | Unrecoverable errors | Database unavailable, config missing |

### 7.16 Comments

**Code should be self-documenting. Comments explain *why*, not *what*:**

```cpp
// Good: Explains intent and reasoning
class RequestThrottler {
    static constexpr int REQUESTS_PER_MINUTE = 60;
    
    // Use exponential backoff for retries to prevent overwhelming
    // a temporarily overloaded service
    std::chrono::milliseconds calculateBackoff(int attempt) {
        return std::chrono::milliseconds(100 * (1 << attempt));
    }
};

// Bad: Comments just restate code
int rate = 60;  // Set rate to 60
if (request.rate > rate) {  // If request rate greater than rate
    throw std::exception();  // Throw exception
}
```

**Documentation comments (before public methods):**

```cpp
/**
 * Find user by ID
 * 
 * @param id User ID to search for
 * @return User if found, nullopt otherwise
 * @throws DatabaseException if connection fails
 */
std::optional<User> findById(int id);
```

### 7.17 Formatting

**Use clang-format. Example .clang-format:**

```yaml
BasedOnStyle: LLVM
ColumnLimit: 100
IndentWidth: 4
UseTab: Never
BreakBeforeBraces: Linux
AlignAfterOpenBracket: Align
```

**Manual formatting rules:**

- 4 spaces per indentation level
- Max 100 characters per line
- One statement per line
- Opening brace on same line (Linux style)

```cpp
void Process(const Request& request) {
    if (request.isValid()) {
        handleValidRequest(request);
    } else {
        logger_->warn("Invalid request received");
    }
}
```

### 7.18 Magic Numbers

**No magic numbers. Use named constants:**

```cpp
// Bad
if (age > 18 && age < 65) { /* ... */ }
if (retries > 3) { /* ... */ }
if (buffer.size() > 1024 * 1024) { /* ... */ }

// Good
constexpr int ADULT_AGE = 18;
constexpr int RETIREMENT_AGE = 65;
constexpr int MAX_RETRIES = 3;
constexpr size_t MAX_BUFFER_SIZE = 1024 * 1024;  // 1MB

if (age > ADULT_AGE && age < RETIREMENT_AGE) { /* ... */ }
if (retries > MAX_RETRIES) { /* ... */ }
if (buffer.size() > MAX_BUFFER_SIZE) { /* ... */ }
```

### 7.19 Modern C++20 Features

**Embrace modern C++ (C++20):**

```cpp
// Concepts: Type constraints
template<typename T>
concept Loggable = requires(T t) {
    { t.toString() } -> std::convertible_to<std::string>;
};

template<Loggable T>
void logValue(const T& value) {
    logger_->info("Value: {}", value.toString());
}

// Ranges
std::vector<User> users = getUsers();
std::ranges::sort(users, {}, &User::id);

// Structured bindings
auto [username, password] = parseCredentials(request);

// std::optional
std::optional<User> user = findById(123);
if (user) { /* ... */ }

// Spaceship operator
if ((a <=> b) < 0) { /* a < b */ }
```

### 7.20 When to Use `auto`

**Good uses of `auto`:**

```cpp
// Obviously verbose type
auto config = std::make_shared<FileConfiguration>("config.json");

// Complex template types
auto result = std::find_if(users.begin(), users.end(),
    [](const User& u) { return u.id == target_id; });

// Deduced from obvious initialization
auto count = 0;  // auto int
auto name = "Cortex"s;  // auto std::string
```

**Bad uses of `auto`:**

```cpp
// Type isn't obvious - BAD
auto processData = getData();  // What type is processData?

// Complex expression - BAD
auto x = a + b * c - d / e;  // What's the result type?
```

**Rule:** If the type isn't obvious from the right side of `=`, write the type explicitly.

---

## 8. CMake Standards

### 8.1 Targets

Always work with targets, never file lists:

```cmake
# Good
add_library(cortex_config STATIC
    src/config/FileConfiguration.cpp
)
target_include_directories(cortex_config PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(cortex_config PRIVATE jsoncpp)

# Bad
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_library(cortex ${SOURCES})
```

### 8.2 Libraries

Create logical library targets:

```cmake
add_library(cortex_config STATIC)
add_library(cortex_logging STATIC)
add_library(cortex_app STATIC)

# Link dependencies
target_link_libraries(cortex_app PRIVATE cortex_config cortex_logging)

# Main executable links to libraries
add_executable(cortex src/main.cpp)
target_link_libraries(cortex PRIVATE cortex_app)
```

### 8.3 Include Directories

Specify visibility:

```cmake
# PUBLIC: Used by this target and downstream targets
target_include_directories(cortex_config PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# PRIVATE: Used only by this target
target_include_directories(cortex_config PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# INTERFACE: Only used by downstream targets, not this one
target_include_directories(cortex_config INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/public
)
```

### 8.4 Compile Options

Set per-target, not global:

```cmake
# Warnings for all targets
target_compile_options(cortex_app PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion
)

# Debug symbols
target_compile_options(cortex_app PRIVATE
    $<$<CONFIG:Debug>:-g -O0>
)

# Optimization for Release
target_compile_options(cortex_app PRIVATE
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)
```

### 8.5 Release vs Debug

Always provide both build types:

```cmake
# Use cmake -DCMAKE_BUILD_TYPE=Debug or Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL Debug)
    message(STATUS "Debug build")
    add_compile_options(-g -O0)
elseif(CMAKE_BUILD_TYPE STREQUAL Release)
    message(STATUS "Release build")
    add_compile_options(-O3 -DNDEBUG)
endif()
```

### 8.6 Sanitizers

Enable for debugging:

```cmake
# Address Sanitizer
if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

# Thread Sanitizer
if(ENABLE_TSAN)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()

# Usage: cmake -DENABLE_ASAN=ON
```

---

## 9. Error Handling Strategy

### 9.1 Recoverable Errors

Expected errors that the application can handle:

```cpp
// Expected: User not found
std::optional<User> UserRepository::findById(int id) {
    // Return nullopt, not exception
    auto result = database_.query("SELECT * FROM users WHERE id = ?", id);
    return result ? parseUser(result) : std::nullopt;
}

// Usage
if (auto user = repository_.findById(123)) {
    process(*user);
} else {
    logger_->info("User not found, returning 404");
    response.setStatus(404);
}
```

### 9.2 Fatal Errors

Unrecoverable errors that terminate the application:

```cpp
// Fatal: No database connection
class Application {
    Application(ConfigPtr config) {
        db_ = std::make_unique<Database>(config->getString("db_url").value());
        // If database fails to connect, this throws
        // Application can't continue without database
        if (!db_->isConnected()) {
            throw std::runtime_error("Database connection failed");
        }
    }
};
```

### 9.3 Exceptions

When to use exceptions:

```cpp
// Use exceptions for:
// 1. Precondition violations (should never happen)
class User {
public:
    explicit User(const std::string& email) {
        if (email.empty()) {
            throw std::invalid_argument("Email cannot be empty");
        }
        email_ = email;
    }
};

// 2. Invariant violations
class Stack {
public:
    int pop() {
        if (items_.empty()) {
            throw std::logic_error("Pop from empty stack");
        }
        return items_.back();
    }
};

// 3. Resource acquisition failures
class FileLogger {
public:
    explicit FileLogger(const std::string& path) {
        file_.open(path);
        if (!file_.is_open()) {
            throw std::runtime_error("Cannot open log file: " + path);
        }
    }
};
```

### 9.4 Expected Failures

Normal failures that are expected:

```cpp
// Use optional/variant for expected failures
enum class ValidationError {
    InvalidEmail,
    PasswordTooShort
};

std::variant<User, ValidationError> parseUser(const std::string& data) {
    // This might fail, but it's expected
    if (!isValidEmail(data)) {
        return ValidationError::InvalidEmail;
    }
    return User(data);
}

// Usage
auto result = parseUser(input);
if (std::holds_alternative<User>(result)) {
    process(std::get<User>(result));
} else {
    ValidationError error = std::get<ValidationError>(result);
    // Expected: respond with validation error
    response.setStatus(400);
    response.setBody(formatValidationError(error));
}
```

### 9.5 Logging

Log errors at appropriate levels:

```cpp
try {
    // Attempt to process
    auto user = userService_.findById(id);
    if (!user) {
        logger_->debug("User {} not found", id);  // Expected
        return;
    }
    
    // Process
    bool success = repository_.update(user);
    if (!success) {
        logger_->warn("Failed to update user {}", id);  // Unexpected but recoverable
        throw std::runtime_error("Database update failed");
    }
    
    logger_->info("User {} updated successfully", id);  // Success
    
} catch (const std::exception& e) {
    logger_->error("Error processing user {}: {}", id, e.what());  // Error
    throw;
}
```

### 9.6 Propagation

When to propagate vs. handle:

```cpp
// Controller layer: Catch all exceptions
class UserController {
    HttpResponse create(const HttpRequest& request) {
        try {
            auto user = service_.create(request.body);
            logger_->info("User created: {}", user.id);
            return HttpResponse::created(user.toJson());
        } catch (const std::exception& e) {
            logger_->error("Failed to create user: {}", e.what());
            return HttpResponse::internalError("Failed to create user");
        }
    }
};

// Service layer: Convert exceptions to return types
class UserService {
    std::optional<User> create(const UserInput& input) noexcept {
        try {
            // Validate
            if (input.email.empty()) return std::nullopt;
            
            // Persist
            return repository_.save(User(input));
        } catch (...) {
            logger_->error("Service error in create");
            return std::nullopt;
        }
    }
};
```

---

## 10. Logging Standards

### 10.1 Log Levels

Use log levels correctly:

| Level | When | What to Log |
|-------|------|------------|
| **TRACE** | Dev only | Loop iterations, variable values, fine-grained flow |
| **DEBUG** | Dev/QA | Function entry/exit, decisions, state changes |
| **INFO** | Prod OK | Startup/shutdown, important business events |
| **WARN** | Investigate | Recoverable errors, degraded conditions |
| **ERROR** | Urgent | Errors that affect operations |
| **CRITICAL** | Emergency | System failures, requires immediate action |

### 10.2 Formatting

**Pattern:** `[timestamp] [logger_name] [level] message`

```cpp
logger_->info("User {} logged in from {}", user.id, ip_address);
// Output: [2026-07-28 14:42:56.289] [Cortex] [info] User 123 logged in from 192.168.1.1
```

### 10.3 Structured Logging

Include context in messages:

```cpp
// Good: Includes relevant context
logger_->info("Request received: method={}, path={}, user_id={}", 
    request.method, request.path, user.id);

// Bad: Vague message
logger_->info("Processing request");

// Good: Include error context
logger_->error("Database query failed: query={}, error={}, retry_attempt={}", 
    query, error.message(), retry_count);
```

### 10.4 Sensitive Data

Never log sensitive information:

```cpp
// Bad: Logging passwords
logger_->info("User login: email={}, password={}", email, password);

// Good: Only log non-sensitive data
logger_->info("User login: email={}, ip={}", email, ip_address);

// Bad: Logging credit cards
logger_->info("Payment: card={}, amount={}", card_number, amount);

// Good
logger_->info("Payment: card_last_4={}, amount={}", card_last_4_digits, amount);
```

### 10.5 Correlation IDs

Add correlation IDs for request tracing:

```cpp
class HttpContext {
public:
    const std::string& getCorrelationId() const {
        if (!correlation_id_) {
            correlation_id_ = generateUUID();
        }
        return correlation_id_;
    }
    
private:
    mutable std::string correlation_id_;
};

// Usage in service
void ProcessRequest(const HttpRequest& request, const HttpContext& ctx) {
    logger_->info("[{}] Processing request: {}", ctx.getCorrelationId(), request.path);
    
    auto user = userService_.findById(userId);
    logger_->info("[{}] User found: {}", ctx.getCorrelationId(), user.id);
}

// All logs related to this request have same correlation_id
// Useful for tracing through distributed system
```

---

## 11. Testing Philosophy

### 11.1 Unit Testing

Test individual components in isolation:

```cpp
// In tests/unit/UserServiceTest.cpp
class UserServiceTest : public ::testing::Test {
protected:
    MockUserRepository repository_;
    std::shared_ptr<Logger> logger_;
    UserService service_{std::make_shared<MockUserRepository>(), logger_};
};

TEST_F(UserServiceTest, FindUserById_ReturnsUserWhenFound) {
    // Arrange
    User expected(123, "John", "john@example.com");
    EXPECT_CALL(repository_, findById(123)).WillOnce(Return(expected));
    
    // Act
    auto result = service_.findById(123);
    
    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, 123);
    EXPECT_EQ(result->name, "John");
}

TEST_F(UserServiceTest, FindUserById_ReturnsNulloptWhenNotFound) {
    // Arrange
    EXPECT_CALL(repository_, findById(999)).WillOnce(Return(std::nullopt));
    
    // Act
    auto result = service_.findById(999);
    
    // Assert
    EXPECT_FALSE(result.has_value());
}
```

### 11.2 Integration Testing

Test components working together:

```cpp
// In tests/integration/DatabaseIntegrationTest.cpp
class DatabaseIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use test database
        db_ = std::make_unique<Database>("postgresql://localhost/cortex_test");
        repository_ = std::make_unique<UserRepository>(db_.get());
    }
};

TEST_F(DatabaseIntegrationTest, SaveAndRetrieveUser) {
    // Arrange
    User user(0, "Alice", "alice@example.com");
    
    // Act
    auto saved = repository_->save(user);
    auto retrieved = repository_->findById(saved.id);
    
    // Assert
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, "Alice");
    EXPECT_EQ(retrieved->email, "alice@example.com");
}
```

### 11.3 Mocking

Mock external dependencies:

```cpp
// tests/mocks/MockUserRepository.h
class MockUserRepository : public UserRepository {
public:
    MOCK_METHOD(std::optional<User>, findById, (int id), (override));
    MOCK_METHOD(bool, save, (const User& user), (override));
    MOCK_METHOD(bool, update, (const User& user), (override));
};

// Usage
MockUserRepository mock;
EXPECT_CALL(mock, findById(123)).WillOnce(Return(User(...)));
```

### 11.4 Dependency Injection for Testing

DI enables easy testing:

```cpp
// Production code
class UserService {
public:
    UserService(std::shared_ptr<UserRepository> repo,
                std::shared_ptr<Logger> logger)
        : repository_(repo), logger_(logger) {}
        
private:
    std::shared_ptr<UserRepository> repository_;
    std::shared_ptr<Logger> logger_;
};

// Test code: Inject mocks
TEST(UserServiceTest, ...) {
    auto mock_repo = std::make_shared<MockUserRepository>();
    auto mock_logger = std::make_shared<MockLogger>();
    
    UserService service(mock_repo, mock_logger);
    // Now we can control behavior of dependencies
}
```

### 11.5 Coverage Goals

- **Overall coverage:** Aim for 80%+
- **Critical paths:** 100% coverage
- **Utils:** 95%+ coverage
- **Infrastructure:** Less critical (logging, config)

---

## 12. API Standards

### 12.1 REST Principles

Follow RESTful conventions:

```
GET    /api/v1/users          - List all users
GET    /api/v1/users/:id      - Get specific user
POST   /api/v1/users          - Create new user
PUT    /api/v1/users/:id      - Update user
DELETE /api/v1/users/:id      - Delete user
```

### 12.2 Response Format

Consistent response structure:

```json
{
    "success": true,
    "data": {
        "id": 123,
        "name": "John Doe",
        "email": "john@example.com"
    },
    "meta": {
        "timestamp": "2026-07-28T14:42:56Z",
        "requestId": "550e8400-e29b-41d4-a716-446655440000"
    }
}
```

### 12.3 Error Responses

```json
{
    "success": false,
    "error": {
        "code": "VALIDATION_ERROR",
        "message": "Invalid input",
        "details": [
            {
                "field": "email",
                "message": "Invalid email format"
            },
            {
                "field": "password",
                "message": "Password too short"
            }
        ]
    },
    "meta": {
        "timestamp": "2026-07-28T14:42:56Z",
        "requestId": "550e8400-e29b-41d4-a716-446655440000"
    }
}
```

### 12.4 HTTP Status Codes

| Code | Use | Example |
|------|-----|---------|
| **200** | Success | User retrieved |
| **201** | Created | New user created |
| **204** | No content | Deleted successfully |
| **400** | Bad request | Invalid input |
| **401** | Unauthorized | Missing auth |
| **403** | Forbidden | Insufficient permissions |
| **404** | Not found | User doesn't exist |
| **409** | Conflict | Email already exists |
| **500** | Server error | Unhandled exception |
| **503** | Service unavailable | Database down |

### 12.5 API Versioning

```
/api/v1/users   - Version 1 API (stable)
/api/v2/users   - Version 2 API (new features)
```

### 12.6 Request/Response Validation

```cpp
class CreateUserRequest {
public:
    static std::variant<CreateUserRequest, std::string> fromJson(
        const Json::Value& json) {
        
        // Validation
        if (!json.isMember("email")) {
            return "Missing field: email";
        }
        if (json["email"].asString().empty()) {
            return "Field 'email' cannot be empty";
        }
        
        // Construction
        return CreateUserRequest{
            json["name"].asString(),
            json["email"].asString(),
            json["password"].asString()
        };
    }
    
    std::string name;
    std::string email;
    std::string password;
};
```

---

## 13. Database Principles

### 13.1 Repository Isolation

Only repositories access the database:

```cpp
// Good: Service uses repository interface
class UserService {
    UserService(std::shared_ptr<UserRepository> repo) : repo_(repo) {}
    
    std::optional<User> findUser(int id) {
        return repo_->findById(id);  // Use interface, not raw DB
    }
};

// Bad: Service accesses database directly
class BadUserService {
    BadUserService(Database& db) : db_(db) {}
    
    std::optional<User> findUser(int id) {
        auto result = db_.query("SELECT * FROM users WHERE id = ?", id);
        // Direct DB access - hard to test, tight coupling
    }
};
```

### 13.2 Transactions

Use transactions for multi-step operations:

```cpp
bool UserRepository::updateWithAudit(const User& user, const std::string& reason) {
    try {
        transaction_ = db_.beginTransaction();
        
        // Update user
        db_.execute("UPDATE users SET ... WHERE id = ?", user.id);
        
        // Add audit log
        db_.execute("INSERT INTO audit_log ... VALUES (...)", 
                   user.id, reason, getCurrentTime());
        
        transaction_.commit();
        return true;
    } catch (...) {
        transaction_.rollback();
        logger_->error("Transaction failed");
        return false;
    }
}
```

### 13.3 Connection Pooling

Always use connection pools:

```cpp
class Database {
private:
    // Connection pool for efficiency
    std::unique_ptr<ConnectionPool> pool_;
    
public:
    Database(const std::string& url) {
        pool_ = std::make_unique<ConnectionPool>(url, 
            /*min_connections=*/ 5,
            /*max_connections=*/ 20);
    }
};
```

### 13.4 Prepared Statements

Always use prepared statements (prevent SQL injection):

```cpp
// Good: Prepared statement
auto stmt = db_.prepare("SELECT * FROM users WHERE email = ? AND deleted_at IS NULL");
auto result = stmt.execute(email);

// Bad: String concatenation
auto query = "SELECT * FROM users WHERE email = '" + email + "'";  // SQL injection!
auto result = db_.execute(query);
```

### 13.5 Migrations

Version control database schema:

```
migrations/
├── 001_create_users_table.sql
├── 002_add_email_unique_constraint.sql
├── 003_create_audit_log_table.sql
└── 004_add_deleted_at_soft_delete.sql
```

```sql
-- migrations/001_create_users_table.sql
CREATE TABLE IF NOT EXISTS users (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    email VARCHAR(255) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    deleted_at TIMESTAMP
);
```

---

## 14. Security Standards

### 14.1 Secrets Management

Never commit secrets:

```
# .gitignore
.env
.env.local
config/secrets.json
config/*.local.json
```

Use environment variables or secure vaults:

```cpp
class Configuration {
public:
    // Read from environment, not config file
    std::string getDatabasePassword() {
        auto password = std::getenv("DB_PASSWORD");
        if (!password) {
            throw std::runtime_error("DB_PASSWORD not set");
        }
        return password;
    }
};
```

### 14.2 Authentication

Use secure authentication patterns:

```cpp
class AuthService {
public:
    std::optional<AuthToken> login(const std::string& email, 
                                   const std::string& password) {
        // Retrieve password hash (never password)
        auto user = repository_->findByEmail(email);
        if (!user) return std::nullopt;
        
        // Compare password hash
        if (!crypto_.verifyPassword(password, user->password_hash)) {
            return std::nullopt;
        }
        
        // Generate JWT token
        return AuthToken::create(user->id);
    }
};
```

### 14.3 Authorization

Implement proper permission checks:

```cpp
class UserController {
public:
    HttpResponse update(const HttpRequest& req) {
        // Extract user from token
        auto auth_user = extractAuthUser(req);
        int target_id = extractUserId(req.path);
        
        // Check authorization
        if (auth_user.id != target_id && !auth_user.isAdmin()) {
            return HttpResponse::forbidden("Cannot update other users");
        }
        
        // Proceed with update
        return service_.update(target_id, req.body);
    }
};
```

### 14.4 Input Validation

Validate all input:

```cpp
class UserService {
public:
    std::variant<User, ValidationError> create(const CreateUserRequest& req) {
        // Email validation
        if (!EmailValidator::isValid(req.email)) {
            return ValidationError::InvalidEmail;
        }
        
        // Check for existing email
        if (repository_->findByEmail(req.email)) {
            return ValidationError::EmailAlreadyExists;
        }
        
        // Password strength
        if (req.password.length() < 8) {
            return ValidationError::PasswordTooShort;
        }
        
        // All valid, create
        return User::create(req);
    }
};
```

### 14.5 SQL Injection Prevention

Use prepared statements exclusively:

```cpp
// Good: Prepared statement (safe)
auto stmt = db_.prepare("SELECT * FROM users WHERE id = ? AND email = ?");
auto result = stmt.execute(id, email);

// Bad: String concatenation (vulnerable)
auto query = "SELECT * FROM users WHERE id = " + std::to_string(id) +
             " AND email = '" + email + "'";  // SQL injection possible
```

### 14.6 Rate Limiting

Prevent abuse with rate limiting:

```cpp
class RateLimiter {
public:
    bool isAllowed(const std::string& client_ip) {
        auto now = Clock::now();
        auto& requests = request_history_[client_ip];
        
        // Remove old requests (outside 1-minute window)
        auto cutoff = now - std::chrono::minutes(1);
        requests.erase(
            std::remove_if(requests.begin(), requests.end(),
                [cutoff](auto t) { return t < cutoff; }),
            requests.end()
        );
        
        // Check limit (e.g., 60 requests per minute)
        if (requests.size() >= 60) {
            return false;  // Rate limited
        }
        
        requests.push_back(now);
        return true;
    }
};
```

### 14.7 TLS/HTTPS

All production communication must be encrypted:

```cpp
class HttpServer {
public:
    void start() {
        app_.setSSLFiles(
            "certs/server.crt",
            "certs/server.key"
        );
        app_.addListener("0.0.0.0", 443);  // HTTPS port
    }
};
```

---

## 15. Performance Guidelines

### 15.1 Avoid Unnecessary Allocations

```cpp
// Good: No allocation
void processData(std::string_view data) {
    // Work with view, no copy
}

// Good: Reserve space when growing
std::vector<User> users;
users.reserve(1000);  // Avoid reallocations
for (int i = 0; i < 1000; ++i) {
    users.emplace_back(...);
}

// Bad: Frequent allocations
std::vector<User> users;
for (int i = 0; i < 1000; ++i) {
    users.push_back(...);  // May reallocate many times
}
```

### 15.2 Move Semantics

Return objects by value (compiler optimizes):

```cpp
// Good: Return by value, compiler applies NRVO/move
User createUser(const std::string& name) {
    User user(name);
    // ... setup
    return user;  // Moved, not copied
}

// Acceptable: Return optional
std::optional<User> findUser(int id) {
    auto result = query("SELECT ... WHERE id = ?", id);
    if (!result) return std::nullopt;
    return User(result);  // Moved
}
```

### 15.3 Memory Ownership

Make ownership clear:

```cpp
// Clear ownership
class Service {
public:
    // Exclusive ownership - responsibility is clear
    void setRepository(std::unique_ptr<Repository> repo) {
        repository_ = std::move(repo);
    }
    
    // Shared ownership - when needed
    std::shared_ptr<Logger> getLogger() {
        return logger_;  // Multiple owners
    }
    
private:
    std::unique_ptr<Repository> repository_;  // Own it, don't share
    std::shared_ptr<Logger> logger_;           // Shared (many owners)
};
```

### 15.4 Threading

Use async patterns, not raw threads:

```cpp
// Good: Async pattern
auto future = app_.runAsync([]() {
    // Work in thread pool
    processLargeDataset();
});

// Acceptable: Owned thread
class BackgroundTask {
public:
    ~BackgroundTask() {
        shutdown_ = true;
        thread_.join();  // Clean shutdown
    }
    
private:
    std::thread thread_{[this]() { backgroundWork(); }};
    std::atomic<bool> shutdown_{false};
};

// Bad: Unowned threads
void processAsync() {
    std::thread t([]() { /* ... */ });
    t.detach();  // Leaked! Don't do this
}
```

### 15.5 Async Programming

Use async for I/O-bound operations:

```cpp
// Good: Non-blocking I/O
class UserController {
    void getUser(const HttpRequest& req, HttpResponse& res) {
        int user_id = extractUserId(req);
        
        // Non-blocking database query
        repository_->findByIdAsync(user_id)
            .thenValue([&res](const User& user) {
                res.setStatus(200);
                res.setBody(user.toJson());
            })
            .thenError([&res](const std::exception& e) {
                res.setStatus(500);
                res.setBody(formatError(e));
            });
    }
};
```

### 15.6 Caching

Cache expensive computations:

```cpp
class CachedUserRepository : public UserRepository {
public:
    std::optional<User> findById(int id) override {
        // Check cache first
        if (auto cached = cache_->get(id)) {
            return cached;
        }
        
        // Miss: query database
        if (auto user = db_->findById(id)) {
            cache_->put(id, *user, std::chrono::minutes(5));
            return user;
        }
        
        return std::nullopt;
    }
    
private:
    std::unique_ptr<Cache<int, User>> cache_;
};
```

### 15.7 Profiling

Always profile before optimizing:

```bash
# Generate profiling data
perf record ./cortex --workload
perf report

# Or use valgrind
valgrind --tool=callgrind ./cortex --workload
kcachegrind callgrind.out.*
```

---

## 16. Code Review Checklist

Every PR must pass this checklist:

### Architecture
- [ ] Changes follow layered architecture (Controller → Service → Repository)
- [ ] No layer skipping (e.g., Controller directly accessing Repository)
- [ ] Dependencies flow inward only
- [ ] Appropriate use of design patterns

### SOLID Principles
- [ ] Single Responsibility: Each class has one reason to change
- [ ] Open/Closed: Changes don't require modifying existing code
- [ ] Liskov Substitution: Subclasses honor base class contracts
- [ ] Interface Segregation: Interfaces are minimal and focused
- [ ] Dependency Inversion: Depends on interfaces, not implementations

### Naming & Clarity
- [ ] Names are clear, descriptive, not abbreviated
- [ ] Naming conventions followed (PascalCase classes, camelCase functions)
- [ ] No magic numbers
- [ ] Comments explain *why*, not *what*

### Testing
- [ ] Code has unit tests
- [ ] Mocks used appropriately via DI
- [ ] Edge cases covered
- [ ] No skipped tests

### Performance
- [ ] No unnecessary allocations
- [ ] Move semantics used where appropriate
- [ ] Queries are efficient (use indices)
- [ ] No premature optimization

### Security
- [ ] Input validation for all public APIs
- [ ] No hardcoded secrets
- [ ] Prepared statements for SQL
- [ ] Proper authorization checks

### Documentation
- [ ] Public methods have documentation
- [ ] Architectural decisions documented
- [ ] Complex algorithms explained
- [ ] No TODO comments

### Error Handling
- [ ] Expected errors return `std::optional` or `std::variant`
- [ ] Unexpected errors thrown as exceptions
- [ ] Appropriate logging at each level
- [ ] No silent failures

### Logging
- [ ] Appropriate log levels used
- [ ] No sensitive data logged
- [ ] Correlation IDs included for tracing
- [ ] Structured logging format

---

## 17. Pull Request Checklist

**Before submitting a PR:**

- [ ] Code builds locally without warnings
- [ ] All tests pass locally
- [ ] No trailing whitespace or formatting issues
- [ ] Commit messages are clear and concise
- [ ] PR description explains what and why
- [ ] Related issues are linked
- [ ] No merge conflicts
- [ ] Code follows project standards

**PR Description Template:**

```markdown
## What
Brief description of what changed

## Why
Why this change is necessary (business or technical reason)

## How
How the change was implemented

## Testing
How was this tested?

## Breaking Changes
Any API changes or migrations?

## Related Issues
Fixes #123, Related to #456
```

---

## 18. Architecture Decision Records (ADR)

Every significant architectural decision must be documented.

**Format:**

```markdown
# ADR-001: Use Dependency Injection

## Status
ACCEPTED

## Context
We needed to decouple components for testability and flexibility.

## Decision
Use manual dependency injection with factories rather than a DI framework.

## Rationale
- Lightweight and zero runtime overhead
- Full control over construction
- Easy to understand and debug
- Can add framework later if needed

## Consequences
- Factories required for each component type
- More boilerplate than frameworks like Spring
- But better for a small-to-medium sized C++ project
```

**Process:**

1. Create ADR before major architectural changes
2. Discuss with team
3. Record decision in `docs/decisions/ADR-XXX.md`
4. Reference ADR in relevant code

---

## 19. Future Contributors Guide

**How to add a new feature without violating architecture:**

### Adding a New Entity/Domain Model

1. Create entity in `src/models/YourEntity.h`
   - Only business logic, no dependencies
   - Value semantics or minimal state

### Adding a New API Endpoint

1. Create controller in `src/http/controllers/YourController.h`
   - Handle HTTP details only
   - Delegate to service
   
2. Create service in `src/services/YourService.h`
   - Business logic
   - Use repositories
   
3. If needed, create repository in `src/repositories/YourRepository.h`
   - Data access only
   - Implement interface

### Adding a New Configuration Value

1. Add to `config/config.json`
2. Access via `Configuration` interface in service/controller
3. Never hardcode in classes

### Adding a New Dependency

1. Assume it will be injected
2. Add to constructor
3. Update ApplicationFactory to wire it in
4. Tests receive mock via constructor

### Example: Adding User Profile Feature

```cpp
// 1. Domain model (no dependencies)
// src/models/UserProfile.h
class UserProfile {
public:
    explicit UserProfile(int user_id, const std::string& bio);
    int userId() const { return user_id_; }
    std::string bio() const { return bio_; }
};

// 2. Repository interface
// src/repositories/UserProfileRepository.h
class UserProfileRepository {
public:
    virtual std::optional<UserProfile> findByUserId(int user_id) = 0;
    virtual bool save(const UserProfile& profile) = 0;
};

// 3. Service with business logic
// src/services/UserProfileService.h
class UserProfileService {
public:
    UserProfileService(std::shared_ptr<UserProfileRepository> repo,
                       std::shared_ptr<Logger> logger)
        : repo_(repo), logger_(logger) {}
    
    std::optional<UserProfile> getProfile(int user_id) {
        logger_->info("Fetching profile for user {}", user_id);
        return repo_->findByUserId(user_id);
    }
    
private:
    std::shared_ptr<UserProfileRepository> repo_;
    std::shared_ptr<Logger> logger_;
};

// 4. Controller handling HTTP
// src/http/controllers/UserProfileController.h
class UserProfileController {
public:
    UserProfileController(std::shared_ptr<UserProfileService> service)
        : service_(service) {}
    
    HttpResponse getProfile(const HttpRequest& req) {
        int user_id = extractUserId(req.path);
        
        if (auto profile = service_->getProfile(user_id)) {
            return HttpResponse::ok(profile.toJson());
        }
        return HttpResponse::notFound("Profile not found");
    }
    
private:
    std::shared_ptr<UserProfileService> service_;
};

// 5. Wire in ApplicationFactory
// src/app/ApplicationFactory.cpp
auto profile_repo = std::make_shared<PostgreSQLUserProfileRepository>(db);
auto profile_service = std::make_shared<UserProfileService>(
    profile_repo, logger
);
auto profile_controller = std::make_unique<UserProfileController>(
    profile_service
);
app.addController(profile_controller);
```

---

## 20. Engineering Rules (Non-Negotiable)

These rules are absolute. Violations require special approval.

1. **No God Classes**  
   A class should never have more than 5 public methods or 200 lines of code.

2. **Controllers Never Access Database**  
   Controllers must use services; services must use repositories.

3. **Services Never Know HTTP Details**  
   Services don't know HttpRequest or HttpResponse; they work with domain objects.

4. **Repositories Contain Only Persistence Logic**  
   No business logic in repositories; they're data access abstractions.

5. **Prefer Composition Over Inheritance**  
   Use object composition instead of class inheritance; max 1 level of inheritance.

6. **Every Class Must Have Single Responsibility**  
   If you can describe the class without using "and", it passes.

7. **Every Dependency Must Point Inward**  
   Controllers know services, services know repositories; never reverse.

8. **No Raw new/delete**  
   Use `std::unique_ptr` or `std::make_shared`; never manual memory management.

9. **Use RAII Everywhere**  
   Acquire resources in constructor, release in destructor.

10. **Keep Functions Small and Cohesive**  
    A function should fit on one screen (< 30 lines typically).

11. **Every Public Class Must Have Clear Purpose**  
    Public classes should answer "what is this for?" in one sentence.

12. **Every Architectural Decision Documented**  
    Significant decisions recorded in docs/decisions/ with rationale.

13. **No Static State or Singletons**  
    Dependencies are injected; no global state or static instances.

14. **Logging in Every Layer**  
    Controllers, services, repositories all log appropriately.

15. **Fail Fast at Boundaries**  
    Validate input at class construction; catch errors early.

16. **Configuration is Immutable**  
    Loaded once at startup; never modified during runtime.

17. **No Silent Failures**  
    Every failure is either returned, logged, or thrown; nothing lost.

18. **Thread Safety or Document Limitations**  
    State the thread-safety guarantees of every class.

19. **Standard Types Over Custom**  
    Use `std::vector`, `std::string`, `std::optional` before custom types.

20. **Production Mindset Always**  
    Write code assuming it will run at scale with millions of users.

---

## Conclusion

This engineering principles document defines the standard for Cortex Code Intelligence Platform. Every engineer is responsible for understanding and upholding these principles. This isn't about following rules blindly—it's about building systems that are maintainable, testable, scalable, and secure.

When you're unsure about a decision, ask: "Does this violate any principle? Is this the simplest solution? Would this decision scale to 1 million users? Can this be tested easily?"

Write code that your future self will thank you for.

---

**Document Version:** 1.0  
**Last Updated:** 2026-07-28  
**Next Review:** 2026-10-28
