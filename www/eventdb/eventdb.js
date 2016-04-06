//
var providers = 'livejournal,facebook,vkontakte,yandex,google';
var evdb_url = 'http://slazav.mccme.ru/perl/auth3s.pl';
var lgnz_url = 'https://loginza.ru/api/widget?token_url=' + evdb_url
             + '&providers_set=' + providers;

// do eventdb request
function do_request(action, callback){
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (xhttp.readyState == 4 && xhttp.status == 200) {
      var data = JSON.parse(xhttp.responseText);
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

// print user identity
function user_identity(user){
  return '<b><a class="nd" href=">' + user.identity + '">'
       + '<img  class="up" src="' + get_ppic(user.provider) + '">'
       + '&nbsp;' + user.full_name + '</a></b>';
}

// fill user data
function update_user_info(my_info){
  var lform = document.getElementById('login_box');
  if (my_info.session && my_info.alias){
    lform.innerHTML = '<a href="user.htm"><span class="user_alias"></span></a> '
       + '<input type="submit" value="выйти" '
       + 'name="LogOut" onclick="do_logout()"/>';
    a = document.getElementsByClassName('user_alias');
    for (var i=0; i < a.length; i++){
      a[i].innerHTML = my_info.alias; }
    a = document.getElementsByClassName('user_rlevel');
    for (var i=0; i < a.length; i++){
      a[i].innerHTML = get_rlevel(my_info.level); }
    a = document.getElementsByClassName('user_identity');
    for (var i=0; i < a.length; i++){
      a[i].innerHTML = user_identity(my_info); }
  } else {
    lform.innerHTML='<a href="' + lgnz_url + '">войти</a>';
  }
}



function do_init(){
  document.cookie = "RETPAGE=" + document.URL;
  var lform = document.getElementById('login_box');
  lform.innerHTML='<a href="' + lgnz_url + '">войти</a>';
  do_request('my_info', update_user_info);
}

function do_logout(){
  do_request('logout', function(info){
    document.cookie = "SESSION=";
    window.location.replace("main.htm")
  });
}

function get_ppic(pr){
  if (pr == 'vk') return 'http://slazav.mccme.ru/eventdb/vk.png';
  if (pr == 'lj') return 'http://slazav.mccme.ru/eventdb/lj.gif';
  if (pr == 'fb') return 'http://slazav.mccme.ru/eventdb/fb.png';
  if (pr == 'yandex') return 'http://slazav.mccme.ru/eventdb/ya.png';
  if (pr == 'google') return 'http://slazav.mccme.ru/eventdb/gp.png';
  return null;
}

function get_rlevel(l){
  if (l == -1) return 'ограниченный';
  if (l == 0) return 'обычный';
  if (l == 1) return 'модератор';
  if (l == 2) return 'администратор';
  if (l == 3) return 'самый главный';
  return null;
}
