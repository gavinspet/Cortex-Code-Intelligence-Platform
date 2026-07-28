# Cortex Code Intelligence Platform - Folder Structure & DI Architecture

## Folder Organization

### `/include/core/`
**Purpose**: Core infrastructure and cross-cutting concerns

**Why this layer exists**:
- Contains foundational abstractions that entire application depends on
- Separates infrastructure from business logic
- Makes it clear what belongs to the framework vs. business domain

**Modules**:
- `di/` - Dependency Injection framework
- `interfaces/` - Abstract interfaces for all injectable services

---

### `/include/core/di/`
**Purpose**: Lightweight dependency injection framework

**Contents**:
- `ServiceContainer.h` - Type-safe DI container
- `ServiceContainer.cpp` - Template implementations

**What problems it solves**:
1. **Decoupling**: Services don't create their own dependencies
2. **Testability**: Can inject mocks/fakes for unit testing
3. **Flexibility**: Can swap implementations without changing code
4. **Clarity**: Dependency graph is explicit and discoverable

**Design Pattern**: Service Locator + Factory Pattern

**Key Features**:
- Header-only templates for compile-time type safety
- Support for Singleton and Transient lifetimes
- Uses `std::any` for type erasure internally
- Uses `std::type_info` for type-safe lookups
- No macros, no global state, no reflection

**Example Usage**:
```cpp
// Registration (in Application::buildDependencyGraph)
container.registerSingleton<ILogger>([](const ServiceContainer& c) {
    return std::make_shared<SpdlogLogger>("Cortex");
});

// Resolution (in services)
auto logger = container.resolve<ILogger>();
```

**Why no third-party DI**:
- Keeps dependencies minimal
- Full control over behavior
- Lightweight (essential for embedded scenarios)
- Learning tool for architectural principles

---

### `/include/core/interfaces/`
**Purpose**: Abstract interfaces for all business services

**Currently Contains**:
- `IService.h` - Base interface all services implement

**Future Contents** (as services are created):
- `IUserService.h`
- `IProjectService.h`
- `IAnalysisService.h`
- etc.

**SOLID Principle**: Interface Segregation
- Each interface represents one specific capability
- Services implement multiple interfaces as needed
- Clients depend on specific interfaces, not implementations

**Why separate from implementations**:
- Clear separation of contract from implementation
- Enables easy testing (interface can be mocked)
- Shows all service contracts in one place
- Facilitates API documentation

---

### `/include/app/`
**Purpose**: Application orchestration and lifecycle

**Contains**:
- `Application.h/cpp` - Main orchestrator with DI coordination
- `ApplicationFactory.h/cpp` - Creates Application with dependencies

**Responsibilities**:
1. **Build Dependency Graph** - Register all services
2. **Configure Framework** - Set up Drogon
3. **Manage Lifecycle** - Start, run, shutdown
4. **Expose Services** - Allow access to ServiceContainer

**Design Pattern**: Facade + Builder
- Facade: Simplifies Drogon complexity
- Builder: Constructs dependency graph

**Why Application is special**:
- Only place where ALL services are known
- Entry point for entire application
- Coordinates between infrastructure and services
- Makes architecture explicit and reviewable

---

### `/include/config/`
**Purpose**: Configuration management

**Contains**:
- `Configuration.h` - Abstract interface
- `FileConfiguration.h/cpp` - JSON file implementation
- `ConfigurationValidator.h/cpp` - Validation + environment overrides

**Why DI doesn't manage config**:
- Config is bootstrapping infrastructure (needed before DI)
- Must be available before any services are created
- Injected directly by ApplicationFactory
- Becomes available to services via ServiceContainer

---

### `/include/logging/`
**Purpose**: Centralized logging infrastructure

**Contains**:
- `Logger.h` - Abstract interface
- `SpdlogLogger.h/cpp` - spdlog implementation
- `LoggerFactory.h/cpp` - Creates loggers

**Why DI doesn't manage logger**:
- Logger is bootstrapping infrastructure (needed during startup)
- Required before services are created
- Used by Application itself
- Available to services via Application::getLogger()

---

### `/include/utils/`
**Purpose**: Utility types and helpers

**Contains**:
- `Result.h` - Type-safe error handling
- Future: Helpers, utilities

**Design Pattern**: Value Object (for Result<T>)

---

### `/src/core/`
**Purpose**: Implementation of core infrastructure

**Mirrors** `/include/core/` structure

---

### `/src/core/di/`
**Purpose**: ServiceContainer implementation

**Note**: Mostly templates in header, minimal .cpp file

---

### `/src/app/`
**Purpose**: Application implementation

**Contains**:
- `Application.cpp` - Orchestration + DI setup
- `ApplicationFactory.cpp` - Dependency creation

---

## Dependency Flow

