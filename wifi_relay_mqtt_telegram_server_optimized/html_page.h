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
  <option value="T1">Timer 1</option>
  <option value="T2">Timer 2</option>
</select>
<br><br>
<input id="timeOn"  type="text" placeholder="Start hh:mm" maxlength="5" oninput="validateTime(this)">
<br><br>
<input id="timeOff" type="text" placeholder="Stop hh:mm" maxlength="5" oninput="validateTime(this)">
<br><br>
<button class="btn" onclick="setTimer()">SET</button>
<button class="btn" onclick="stopTimer()">TOOGLE</button><br>
<button class="btn" onclick="sendLocal('RANDOM ON')">Random ON</button>
<button class="btn" onclick="sendLocal('RANDOM OFF')">Random OFF</button><br>
<div id="statusBox">STATUS:</div><br>

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
    const timer = document.getElementById("timerSelector").value;
    const timer_on  = document.getElementById("timeOn").value;
    const timer_off = document.getElementById("timeOff").value

    if (!/^(?:[01]\d|2[0-3]):[0-5]\d$/.test(timer_on)) { alert("Invalid format."); return; }
    if (!/^(?:[01]\d|2[0-3]):[0-5]\d$/.test(timer_off)) { alert("Invalid format."); return; }

    sendLocal("SET " +timer +" ON " +timer_on);
    send("SET " +timer +" OFF " +timer_off);
  }
  function stopTimer() {
    const timer = document.getElementById("timerSelector").value;
    sendLocal("TOOGLE " +timer);
  }

  // -----------------------------------------------
  // Update status
  // -----------------------------------------------
  function pollStatus() {
    fetch('/cmd?c=STATUS')
      .then(r => r.text())
      .then(txt => {
      document.getElementById("statusBox").innerText = txt;
    });
  }
  setInterval(pollStatus, 2000);
  pollStatus();
</script>
</body>
</html>
)=====";