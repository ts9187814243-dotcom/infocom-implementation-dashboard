function App() {
  const team = ["Трушинский Степан — Team Lead", "Тагиров Эдгар — Fullstack"];
  return (
    <main style={{ padding: 24, fontFamily: "sans-serif" }}>
      <h1>Infocom Implementation Dashboard</h1>
      <h2>Участники команды</h2>
      <ul>
        {team.map((m, i) => (
          <li key={i}>{m}</li>
        ))}
      </ul>
    </main>
  );
}

export default App;