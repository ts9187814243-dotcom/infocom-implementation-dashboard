#!/bin/sh
set -eu

: "${DATABASE_URL:?DATABASE_URL must be set}"

# Wait for PostgreSQL readiness. The compose healthcheck also gates startup,
# but this protects direct container runs.
attempt=0
until pg_isready -d "$DATABASE_URL" >/dev/null 2>&1; do
    attempt=$((attempt + 1))
    if [ "$attempt" -ge 30 ]; then
        echo "PostgreSQL did not become ready" >&2
        exit 1
    fi
    sleep 2
done

psql "$DATABASE_URL" -v ON_ERROR_STOP=1 -c \
    "CREATE TABLE IF NOT EXISTS schema_migrations (version TEXT PRIMARY KEY, applied_at TIMESTAMPTZ NOT NULL DEFAULT NOW());"

for migration in /app/migrations/*.sql; do
    [ -f "$migration" ] || continue
    version="$(basename "$migration")"
    applied="$(psql "$DATABASE_URL" -tAc "SELECT 1 FROM schema_migrations WHERE version = '$version'")"
    if [ "$applied" = "1" ]; then
        echo "Already applied: $version"
        continue
    fi

    echo "Applying migration: $version"
    psql "$DATABASE_URL" -v ON_ERROR_STOP=1 -f "$migration"
    psql "$DATABASE_URL" -v ON_ERROR_STOP=1 -c \
        "INSERT INTO schema_migrations(version) VALUES ('$version') ON CONFLICT DO NOTHING;"
done

exec /app/server
