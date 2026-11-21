-- Base schema. IF NOT EXISTS allows recovery on databases initialized
-- by the previous docker-entrypoint-initdb.d approach.
CREATE TABLE IF NOT EXISTS projects (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    status VARCHAR(50) NOT NULL DEFAULT 'green'
);

CREATE TABLE IF NOT EXISTS tasks (
    id SERIAL PRIMARY KEY,
    project_id INTEGER NOT NULL REFERENCES projects(id) ON DELETE CASCADE,
    title VARCHAR(255) NOT NULL,
    assignee VARCHAR(255),
    deadline TIMESTAMPTZ,
    status VARCHAR(50) NOT NULL DEFAULT 'todo'
);

CREATE TABLE IF NOT EXISTS users (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    role VARCHAR(50) NOT NULL
);
