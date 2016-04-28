// Login URL
var providers = 'livejournal,facebook,vkontakte,yandex,google';
var evdb_url = 'http://MYHOST/EVDB_CGI';
var lgnz_url = 'https://loginza.ru/api/widget?token_url=' + evdb_url
             + '&providers_set=' + providers;

// do eventdb request
function do_request(action, callback){
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (xhttp.readyState == 4 && xhttp.status == 200) {
      if (xhttp.responseText == ""){ return; }
      try {
        var data = JSON.parse(xhttp.responseText);
      }
      catch(e){
        var data = {error_type: "xhttp",
                    error_message: xhttp.responseText};
      }
      if (data.error_type){ process_err(data); }
      else { callback(data);}
    }
  };
  xhttp.open("GET", evdb_url + '?action=' + action
                  + '&t=' + Math.random(), true);
  xhttp.send();
}

// process error
function process_err(data){
  alert(data.error_type + ": " + data.error_message);
}


/////////////////////////////////////////////////////////////////
// make the model for user face (external account information)
// which is used in severel places.
function make_face_model(data){
  var mk_icon = function(d){
      if (d.site == 'vk') return 'RESDIR/vk.png';
      if (d.site == 'lj') return 'RESDIR/lj.gif';
      if (d.site == 'fb') return 'RESDIR/fb.png';
      if (d.site == 'yandex') return 'RESDIR/ya.png';
      if (d.site == 'google') return 'RESDIR/go.png';
      if (d.site == 'gplus')  return 'RESDIR/gp.png';
      return "RESDIR/q.png";};
  var mk_html = function(d){
      return '<a style="text-decoration: none;" href="' + d.id + '">'
           + '<img style="margin-bottom:-2px;" src="' + mk_icon(d) + '">'
           + '&nbsp;<b>' + d.name + '</b></a>'};
  return {
    id:    ko.observable(data.id),
    name:  ko.observable(data.name),
    site:  ko.observable(data.site),
    icon:  ko.pureComputed(function(){return mk_icon(data)}, this),
    html:  ko.pureComputed(function(){return mk_html(data)}, this)
  };
}

/////////////////////////////////////////////////////////////////
// Russion level names
var mk_rlevel = function(l) {
  if (l == -1) return 'ограниченный';
  if (l == 0) return 'обычный';
  if (l == 1) return 'модератор';
  if (l == 2) return 'администратор';
  if (l == 3) return 'самый главный';
  return null;};

/////////////////////////////////////////////////////////////////
// simplified user model (for main and other pages)
function simple_view_model(data) {
  // Data, first create empty observables.
  var self = this;
  self.alias    = ko.observable();
  self.level    = ko.observable();

  // Function for filling observables from the data.
  // It will be also needed for updating data after some actions.
  var update_data = function(data){ // my user info
    self.alias(data.alias);
    self.level(data.level);
  };
  // Fill the data in the beginning
  update_data(data);

  // Make an observable with user level in Russian.
  self.rlevel = ko.pureComputed(
    function(){return mk_rlevel(self.level())}, this);

  //////////////////////////////////////////////
  // logout/logout
  // is the user logged in
  self.is_logged = ko.pureComputed(
    function(){return self.alias()!=undefined}, this);
  // loginza url
  self.lgnz_url = lgnz_url;
  // logout action
  self.logout  = function(){ do_request('logout', update_data); }
}

