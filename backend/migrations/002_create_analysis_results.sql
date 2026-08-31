-- Cortex Code Intelligence Platform - Analysis Results Migration
-- Version: 002
-- Description: Persist core analysis output for completed jobs

CREATE TABLE IF NOT EXISTS analysis_results (
    job_id CHAR(36) PRIMARY KEY COMMENT 'Foreign key to jobs.id',
    file_count INT NOT NULL COMMENT 'Number of files scanned',
    dir_count INT NOT NULL COMMENT 'Number of directories scanned',
    total_lines BIGINT NOT NULL COMMENT 'Total line count',
    language_distribution_json JSON NOT NULL COMMENT 'Serialized languageDistribution map',
    analyzed_at DATETIME NOT NULL COMMENT 'Analysis completion timestamp',
    clone_path TEXT NOT NULL COMMENT 'Local clone path used during analysis',

    CONSTRAINT fk_analysis_results_job
        FOREIGN KEY (job_id)
        REFERENCES jobs(id)
        ON DELETE CASCADE,

    INDEX idx_analysis_analyzed_at (analyzed_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Repository code analysis results';
