/* script.js */
const subjects = [
  { id: 1, name: "Real-time test", age: 73, gender: "Female", room: "203A" },
  { id: 2, name: "Fake person 1", age: 82, gender: "Male", room: "105B" },
  {
    id: 3,
    name: "Fake person 2",
    age: 92,
    gender: "Female",
    room: "210C",
  },
  { id: 4, name: "Fake person 3", age: 61, gender: "Male", room: "112A" },
  { id: 5, name: "Fake person 4", age: 83, gender: "Female", room: "215B" },
  { id: 6, name: "Fake person 5", age: 78, gender: "Male", room: "107C" },
];

let lastAlarmState = false;
let fakeAlarmStates = [false, false, false, false, false]; // for subjects 2–6

// Update fake alarms every 15 seconds
function updateFakeAlarms() {
  fakeAlarmStates = fakeAlarmStates.map(() => Math.random() < 0.2);
}
setInterval(updateFakeAlarms, 15000);
updateFakeAlarms(); // initial call

function renderCards(data) {
  const container = document.getElementById("subjects");
  container.innerHTML = "";

  subjects.forEach((subject, index) => {
    const alarmOn = index === 0 ? data.alarmOn : fakeAlarmStates[index - 1];
    const card = document.createElement("div");
    card.className = "card" + (alarmOn ? " alarm-on" : "");
    const now = new Date().toLocaleTimeString();
    card.innerHTML = `
      <h2>${subject.name}</h2>
      <p><strong>Age:</strong> ${subject.age}</p>
      <p><strong>Gender:</strong> ${subject.gender}</p>
      <p><strong>Room:</strong> ${subject.room}</p>
      <p class="status ${alarmOn ? "alarm" : "safe"}">
        ${alarmOn ? "🚨 Alarm ON" : "✅ Safe"}
      </p>
      <p class="timestamp">Last checked: ${now}</p>
      ${
        alarmOn && index === 0
          ? '<button onclick="resetAlarm()">Acknowledge</button>'
          : ""
      }
    `;
    container.appendChild(card);
  });

  // Sound alert if real patient's alarm just turned on
  if (data.alarmOn && !lastAlarmState) {
    document.getElementById("alertSound").play();
    if (Notification.permission === "granted") {
      new Notification("🚨 Patient Alert", {
        body: "Test Subject 1 is signaling for help!",
      });
    }
  }
  lastAlarmState = data.alarmOn;

  document.getElementById("lastUpdate").innerText =
    "Last update: " + new Date().toLocaleTimeString();
}

async function fetchData() {
  try {
    const res = await fetch("/data");
    const json = await res.json();
    renderCards(json);
  } catch (err) {
    console.error("Error fetching data:", err);
  }
}

async function resetAlarm() {
  try {
    await fetch("/reset", { method: "POST" });
    fetchData();
  } catch (err) {
    console.error("Error resetting alarm:", err);
  }
}

// Ask for notification permission
if (Notification && Notification.permission !== "granted") {
  Notification.requestPermission();
}

// Fetch every 5 seconds
fetchData();
setInterval(fetchData, 5000);
