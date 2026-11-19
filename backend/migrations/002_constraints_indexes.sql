-- Apply safe defaults and enforce valid statuses on existing/new rows.
UPDATE projects SET status = 'green'
WHERE status IS NULL OR status NOT IN ('green', 'yellow', 'red');

UPDATE tasks SET status = 'todo'
WHERE status IS NULL OR status NOT IN ('todo', 'in_progress', 'done', 'blocker');

ALTER TABLE projects ALTER COLUMN status SET DEFAULT 'green';
ALTER TABLE projects ALTER COLUMN status SET NOT NULL;
ALTER TABLE tasks ALTER COLUMN status SET DEFAULT 'todo';
ALTER TABLE tasks ALTER COLUMN status SET NOT NULL;

DO $$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'projects_status_check') THEN
        ALTER TABLE projects ADD CONSTRAINT projects_status_check
            CHECK (status IN ('green', 'yellow', 'red'));
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'tasks_status_check') THEN
        ALTER TABLE tasks ADD CONSTRAINT tasks_status_check
            CHECK (status IN ('todo', 'in_progress', 'done', 'blocker'));
    END IF;
END $$;

CREATE INDEX IF NOT EXISTS idx_tasks_project_id ON tasks(project_id);
CREATE INDEX IF NOT EXISTS idx_tasks_deadline ON tasks(deadline);
CREATE INDEX IF NOT EXISTS idx_tasks_status ON tasks(status);
CREATE INDEX IF NOT EXISTS idx_tasks_project_status ON tasks(project_id, status);
