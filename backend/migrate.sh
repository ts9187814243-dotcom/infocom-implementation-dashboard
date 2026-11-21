#!/bin/sh
set -eu

: "${DATABASE_URL:?DATABASE_URL must be set}"

attempt=0
until pg_isready -d "$DATABASE_URL" >/dev/null 2>&1; do
    attempt=$((attempt + 1))
    if [ "$attempt" -ge 30 ]; then
        echo "PostgreSQL did not become ready" >&2
        exit 1
    fi
    sleep 2
done

psql "$DATABASE_URL" -v ON_ERROR_STOP=1 <<'SQL'
CREATE TABLE IF NOT EXISTS schema_migrations (
    version TEXT PRIMARY KEY,
    applied_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
SQL

for migration in /app/migrations/*.sql; do
    [ -f "$migration" ] || continue
    version="$(basename "$migration")"

    echo "Checking migration: $version"

    {
        printf '%s\n' "SELECT pg_advisory_lock(7142026);"
        printf "%s\n" "SELECT EXISTS (SELECT 1 FROM schema_migrations WHERE version = '$version') AS already_applied \gset"
        printf '%s\n' '\if :already_applied'
        printf "    \\echo 'Already applied: %s'\n" "$version"
        printf '%s\n' '\else'
        printf '%s\n' '    BEGIN;'
        printf "    \\i %s\n" "$migration"
        printf "    INSERT INTO schema_migrations(version) VALUES ('%s');\n" "$version"
        printf '%s\n' '    COMMIT;'
        printf "    \\echo 'Applied: %s'\n" "$version"
        printf '%s\n' '\endif'
        printf '%s\n' "SELECT pg_advisory_unlock(7142026);"
    } | psql "$DATABASE_URL" -v ON_ERROR_STOP=1
done

exec /app/server
