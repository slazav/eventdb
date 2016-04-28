function map_on_load(mapid){

  // Setup the map
  var map = L.map(mapid);
  var osm = L.tileLayer('http://{s}.tile.osm.org/{z}/{x}/{y}.png', {
    attribution: '&copy; <a href="http://osm.org/about/" target="_blank">OpenStreetMap</a> contributors'
  }).addTo(map);

  // change URL on map move
  map.on('moveend', function(e){
    z = map.getZoom();
    x = map.getCenter().lat.toFixed(5);
    y = map.getCenter().lng.toFixed(5);
    hasher.setHash(z+"/"+x+"/"+y);
  });

  // setup hasher: listen for history changes and run crossroads.
  function parseHash(newHash, oldHash){ crossroads.parse(newHash); }
  hasher.prependHash = '';
  hasher.initialized.add(parseHash); //parse initial hash
  hasher.changed.add(parseHash); //parse hash changes
  hasher.init(); //start listening for history change

  // setup crossroads
  crossroads.addRoute('{z}/{x}/{y}', function(z,x,y) { map.setView([x, y], z); });

  //initial URL
  hasher.setHash("8/55.66674/37.50732");
}
