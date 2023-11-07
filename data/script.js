socket = new WebSocket("ws:/" + "/" + location.host + ":81");
socket.onopen = function(e) { console.log("[socket] socket.onopen "); };
socket.onerror = function(e) { console.log("[socket] socket.onerror "); };
socket.onmessage = function(e) {
  console.log("[socket] " + e.data);

  if (e.data) {
    // Split the received message into parts
    const parts = e.data.split(" ");
    const sensor1Data = parts.find(part => part.startsWith("sensor1:"));
    const sensor2Data = parts.find(part => part.startsWith("sensor2:"));

    if (sensor1Data && sensor2Data) {
      const sensor1Value = sensor1Data.split(":")[1];
      const sensor2Value = sensor2Data.split(":")[1];
      console.log(sensor1Value, sensor2Value);

      // Update the HTML elements
      document.getElementById("parkingStatus1").innerHTML = (sensor1Value === '1' ? 'Occupied' : 'Empty');
      document.getElementById("parkingStatus2").innerHTML = (sensor2Value === '1' ? 'Occupied' : 'Empty');
    }
  }
};
