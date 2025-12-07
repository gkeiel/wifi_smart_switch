const char HTML_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Local Panel</title>

<style>
  body {
    font-family: Arial;
    text-align: center;
    margin-top: 40px;
    background: white;
  }
  h2 {margin-bottom: 30px;}
  .btn {
    font-size: 18px;
    padding: 15px 40px;
    margin: 12px;
    border: none;
    border-radius: 12px;
    cursor: pointer;
    background: #2196F3;
    color: white;
  }
  #statusBox, #timersBox {
    margin-top: 30px;
    padding: 15px;
    font-size: 20px;
    background: white;
    display: inline-block;
    border-radius: 10px;
    min-width: 300px;
  }
</style>
</head>

<body>
<h2>MQTT Controller</h2>
<h3>Main control</h3>
<button class="btn" onclick="sendLocal('POWER ON 1')">POWER ON 1</button>
<button class="btn" onclick="sendLocal('POWER ON 2')">POWER ON 2</button><br>
<button class="btn" onclick="sendLocal('POWER OFF 1')">POWER OFF 1</button>
<button class="btn" onclick="sendLocal('POWER OFF 2')">POWER OFF 2</button><br>

<h3>Timers control</h3>
<select id="timerSelector">
  <option value="T1 ON">Timer 1 ON</option>
  <option value="T1 OFF">Timer 1 OFF</option>
  <option value="T2 ON">Timer 2 ON</option>
  <option value="T2 OFF">Timer 2 OFF</option>
</select>
<br><br>
<input id="timerInput" type="text" placeholder="hh:mm" maxlength="5" oninput="validateTime(this)">
<br><br>
<button class="btn" onclick="setTimer()">SET</button>
<button class="btn" onclick="stopTimer()">STOP</button><br>
<div id="statusBox">STATUS:</div><br>
<div id="timersBox">TIMERS:</div>

<script>
  function validateTime(input) {
    let v = input.value;
    v = v.replace(/[^0-9:]/g, "");
    if (v.length === 2 && !v.includes(":")) v += ":";
    if (v.length > 5) v = v.substring(0,5);
    input.value = v;
  }

  // -----------------------------------------------
  // Relay Commands
  // -----------------------------------------------
  function sendLocal(cmd) {
    fetch('/cmd?c=' + encodeURIComponent(cmd))
    .then(r => r.text())
    .then(t => console.log(t));
  }

  // -----------------------------------------------
  // Timer commands
  // -----------------------------------------------
  function setTimer() {
    const type = document.getElementById("timerSelector").value;
    const time = document.getElementById("timerInput").value;

    if (!/^(?:[01]\d|2[0-3]):[0-5]\d$/.test(time)) {
      alert("Invalid format.");
      return;
    }

    const cmd = "SET " + type + " " + time;
    sendLocal(cmd);
  }
  function stopTimer() {
    const type = document.getElementById("timerSelector").value;
    const cmd = "STOP " +type;
    sendLocal(cmd);
  }

  // -----------------------------------------------
  // Update status
  // -----------------------------------------------
  function pollStatus() {
    fetch('/cmd?c=STATUS')
      .then(r => r.text())
      .then(txt => {

      // STATUS
      document.getElementById("statusBox").innerText = txt;
      document.getElementById("timersBox").innerText = "";
    });
  }
  setInterval(pollStatus, 2000);
  pollStatus();
</script>
</body>
</html>
)=====";