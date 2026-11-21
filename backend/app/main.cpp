#include "httplib.h"
#include "json.hpp"
#include <pqxx/pqxx>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

using json = nlohmann::json;

const std::vector<std::string> TASK_STATUSES = {
    "todo", "in_progress", "done", "blocker"
};

const std::string& get_database_url() {
    static const std::string url = [] {
        const char* value = std::getenv("DATABASE_URL");
        if (!value || std::string(value).empty()) {
            throw std::runtime_error("DATABASE_URL is not set");
        }
        return std::string(value);
    }();
    return url;
}

void send_json(httplib::Response& res, int status, const json& data) {
    res.status = status;
    res.set_content(data.dump(), "application/json; charset=utf-8");
}

void send_error(httplib::Response& res, int status, const std::string& message) {
    send_json(res, status, {{"error", message}});
}

bool is_valid_task_status(const std::string& status) {
    return std::any_of(TASK_STATUSES.begin(), TASK_STATUSES.end(),
        [&status](const std::string& allowed) { return allowed == status; });
}

// GET /api/projects — projects with calculated traffic-light status.
void get_projects(const httplib::Request&, httplib::Response& res) {
    try {
        pqxx::connection connection(get_database_url());
        pqxx::read_transaction tx(connection);
        auto result = tx.exec(R"(
            SELECT p.id, p.name,
                CASE
                    WHEN EXISTS (SELECT 1 FROM tasks t
                        WHERE t.project_id = p.id AND t.status = 'blocker')
                    THEN 'red'
                    WHEN EXISTS (SELECT 1 FROM tasks t
                        WHERE t.project_id = p.id AND t.status <> 'done'
                          AND t.deadline IS NOT NULL AND t.deadline < NOW())
                    THEN 'yellow'
                    ELSE 'green'
                END AS calculated_status
            FROM projects p ORDER BY p.id
        )");
        json response = json::array();
        std::transform(
            result.begin(), result.end(), std::back_inserter(response),
            [](const pqxx::row& row) {
                return json{
                    {"id", row["id"].as<int>()},
                    {"name", row["name"].as<std::string>()},
                    {"status", row["calculated_status"].as<std::string>()}
                };
            }
        );
        send_json(res, 200, response);
    } catch (const std::exception& e) {
        std::cerr << "GET /api/projects: " << e.what() << '\n';
        send_error(res, 500, "Internal server error");
    }
}

// GET /api/tasks — list tasks.
void get_tasks(const httplib::Request&, httplib::Response& res) {
    try {
        pqxx::connection connection(get_database_url());
        pqxx::read_transaction tx(connection);
        auto result = tx.exec(R"(
            SELECT id, project_id, title, assignee, deadline, status
            FROM tasks ORDER BY id
        )");
        json response = json::array();
        std::transform(
            result.begin(), result.end(), std::back_inserter(response),
            [](const pqxx::row& row) {
                json deadline = nullptr;
                if (!row["deadline"].is_null()) deadline = row["deadline"].as<std::string>();
                return json{
                    {"id", row["id"].as<int>()},
                    {"project_id", row["project_id"].as<int>()},
                    {"title", row["title"].as<std::string>()},
                    {"assignee", row["assignee"].is_null() ? json(nullptr) : json(row["assignee"].as<std::string>())},
                    {"deadline", deadline},
                    {"status", row["status"].as<std::string>()}
                };
            }
        );
        send_json(res, 200, response);
    } catch (const std::exception& e) {
        std::cerr << "GET /api/tasks: " << e.what() << '\n';
        send_error(res, 500, "Internal server error");
    }
}

