#include "httplib.h"
#include "json.hpp"

#include <pqxx/pqxx>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using json = nlohmann::json;

const std::string DEFAULT_STATUS = "todo";

const std::vector<std::string> TASK_STATUSES = {
    "todo",
    "in_progress",
    "done",
    "blocker"
};

const std::string& get_database_url() {
    static const std::string database_url = []() {
        const char* value = std::getenv("DATABASE_URL");

        if (value == nullptr || std::string(value).empty()) {
            throw std::runtime_error(
                "DATABASE_URL environment variable is not set"
            );
        }

        return std::string(value);
    }();

    return database_url;
}

void send_json(
    httplib::Response& res,
    int status,
    const json& data
) {
    res.status = status;
    res.set_content(data.dump(), "application/json; charset=utf-8");
}

void send_error(
    httplib::Response& res,
    int status,
    const std::string& message
) {
    send_json(res, status, {
        {"error", message}
    });
}

// Проверка допустимого статуса задачи.
bool is_valid_task_status(const std::string& status) {
    for (const auto& allowed : TASK_STATUSES) {
        if (status == allowed) {
            return true;
        }
    }

    return false;
}

// Получение списка проектов.
void get_projects(
    const httplib::Request&,
    httplib::Response& res
) {
    try {
        pqxx::connection connection(get_database_url());
        pqxx::read_transaction transaction(connection);

        auto result = transaction.exec(
            R"(
                SELECT
                    p.id,
                    p.name,
                    CASE
                        WHEN EXISTS (
                            SELECT 1
                            FROM tasks t
                            WHERE t.project_id = p.id
                              AND t.status = 'blocker'
                        )
                        THEN 'red'

                        WHEN EXISTS (
                            SELECT 1
                            FROM tasks t
                            WHERE t.project_id = p.id
                              AND t.status <> 'done'
                              AND t.deadline IS NOT NULL
                              AND t.deadline < NOW()
                        )
                        THEN 'yellow'

                        ELSE 'green'
                    END AS status
                FROM projects p
                ORDER BY p.id;
            )"
        );

        json response = json::array();

        for (const auto& row : result) {
            response.push_back({
                {"id", row["id"].as<int>()},
                {"name", row["name"].as<std::string>()},
                {"status", row["status"].as<std::string>()}
            });
        }

        send_json(res, 200, response);
    }
    catch (const std::exception& e) {
        std::cerr << "GET /api/projects: "
                  << e.what() << std::endl;

        send_error(res, 500, "Database error");
    }
}

// Создание задачи.
void create_task(
    const httplib::Request& req,
    httplib::Response& res
) {
    json body;

    try {
        body = json::parse(req.body);
    }
    catch (const json::exception&) {
        send_error(res, 400, "Invalid JSON");
        return;
    }

    // Проверяем обязательные поля.
    if (!body.is_object() ||
        !body.contains("project_id") ||
        !body.contains("title")) {

        send_error(
            res,
            400,
            "project_id and title are required"
        );

        return;
    }

    if (!body["project_id"].is_number_integer() ||
        !body["title"].is_string()) {

        send_error(
            res,
            400,
            "Invalid project_id or title type"
        );

        return;
    }

    int project_id = body["project_id"].get<int>();
    std::string title = body["title"].get<std::string>();

    if (project_id <= 0 || title.empty() || title.size() > 255) {
        send_error(res, 400, "Invalid project_id or title");
        return;
    }

    std::string assignee = "Не назначен";

    if (body.contains("assignee") && !body["assignee"].is_null()) {
        if (!body["assignee"].is_string()) {
            send_error(res, 400, "Invalid assignee");
            return;
        }

        assignee = body["assignee"].get<std::string>();

        if (assignee.size() > 255) {
            send_error(res, 400, "Assignee is too long");
            return;
        }
    }

    std::string status = DEFAULT_STATUS;

    if (body.contains("status")) {
        if (!body["status"].is_string()) {
            send_error(res, 400, "Invalid task status");
            return;
        }

        status = body["status"].get<std::string>();
    }

    if (!is_valid_task_status(status)) {
        send_error(res, 400, "Unsupported task status");
        return;
    }

    std::string deadline;

    if (body.contains("deadline") && !body["deadline"].is_null()) {
        if (!body["deadline"].is_string()) {
            send_error(res, 400, "Invalid deadline");
            return;
        }

        deadline = body["deadline"].get<std::string>();

        if (deadline.empty() || deadline.size() > 64) {
            send_error(res, 400, "Invalid deadline");
            return;
        }
    }

    try {
        pqxx::connection connection(get_database_url());
        pqxx::work transaction(connection);

        // Проверяем существование проекта.
        auto project = transaction.exec_params(
            "SELECT id FROM projects WHERE id = $1",
            project_id
        );

        if (project.empty()) {
            send_error(res, 404, "Project not found");
            return;
        }

        // Создаем задачу.
        auto result = transaction.exec_params(
            R"(
                INSERT INTO tasks (
                    project_id,
                    title,
                    assignee,
                    deadline,
                    status
                )
                VALUES (
                    $1,
                    $2,
                    $3,
                    NULLIF($4, '')::timestamptz,
                    $5
                )
                RETURNING id;
            )",
            project_id,
            title,
            assignee,
            deadline,
            status
        );

        int task_id = result[0]["id"].as<int>();

        transaction.commit();

        send_json(res, 201, {
            {"id", task_id},
            {"project_id", project_id},
            {"title", title},
            {"assignee", assignee},
            {"deadline", deadline.empty() ? json(nullptr) : json(deadline)},
            {"status", status}
        });
    }
    catch (const pqxx::sql_error& e) {
        std::cerr << "POST /api/tasks SQL error: "
                  << e.what() << std::endl;

        send_error(res, 400, "Invalid task data");
    }
    catch (const std::exception& e) {
        std::cerr << "POST /api/tasks: "
                  << e.what() << std::endl;

        send_error(res, 500, "Internal server error");
    }
}

