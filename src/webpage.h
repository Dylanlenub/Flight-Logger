#pragma once

const char htmlPage[] = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body { background:#121212; color:white; font-family:Arial; padding:20px; }
table { border-collapse:collapse; }
td { padding:6px 20px 6px 0; font-size:16px; }
td:first-child { color:#aaa; }
</style>
<script>
function update() {
  fetch("/status").then(r => r.json()).then(d => {
    document.getElementById("temp").innerText     = d.temp     + " C";
    document.getElementById("pressure").innerText = d.pressure + " hPa";
    document.getElementById("altitude").innerText = d.altitude + " m";
    document.getElementById("pitch").innerText  = d.pitch    + " deg";
    document.getElementById("roll").innerText   = d.roll     + " deg";
    document.getElementById("yaw").innerText    = d.yaw      + " deg";
    document.getElementById("accelX").innerText   = d.accelX   + " m/s2";
    document.getElementById("accelY").innerText   = d.accelY   + " m/s2";
    document.getElementById("accelZ").innerText   = d.accelZ   + " m/s2";
  });
}
setInterval(update, 1000);
update();
</script>
</head>
<body>
<h2>Flight Data</h2>
<table>
  <tr><td>Temperature</td><td id="temp">--</td></tr>
  <tr><td>Pressure</td>   <td id="pressure">--</td></tr>
  <tr><td>Altitude</td>   <td id="altitude">--</td></tr>
  <tr><td>Pitch</td>  <td id="pitch">--</td></tr>
  <tr><td>Roll</td>   <td id="roll">--</td></tr>
  <tr><td>Yaw</td>    <td id="yaw">--</td></tr>
  <tr><td>Accel X</td>    <td id="accelX">--</td></tr>
  <tr><td>Accel Y</td>    <td id="accelY">--</td></tr>
  <tr><td>Accel Z</td>    <td id="accelZ">--</td></tr>
</table>

<br>
<button onclick="doCalibrate()" style="padding:10px 24px;font-size:15px;background:#333;color:white;border:1px solid #555;border-radius:6px;cursor:pointer;">Calibrate</button>
<span id="calStatus" style="margin-left:12px;color:#aaa;"></span>

<script>
function doCalibrate() {
  document.getElementById("calStatus").innerText = "Calibrating...";
  fetch("/calibrate").then(() => {
    document.getElementById("calStatus").innerText = "Done!";
  });
}
</script>

</body>
</html>
)=====";