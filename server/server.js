const express = require("express");
const bodyParser = require("body-parser");
const app = express();

// Store the latest posted data from Arduino
let latestData = { alarmOn: false };

app.use(bodyParser.json());
app.use(express.static("public"));

// Endpoint for Arduino to send data (POST)
app.post("/data", (req, res) => {
  latestData = req.body || { alarmOn: false };
  console.log("Received /data POST:", latestData);
  res.json({ ok: true, received: latestData });
});

// Endpoint for website to fetch latest data (GET)
app.get("/data", (req, res) => {
  const out = { alarmOn: !!latestData.alarmOn };
  res.json(out);
});

// Endpoint to reset alarm (called by the website)
app.post("/reset", (req, res) => {
  latestData = { alarmOn: false };
  console.log("Alarm reset via /reset");
  res.json({ ok: true, alarmOn: false });
});

const PORT = 3000;
app.listen(PORT, "0.0.0.0", () => {
  console.log(
    `Server running on http://0.0.0.0:${PORT} (accessible via host IP)`
  );
});
