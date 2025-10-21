const express = require("express");
const bodyParser = require("body-parser");
const app = express();
let latestData = {};

app.use(bodyParser.json());
app.use(express.static("public")); // Serve your HTML from /public

// Endpoint for Arduino to send data (POST)
app.post("/data", (req, res) => {
  latestData = req.body;
  res.sendStatus(200);
});

// Endpoint for website to fetch latest data (GET)
app.get("/data", (req, res) => {
  res.json(latestData);
});

const PORT = 3000;
app.listen(PORT, () => {
  console.log(`Server running on http://localhost:${PORT}`);
});