// POST /api/tasks — create a task. deadline is required by the API contract.
void create_task(const httplib::Request& req, httplib::Response& res) {
    json body;
    try {
        body = json::parse(req.body);
    } catch (const json::exception&) {
        send_error(res, 400, "Invalid JSON");
        return;
    }

    if (!body.is_object() || !body.contains("project_id") ||
        !body.contains("title") || !body.contains("deadline")) {
        send_error(res, 400, "project_id, title and deadline are required");
        return;
    }
    if (!body["project_id"].is_number_integer() || !body["title"].is_string() ||
        !body["deadline"].is_string()) {
        send_error(res, 400, "Invalid field types");
        return;
    }

    const int project_id = body["project_id"].get<int>();
    const std::string title = body["title"].get<std::string>();
    const std::string deadline = body["deadline"].get<std::string>();
    if (project_id <= 0 || title.empty() || title.size() > 255 ||
        deadline.empty() || deadline.size() > 64) {
        send_error(res, 400, "Invalid project_id, title or deadline");
        return;
    }

    std::string assignee;
    if (body.contains("assignee") && !body["assignee"].is_null()) {
        if (!body["assignee"].is_string()) {
            send_error(res, 400, "assignee must be a string or null");
            return;
        }
        assignee = body["assignee"].get<std::string>();
        if (assignee.size() > 255) {
            send_error(res, 400, "assignee is too long");
            return;
        }
    }

    std::string status = "todo";
    if (body.contains("status")) {
        if (!body["status"].is_string()) {
            send_error(res, 400, "status must be a string");
            return;
        }
        status = body["status"].get<std::string>();
    }
    if (!is_valid_task_status(status)) {
        send_error(res, 400, "Unsupported task status");
        return;
    }

    try {
        pqxx::connection connection(get_database_url());
        pqxx::work tx(connection);
        auto project = tx.exec_params("SELECT id FROM projects WHERE id = $1", project_id);
        if (project.empty()) {
            send_error(res, 404, "Project not found");
            return;
        }

        // PostgreSQL validates the date/time format through the typed cast.
        auto result = tx.exec_params(R"(
            INSERT INTO tasks (project_id, title, assignee, deadline, status)
            VALUES ($1, $2, NULLIF($3, ''), $4::timestamptz, $5)
            RETURNING id, deadline
        )", project_id, title, assignee, deadline, status);

        const int task_id = result[0]["id"].as<int>();
        const std::string normalized_deadline = result[0]["deadline"].as<std::string>();
        tx.commit();
        send_json(res, 201, {
            {"id", task_id}, {"project_id", project_id}, {"title", title},
            {"assignee", assignee.empty() ? json(nullptr) : json(assignee)},
            {"deadline", normalized_deadline}, {"status", status}
        });
    } catch (const pqxx::sql_error& e) {
        std::cerr << "POST /api/tasks SQL error: " << e.what() << '\n';
        send_error(res, 400, "Invalid task data or deadline");
    } catch (const std::exception& e) {
        std::cerr << "POST /api/tasks: " << e.what() << '\n';
        send_error(res, 500, "Internal server error");
    }
}

// GET /api/projects/{id}/status
void get_project_status(const httplib::Request& req, httplib::Response& res) {
    try {
        if (req.matches.size() < 2) {
            send_error(res, 400, "Invalid project ID");
            return;
        }
        const std::string id_text = req.matches[1].str();
        if (id_text.empty() || !std::all_of(id_text.begin(), id_text.end(),
                [](unsigned char c) { return c >= '0' && c <= '9'; })) {
            send_error(res, 400, "Invalid project ID");
            return;
        }
        const long id_long = std::stol(id_text);
        if (id_long <= 0 || id_long > 2147483647L) {
            send_error(res, 400, "Project ID is out of range");
            return;
        }
        const int project_id = static_cast<int>(id_long);

        pqxx::connection connection(get_database_url());
        pqxx::work tx(connection);
        auto result = tx.exec_params(R"(
            SELECT p.id,
                CASE
                    WHEN EXISTS (SELECT 1 FROM tasks t
                        WHERE t.project_id = p.id AND t.status = 'blocker')
                    THEN 'red'
                    WHEN EXISTS (SELECT 1 FROM tasks t
                        WHERE t.project_id = p.id AND t.status <> 'done'
                          AND t.deadline IS NOT NULL AND t.deadline < NOW())
                    THEN 'yellow'
                    ELSE 'green'
                END AS calculated_status
            FROM projects p WHERE p.id = $1
        )", project_id);
        if (result.empty()) {
            send_error(res, 404, "Project not found");
            return;
        }
        const std::string status = result[0]["calculated_status"].as<std::string>();
        const std::string reason = status == "red" ? "Найден блокер" :
            (status == "yellow" ? "Есть просроченные задачи" : "Все в порядке");
        tx.exec_params("UPDATE projects SET status = $1 WHERE id = $2", status, project_id);
        tx.commit();
        send_json(res, 200, {
            {"project_id", project_id}, {"calculated_status", status}, {"reason", reason}
        });
    } catch (const std::exception& e) {
        std::cerr << "GET project status: " << e.what() << '\n';
        send_error(res, 500, "Internal server error");
    }
}

int main() {
    try {
        // Fail early if the configured database is not reachable.
        pqxx::connection connection(get_database_url());
        if (!connection.is_open()) {
            throw std::runtime_error("Database connection is not open");
        }
    } catch (const std::exception& e) {
        std::cerr << "Database connection failed: " << e.what() << '\n';
        return 1;
    }

    httplib::Server server;
    server.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        send_json(res, 200, {{"status", "ok"}});
    });
    server.Get("/api/projects", get_projects);
    server.Get("/api/tasks", get_tasks);
    server.Post("/api/tasks", create_task);
    server.Get(R"(/api/projects/(\d+)/status)", get_project_status);

    std::cout << "Backend listening on 0.0.0.0:8080\n";
    if (!server.listen("0.0.0.0", 8080)) {
        std::cerr << "Failed to start HTTP server\n";
        return 1;
    }
    return 0;
}
