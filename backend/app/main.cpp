#include "httplib.h"
#include <iostream>
#include <vector>
#include <string>
#include <mutex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct Project {
    int id;
    std::string name;
    std::string status;
};

struct Task {
    int id;
    int project_id;
    std::string title;
    std::string assignee;
    std::string deadline;
    std::string status;
};

std::vector<Project> projects_db = {
    {1, "Внедрение CRM", "green"},
    {2, "Разработка приложения", "yellow"}
};
std::vector<Task> tasks_db;
int next_task_id = 1;
std::mutex db_mutex;

bool is_overdue(const std::string& deadline_str) {
    return deadline_str.find("2023") != std::string::npos;
}

int main() {
    httplib::Server svr;

    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
        });

    svr.Get("/api/projects", [](const httplib::Request&, httplib::Response& res) {
        json response = json::array();
        for (const auto& p : projects_db) {
            response.push_back({ {"id", p.id}, {"name", p.name}, {"status", p.status} });
        }
        res.set_content(response.dump(), "application/json");
        });

    svr.Post("/api/tasks", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            std::lock_guard<std::mutex> lock(db_mutex);

            Task new_task;
            new_task.id = next_task_id++;
            new_task.project_id = body["project_id"];
            new_task.title = body["title"];
            new_task.assignee = body.value("assignee", "Не назначен");
            new_task.deadline = body["deadline"];
            new_task.status = body.value("status", "todo");

            tasks_db.push_back(new_task);

            for (auto& p : projects_db) {
                if (p.id == new_task.project_id && new_task.status == "blocker") {
                    p.status = "red";
                }
            }

            res.status = 201;
            res.set_content(json({ {"id", new_task.id}, {"title", new_task.title} }).dump(), "application/json");
        }
        catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({ {"error", "Invalid JSON"} }).dump(), "application/json");
        }
        });

    svr.Get(R"(/api/projects/(\d+)/status)", [](const httplib::Request& req, httplib::Response& res) {
        int project_id = std::stoi(req.matches[1]);
        std::lock_guard<std::mutex> lock(db_mutex);

        bool has_blocker = false;
        bool has_overdue = false;

        for (const auto& t : tasks_db) {
            if (t.project_id == project_id) {
                if (t.status == "blocker") has_blocker = true;
                if (t.status != "done" && is_overdue(t.deadline)) has_overdue = true;
            }
        }

        std::string calc_status = "green";
        std::string reason = "Все в порядке";

        if (has_blocker) {
            calc_status = "red";
            reason = "Найден блокер";
        }
        else if (has_overdue) {
            calc_status = "yellow";
            reason = "Есть просроченные задачи";
        }

        for (auto& p : projects_db) {
            if (p.id == project_id) p.status = calc_status;
        }

        res.set_content(json({ {"project_id", project_id}, {"calculated_status", calc_status}, {"reason", reason} }).dump(), "application/json");
        });

    std::cout << "Server started on port 8080" << std::endl;
    svr.listen("0.0.0.0", 8080);
}