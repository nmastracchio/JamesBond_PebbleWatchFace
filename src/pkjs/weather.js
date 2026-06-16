function wmoCondition(code) {
  if (code === 0 || code === 1) return 'CLEAR';
  if (code === 2)               return 'PARTLY CLOUDY';
  if (code === 3)               return 'OVERCAST';
  if (code === 45 || code === 48) return 'FOG';
  if (code >= 51 && code <= 57) return 'DRIZZLE';
  if (code >= 61 && code <= 67) return 'RAIN';
  if (code >= 71 && code <= 77) return 'SNOW';
  if (code >= 80 && code <= 82) return 'SHOWERS';
  if (code === 85 || code === 86) return 'SNOW SHOWERS';
  if (code >= 95)               return 'THUNDERSTORM';
  return 'UNKNOWN';
}

function locationError(err) {
  console.log('Location error: ' + err.message);
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

Pebble.addEventListener('ready', function(e) {
  console.log('PebbleKit JS ready!');
  getWeather();
});

Pebble.addEventListener('appmessage', function(e) {
  console.log('AppMessage received — refreshing weather');
  getWeather();
});

function locationSuccess(pos) {
  var url = 'https://api.open-meteo.com/v1/forecast' +
    '?latitude='        + pos.coords.latitude +
    '&longitude='       + pos.coords.longitude +
    '&current_weather=true' +
    '&temperature_unit=celsius';

  var xhr = new XMLHttpRequest();
  xhr.onload = function() {
    try {
      var json = JSON.parse(this.responseText);
      var cw = json.current_weather;
      var temperature = Math.round(cw.temperature);
      var conditions  = wmoCondition(cw.weathercode);
      console.log('Weather: ' + conditions + ' ' + temperature + 'C');

      Pebble.sendAppMessage(
        { 'KEY_TEMPERATURE': temperature, 'KEY_CONDITIONS': conditions },
        function() { console.log('Weather sent to Pebble'); },
        function(e) { console.log('Send failed: ' + JSON.stringify(e)); }
      );
    } catch(err) {
      console.log('Parse error: ' + err);
    }
  };
  xhr.onerror = function() {
    console.log('XHR error fetching weather');
  };
  xhr.open('GET', url);
  xhr.send();
}