// Расчет статуса конкретного проекта.
void get_project_status(
    const httplib::Request& req,
    httplib::Response& res
) {
    try {
        if (req.matches.size() < 2) {
            send_error(res, 400, "Invalid project ID");
            return;
        }

        int project_id = std::stoi(req.matches[1].str());

        if (project_id <= 0) {
            send_error(res, 400, "Invalid project ID");
            return;
        }

        pqxx::connection connection(get_database_url());
        pqxx::work transaction(connection);

        auto result = transaction.exec_params(
            R"(
                SELECT
                    p.id,
                    CASE
                        WHEN EXISTS (
                            SELECT 1
                            FROM tasks t
                            WHERE t.project_id = p.id
                              AND t.status = 'blocker'
                        )
                        THEN 'red'

                        WHEN EXISTS (
                            SELECT 1
                            FROM tasks t
                            WHERE t.project_id = p.id
                              AND t.status <> 'done'
                              AND t.deadline IS NOT NULL
                              AND t.deadline < NOW()
                        )
                        THEN 'yellow'

                        ELSE 'green'
                    END AS calculated_status
                FROM projects p
                WHERE p.id = $1;
            )",
            project_id
        );

        if (result.empty()) {
            send_error(res, 404, "Project not found");
            return;
        }

        std::string calculated_status =
            result[0]["calculated_status"].as<std::string>();

        std::string reason = "Все в порядке";

        if (calculated_status == "red") {
            reason = "Найден блокер";
        }
        else if (calculated_status == "yellow") {
            reason = "Есть просроченные задачи";
        }

        // Сохраняем рассчитанный статус проекта.
        transaction.exec_params(
            "UPDATE projects SET status = $1 WHERE id = $2",
            calculated_status,
            project_id
        );

        transaction.commit();

        send_json(res, 200, {
            {"project_id", project_id},
            {"calculated_status", calculated_status},
            {"reason", reason}
        });
    }
    catch (const std::invalid_argument&) {
        send_error(res, 400, "Invalid project ID");
    }
    catch (const std::out_of_range&) {
        send_error(res, 400, "Project ID is out of range");
    }
    catch (const std::exception& e) {
        std::cerr << "GET project status: "
                  << e.what() << std::endl;

        send_error(res, 500, "Internal server error");
    }
}

int main() {
    httplib::Server server;

    // Проверяем подключение к базе при запуске.
    try {
        pqxx::connection connection(get_database_url());

        if (!connection.is_open()) {
            throw std::runtime_error("Cannot connect to PostgreSQL");
        }

        std::cout << "PostgreSQL connection established"
                  << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Database connection failed: "
                  << e.what() << std::endl;

        return 1;
    }

    // Проверка работоспособности сервера.
    server.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        send_json(res, 200, {
            {"status", "ok"}
        });
    });

    // Получение проектов.
    server.Get("/api/projects", get_projects);

    // Создание задачи.
    server.Post("/api/tasks", create_task);

    // Получение статуса проекта.
    server.Get(
        R"(/api/projects/(\d+)/status)",
        get_project_status
    );

    std::cout << "Backend started on port 8080"
              << std::endl;

    if (!server.listen("0.0.0.0", 8080)) {
        std::cerr << "Failed to start HTTP server"
                  << std::endl;

        return 1;
    }

    return 0;
}
