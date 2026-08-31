-- Cortex Code Intelligence Platform - Jobs Table Migration
-- Version: 001
-- Description: Create jobs table for repository analysis tracking

CREATE TABLE IF NOT EXISTS jobs (
    id CHAR(36) PRIMARY KEY COMMENT 'UUID job identifier',
    repository_url VARCHAR(1024) NOT NULL COMMENT 'Git repository URL',
    status VARCHAR(30) NOT NULL COMMENT 'Job status: QUEUED, RUNNING, COMPLETED, FAILED',
    created_at DATETIME NOT NULL COMMENT 'Job creation timestamp',
    started_at DATETIME NULL COMMENT 'Job processing start timestamp',
    completed_at DATETIME NULL COMMENT 'Job processing completion timestamp',
    repository_path TEXT NULL COMMENT 'Local path where repository was cloned',
    error_message TEXT NULL COMMENT 'Error details if job failed',
    clone_duration_ms BIGINT NULL COMMENT 'Repository clone duration in milliseconds',

    INDEX idx_status (status),
    INDEX idx_created_at (created_at),
    INDEX idx_status_created (status, created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Repository analysis jobs for Cortex platform';
