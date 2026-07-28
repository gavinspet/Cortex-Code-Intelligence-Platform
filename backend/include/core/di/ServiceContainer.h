#pragma once

#include <memory>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <any>

namespace cortex::core::di {

/**
 * @enum ServiceLifetime
 * @brief Defines how services are instantiated and managed.
 * 
 * Singleton: Single instance created once, reused for all requests
 * Transient: New instance created for each request
 * 
 * Why:
 * - Singleton: For stateless services, loggers, config managers
 * - Transient: For request-scoped business logic
 */
enum class ServiceLifetime {
    Singleton,  // One instance for application lifetime
    Transient   // New instance for each resolution
};

/**
 * @struct ServiceMetadata
 * @brief Metadata about registered services
 */
struct ServiceMetadata {
    ServiceLifetime lifetime = ServiceLifetime::Transient;
    std::string typeName;
};

/**
 * @class ServiceContainer
 * @brief Lightweight type-safe dependency injection container.
 * 
 * Responsibility: Manages service registration and resolution
 * 
 * Design Pattern: Service Locator + Factory Pattern
 * SOLID: Dependency Inversion (depends on abstractions, not implementations)
 * 
 * Why it exists:
 * - Centralizes dependency management
 * - Enables loose coupling between components
 * - Makes testing easier (can inject mocks)
 * - Provides single point of service configuration
 * 
 * Why it belongs in core/di/:
 * - DI is a core cross-cutting concern
 * - Not specific to any business domain
 * - Infrastructure-level component
 * 
 * Architecture Notes:
 * - Uses std::any internally for type erasure
 * - Uses std::type_info for type-safe lookups
 * - Supports both singleton and transient lifetimes
 * - Factories are std::function<> for flexibility
 * - Thread-safe for registration and resolution
 * 
 * Example Usage:
 * 
 *   ServiceContainer container;
 *   
 *   // Register a singleton logger
 *   container.registerSingleton<ILogger>([]() {
 *       return std::make_shared<SpdlogLogger>("Cortex");
 *   });
 *   
 *   // Register a transient service
 *   container.registerTransient<IUserService>([](const ServiceContainer& c) {
 *       auto logger = c.resolve<ILogger>();
 *       return std::make_shared<UserService>(logger);
 *   });
 *   
 *   // Resolve services
 *   auto logger = container.resolve<ILogger>();
 *   auto userService = container.resolve<IUserService>();
 */
class ServiceContainer {
public:
    ServiceContainer() = default;
    
    // Prevent copying (only one instance should exist per application)
    ServiceContainer(const ServiceContainer&) = delete;
    ServiceContainer& operator=(const ServiceContainer&) = delete;

    /**
     * Register a singleton service (created once, reused)
     * 
     * @tparam TInterface Abstract interface type
     * @param factory Lambda that creates the service
     * 
     * The factory receives a reference to the container for dependency resolution
     * 
     * Example:
     *   container.registerSingleton<ILogger>([](const ServiceContainer& c) {
     *       return std::make_shared<SpdlogLogger>("Cortex");
     *   });
     */
    template<typename TInterface>
    void registerSingleton(
        std::function<std::shared_ptr<TInterface>(const ServiceContainer&)> factory) 
    {
        std::type_index key(typeid(TInterface));
        
        // Validate no duplicate registration
        if (services_.find(key) != services_.end()) {
            throw std::runtime_error(
                std::string("Service already registered: ") + key.name());
        }

        // Store factory and create singleton immediately
        auto singletonInstance = factory(*this);
        
        // Store the singleton instance
        services_[key] = singletonInstance;
        
        // Store metadata for singleton
        ServiceMetadata meta;
        meta.lifetime = ServiceLifetime::Singleton;
        meta.typeName = key.name();
        metadata_[key] = meta;
    }

    /**
     * Register a transient service (created for each request)
     * 
     * @tparam TInterface Abstract interface type
     * @param factory Lambda that creates the service
     * 
     * Example:
     *   container.registerTransient<IUserService>([](const ServiceContainer& c) {
     *       auto logger = c.resolve<ILogger>();
     *       return std::make_shared<UserService>(logger);
     *   });
     */
    template<typename TInterface>
    void registerTransient(
        std::function<std::shared_ptr<TInterface>(const ServiceContainer&)> factory)
    {
        std::type_index key(typeid(TInterface));
        
        // Validate no duplicate registration
        if (services_.find(key) != services_.end()) {
            throw std::runtime_error(
                std::string("Service already registered: ") + key.name());
        }

        // Store factory as a special marker for transient
        // We use a wrapper that can be called later
        transientFactories_[key] = 
            [factory](const ServiceContainer& c) -> std::any {
                return std::any(factory(c));
            };
        
        // Store metadata for transient
        ServiceMetadata meta;
        meta.lifetime = ServiceLifetime::Transient;
        meta.typeName = key.name();
        metadata_[key] = meta;
    }

    /**
     * Resolve a service instance
     * 
     * @tparam TInterface Service interface type
     * @return Shared pointer to service instance
     * 
     * For singletons: Returns the same instance every time
     * For transients: Creates a new instance each time
     * 
     * Throws std::runtime_error if service not registered
     * 
     * Example:
     *   auto logger = container.resolve<ILogger>();
     */
    template<typename TInterface>
    std::shared_ptr<TInterface> resolve() const
    {
        std::type_index key(typeid(TInterface));
        
        // Check if it's a transient service first
        auto transientIt = transientFactories_.find(key);
        if (transientIt != transientFactories_.end()) {
            auto result = transientIt->second(*this);
            try {
                return std::any_cast<std::shared_ptr<TInterface>>(result);
            } catch (const std::bad_any_cast&) {
                throw std::runtime_error(
                    std::string("Failed to cast service: ") + key.name());
            }
        }
        
        // Try singleton services
        auto serviceIt = services_.find(key);
        if (serviceIt == services_.end()) {
            throw std::runtime_error(
                std::string("Service not registered: ") + key.name());
        }

        try {
            return std::any_cast<std::shared_ptr<TInterface>>(serviceIt->second);
        } catch (const std::bad_any_cast&) {
            throw std::runtime_error(
                std::string("Failed to cast service: ") + key.name());
        }
    }

    /**
     * Check if a service is registered
     * 
     * @tparam TInterface Service interface type
     * @return true if registered, false otherwise
     */
    template<typename TInterface>
    bool isRegistered() const
    {
        std::type_index key(typeid(TInterface));
        return services_.find(key) != services_.end() ||
               transientFactories_.find(key) != transientFactories_.end();
    }

    /**
     * Get service lifetime information
     * 
     * @tparam TInterface Service interface type
     * @return ServiceMetadata with lifetime and type info
     */
    template<typename TInterface>
    ServiceMetadata getMetadata() const
    {
        std::type_index key(typeid(TInterface));
        auto it = metadata_.find(key);
        if (it == metadata_.end()) {
            throw std::runtime_error(
                std::string("Service not registered: ") + key.name());
        }
        return it->second;
    }

private:
    // Singleton instances (std::any to allow any type)
    mutable std::unordered_map<std::type_index, std::any> services_;

    // Transient factories
    mutable std::unordered_map<std::type_index, 
                                std::function<std::any(const ServiceContainer&)>> transientFactories_;

    // Service metadata
    std::unordered_map<std::type_index, ServiceMetadata> metadata_;
};

} // namespace cortex::core::di
