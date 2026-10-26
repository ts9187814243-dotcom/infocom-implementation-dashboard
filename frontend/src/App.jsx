import { useState, useEffect } from "react";

export default function App() {
  const [projects, setProjects] = useState([]);
  const [isModalOpen, setIsModalOpen] = useState(false);
  
  const [taskData, setTaskData] = useState({
    title: "",
    project_id: 1,
    assignee: "",
    deadline: "2024-12-31T23:59:59Z",
    status: "todo"
  });

  const fetchProjects = async () => {
    try {
      const res = await fetch("http://localhost:8080/api/projects");
      const data = await res.json();
      setProjects(data);
    } catch (e) {
      console.error("Ошибка загрузки проектов", e);
    }
  };

  useEffect(() => {
    fetchProjects();
  }, []);


  const handleCreateTask = async (e) => {
    e.preventDefault();
    try {
      await fetch("http://localhost:8080/api/tasks", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(taskData)
      });
      setIsModalOpen(false);
      fetchProjects();
      alert("Задача создана!");
    } catch (e) {
      alert("Ошибка при создании задачи");
    }
  };

  const statusColors = { green: "#4caf50", yellow: "#ffeb3b", red: "#f44336" };

  return (
    <div style={{ padding: 24, fontFamily: "sans-serif", maxWidth: 800, margin: "0 auto" }}>
      <h1>Infocom Implementation Dashboard</h1>
      <button 
        onClick={() => setIsModalOpen(true)}
        style={{ padding: "10px 20px", background: "#1976d2", color: "white", border: "none", borderRadius: 4, cursor: "pointer", marginBottom: 20 }}
      >
        + Создать задачу
      </button>

      <div style={{ display: "grid", gap: 16 }}>
        {projects.map((p) => (
          <div key={p.id} style={{ border: "1px solid #ccc", padding: 16, borderRadius: 8, display: "flex", alignItems: "center", justifyContent: "space-between" }}>
            <div>
              <h3 style={{ margin: 0 }}>{p.name}</h3>
              <p style={{ margin: "4px 0", color: "#666" }}>ID: {p.id}</p>
            </div>
            <div style={{ width: 24, height: 24, borderRadius: "50%", background: statusColors[p.status] || "gray" }} title={p.status}></div>
          </div>
        ))}
      </div>

      {/* Модальное окно создания задачи */}
      {isModalOpen && (
        <div style={{ position: "fixed", top: 0, left: 0, right: 0, bottom: 0, background: "rgba(0,0,0,0.5)", display: "flex", justifyContent: "center", alignItems: "center" }}>
          <div style={{ background: "white", padding: 24, borderRadius: 8, width: 300 }}>
            <h2>Новая задача</h2>
            <form onSubmit={handleCreateTask} style={{ display: "flex", flexDirection: "column", gap: 10 }}>
              <input required placeholder="Название задачи" value={taskData.title} onChange={e => setTaskData({...taskData, title: e.target.value})} style={{ padding: 8 }} />
              <input placeholder="Исполнитель" value={taskData.assignee} onChange={e => setTaskData({...taskData, assignee: e.target.value})} style={{ padding: 8 }} />
              <select value={taskData.status} onChange={e => setTaskData({...taskData, status: e.target.value})} style={{ padding: 8 }}>
                <option value="todo">Todo</option>
                <option value="in_progress">In Progress</option>
                <option value="blocker">Blocker</option>
                <option value="done">Done</option>
              </select>
              <div style={{ display: "flex", gap: 10, marginTop: 10 }}>
                <button type="submit" style={{ flex: 1, padding: 10, background: "#4caf50", color: "white", border: "none", borderRadius: 4 }}>Создать</button>
                <button type="button" onClick={() => setIsModalOpen(false)} style={{ flex: 1, padding: 10, background: "#f44336", color: "white", border: "none", borderRadius: 4 }}>Отмена</button>
              </div>
            </form>
          </div>
        </div>
      )}
    </div>
  );
}