/////////////////////////////////////////////////////////////////
// full user model (for user page)
function full_view_model(data) {
  // Data, first create empty observables.
  var self = this;
  self.alias    = ko.observable();
  self.level    = ko.observable();
  self.faces    = ko.observableArray();
  self.joinreq  = ko.observableArray();
  self.users    = ko.observableArray(); // user list

  // Function for filling observables from the data.
  // It will be also needed for updating data after some actions.
  var update_data = function(data){ // my user info
    self.alias(data.alias);
    self.level(data.level);
    self.faces(ko.utils.arrayMap(data.faces, make_face_model));
    self.joinreq(ko.utils.arrayMap(data.joinreq, make_face_model));
    // hide the user list
    self.user_list(false);
    // update new_alias
    self.new_alias(self.alias());
  };

  // Make an observable with user level in Russian.
  self.rlevel = ko.pureComputed(
    function(){return mk_rlevel(self.level())}, this);

  //////////////////////////////////////////////
  // logout/logout
  // is the user logged in
  self.is_logged = ko.pureComputed(
    function(){return self.alias()!=undefined}, this);
  // loginza url
  self.lgnz_url = lgnz_url;
  // logout action
  self.logout  = function(){ do_request('logout', update_data); }

  //////////////////////////////////////////////
  // Change alias.
  self.alias_form = ko.observable(false); // alias form switcher
  self.sw_alias_form = function(){self.alias_form(!self.alias_form());};
  self.new_alias  = ko.observable(self.alias());
  self.set_alias  = function(){
     if (self.new_alias() == self.alias()) return;
     do_request('set_alias ' + self.new_alias(), function(data){
       self.alias(data.alias); })};

  //////////////////////////////////////////////
  // Join form
  self.join_form = ko.observable(false); // join form switcher
  self.sw_join_form = function(){self.join_form(!self.join_form());};
  self.joinreq_done = ko.observable(false);
  self.joinreq_name = ko.observable();
  self.joinreq_add = function(){
     if (self.joinreq_name() == self.alias()) return;
     do_request('joinreq_add ' + self.joinreq_name(), function(data){
       self.joinreq_done(true); })};
  self.joinreq_accept = function(i){
    do_request('joinreq_accept ' + i(), update_data);}
  self.joinreq_delete = function(i){
    do_request('joinreq_delete ' + i(), update_data);};

  //////////////////////////////////////////////
  // User list
  var user_list_vars = ['показать', 'скрыть'];
  self.user_list = ko.observable(false); // join form switcher
  self.user_list_btn = ko.observable(user_list_vars[0]);
  // Show/hide user list. The request runs here.
  self.sw_user_list = function(){
    self.user_list(!self.user_list());
    self.user_list_btn(user_list_vars[self.user_list()?1:0]);
    if (self.user_list()){
      do_request('user_list', update_users);
    }
  };
  // build user_list model as an observableArray
  var update_users = function(data){
    self.users(ko.utils.arrayMap(data,
      function(d){ return new user_list_entry(d);}));
  }
  // build user model from a user_list entry
  var user_list_entry = function(data){
    var self = this;
    self.alias  = ko.observable(data.alias);
    self.level  = ko.observable(data.level);
    self.faces  = ko.observableArray(
        ko.utils.arrayMap(data.faces, make_face_model));
    self.rlevel = ko.pureComputed(
        function(){return mk_rlevel(self.level())}, this);
    // level form switcher
    self.level_form = ko.observable(false);
    self.sw_level_form = function(){self.level_form(!self.level_form());};
    // level hints
    self.level_hints = ko.observableArray(ko.utils.arrayMap(
         data.level_hints, function(l){
           return {level:l, rlevel:mk_rlevel(l)};}));
    self.new_level = ko.observable(data.level);
  };
  // set level of the user
  self.set_level = function(user){
    do_request('set_level "' + user.alias() + '" ' + user.new_level(),
      function(u){ user.level(u.level); user.level_form(false);});
  }

  //////////////////////////////////////////////
  // Fill the data
  update_data(data);
}



// This function is run once after loading user.htm
function user_on_load(){
  document.cookie = "RETPAGE=" + document.URL;
  do_request('my_info', function(data){
    ko.applyBindings(new full_view_model(data));
  });
}

// This function is run once after loading user.htm
function main_on_load(){
  document.cookie = "RETPAGE=" + document.URL;
  do_request('my_info', function(data){
    ko.applyBindings(new simple_view_model(data));
  });
}