```
┌─────────────────────────────────────────────────┐
│  main.cpp                                       │
│  - Create ApplicationFactory                    │
│  - Call ApplicationFactory::create()            │
│  - Call app->run()                              │
└──────────────────┬──────────────────────────────┘
                   │ creates
                   ▼
┌─────────────────────────────────────────────────┐
│  ApplicationFactory                             │
│  - Load configuration                           │
│  - Create logger                                │
│  - Create Application                           │
└──────────────────┬──────────────────────────────┘
                   │ creates with deps
                   ▼
┌─────────────────────────────────────────────────┐
│  Application                                    │
│  - Store config, logger, ServiceContainer      │
│  - buildDependencyGraph() - Register services  │
│  - initializeDrogon() - Configure framework    │
│  - run() - Start HTTP server                    │
└──────────────────┬──────────────────────────────┘
                   │ manages
                   ▼
┌─────────────────────────────────────────────────┐
│  ServiceContainer                               │
│  - registerSingleton<T>()                       │
│  - registerTransient<T>()                       │
│  - resolve<T>()                                 │
│                                                 │
│  Stores:                                        │
│  - Singletons (created once, reused)           │
│  - Transient factories (created per request)   │
└─────────────────────────────────────────────────┘
```

## Service Lifecycle

### Bootstrap (Non-DI) Services
1. **Configuration** - Loaded by ApplicationFactory
   - Singleton by nature
   - Created FIRST (needed by everything)
   - Passed to Application

2. **Logger** - Created by LoggerFactory
   - Singleton by nature
   - Created SECOND (used by everything)
   - Passed to Application

3. **Application** - Created by ApplicationFactory
   - Orchestrator
   - Owns ServiceContainer
   - Builds dependency graph

### DI-Managed Services
All other services:
- Registered in Application::buildDependencyGraph()
- Can depend on config/logger via ServiceContainer
- Can depend on other services
- Can be Singleton or Transient

---

## Adding New Services

### Step 1: Define Interface
```cpp
// include/core/interfaces/IMyService.h
class IMyService : public IService {
public:
    virtual ~IMyService() noexcept = default;
    virtual std::string doWork() = 0;
};
```

### Step 2: Implement Service
```cpp
// include/services/MyService.h
class MyService : public IMyService {
private:
    LoggerPtr logger_;
public:
    MyService(LoggerPtr logger) : logger_(logger) {}
    std::string doWork() override { ... }
};
```

### Step 3: Register in DI
```cpp
// src/app/Application.cpp - in buildDependencyGraph()
container.registerSingleton<IMyService>([](const ServiceContainer& c) {
    return std::make_shared<MyService>(logger_);  // Access logger via Application
});
```

### Step 4: Use Service
```cpp
// Anywhere with access to ServiceContainer
auto myService = container.resolve<IMyService>();
myService->doWork();
```

---

## Design Principles

### 1. **Dependency Inversion**
- Code depends on interfaces, not implementations
- High-level modules don't depend on low-level modules
- Both depend on abstractions

### 2. **Single Responsibility**
- Each class has ONE reason to change
- ServiceContainer: Manages service lifecycle
- Application: Orchestrates startup
- Individual services: Do one thing well

### 3. **Open/Closed**
- Services and interfaces are open for extension
- Adding new services doesn't modify existing code
- Just register new service in ServiceContainer

### 4. **Interface Segregation**
- Interfaces are focused and minimal
- Services implement multiple small interfaces
- Clients depend on specific interfaces they need

### 5. **Liskov Substitution**
- Any implementation of IService can replace another
- Enables testing with mocks/stubs
- Enables multiple implementations

---

## Testing Strategy

### Unit Testing
- Mock dependencies injected via ServiceContainer
- Test services in isolation
- Test Application with mock ServiceContainer

### Integration Testing
- Use real ServiceContainer
- Register real services
- Test service interactions

### Example Mock Injection
```cpp
// Create mock ServiceContainer
ServiceContainer mockContainer;
mockContainer.registerSingleton<ILogger>([](const ServiceContainer& c) {
    return std::make_shared<MockLogger>();  // Fake implementation
});

// Resolve with mock
auto myService = mockContainer.resolve<IMyService>();  // Gets injected MockLogger
```

---

## Files and Responsibilities

| File | Responsibility | SOLID | Pattern |
|------|---|---|---|
| `ServiceContainer.h` | Manage service lifecycle | SRP | Factory, Locator |
| `Application.h/cpp` | Orchestrate startup, build DI graph | SRP | Facade, Builder |
| `ApplicationFactory.h/cpp` | Create Application with bootstrap deps | SRP | Factory |
| `IService.h` | Base interface for services | ISP | Strategy |
| `Configuration.h` | Configuration abstraction | DIP | Strategy |
| `Logger.h` | Logging abstraction | DIP | Adapter |

---

## Next Steps

1. **Implement business services** following the same pattern
2. **Register services** in `Application::buildDependencyGraph()`
3. **Create controllers** that inject services from ServiceContainer
4. **Write unit tests** with mock ServiceContainer
5. **Review architecture** for adherence to ENGINEERING_PRINCIPLES.md
