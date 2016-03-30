var VERSION = '1.8';

var Clay = require('clay');
var clayConfig = require('config.json');
var clay = new Clay(clayConfig);

Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready! Version: ' + VERSION);
});
