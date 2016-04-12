//
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
      catch(e){ alert(e + "\n" + xhttp.responseText); }
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


// fill user data
function update_user_info(my_info){
  var lform = document.getElementById('login_box');
  var name_block = 'inline';
  var list_block = 'inline';
  if (my_info.session && my_info.alias){
    lform.innerHTML = '<a href="USERHTM"><span class="user_alias"></span></a> '
       + '<input type="submit" value="выйти" '
       + 'name="LogOut" onclick="do_logout()"/>';

    a = document.getElementsByClassName('user_alias');
    for (var i=0; i < a.length; i++){
      a[i].innerHTML = my_info.alias; }

    a = document.getElementsByClassName('user_rlevel');
    for (var i=0; i < a.length; i++){
      a[i].innerHTML = get_rlevel(my_info.level); }

    a = document.getElementsByClassName('user_joinreqs');
    for (var i=0; i < a.length; i++){
      a[i].innerHTML = user_joinreqs(my_info, '<br>'); }

    a = document.getElementsByClassName('user_identity');
    for (var i=0; i < a.length; i++){
      a[i].innerHTML = user_faces(my_info, '<br>'); }

    if (my_info.level<1) {list_block='none';}
  } else {
    lform.innerHTML='<a href="' + lgnz_url + '">войти</a>';
    name_block = 'none';
    list_block = 'none';
  }

  a = document.getElementsByClassName('name_block');
  for (var i=0; i < a.length; i++){ a[i].style.display=name_block; }

  a = document.getElementsByClassName('list_block');
  for (var i=0; i < a.length; i++){ a[i].style.display=list_block; }

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
    window.location.replace("MAINHTM")
  });
}

function get_ppic(pr){
  var a = '<img style="margin-bottom:-2px;" src="';
  var b = '">';
  if (pr == 'vk') return a + 'RESDIR/vk.png' + b;
  if (pr == 'lj') return a + 'RESDIR/lj.gif' + b;
  if (pr == 'fb') return a + 'RESDIR/fb.png' + b;
  if (pr == 'yandex') return a + 'RESDIR/ya.png' + b;
  if (pr == 'google') return a + 'RESDIR/go.png' + b;
  if (pr == 'gplus')  return a + 'RESDIR/gp.png' + b;
  return "";
}

function get_rlevel(l){
  if (l == -1) return 'ограниченный';
  if (l == 0) return 'обычный';
  if (l == 1) return 'модератор';
  if (l == 2) return 'администратор';
  if (l == 3) return 'самый главный';
  return null;
}

// print user face
function user_face(face){
  return '<b><a class="nd" href="' + face.id + '">'
       + get_ppic(face.site) + '&nbsp;'
       + face.name + '</a></b>';
}

// print user faces
function user_faces(user, sep){
  var ret="";
  for (var i=0; i<user.faces.length; i++){
    ret += (i==0?"":sep) + user_face(user.faces[i]);
  }
  return ret;
}

// print user join requests
function user_joinreqs(user, sep){
  var ret="";
  if (user.joinreq == undefined) return "";
  for (var i=0; i<user.joinreq.length; i++){
    ret += (i==0?"":sep) + user_face(user.joinreq[i]);
    ret += ' <input type=submit value="принять" onclick="joinreq_accept('+ i +')">';
    ret += ' <input type=submit value="отклонить" onclick="joinreq_delete('+ i +')">';
  }
  return ret;
}

function joinreq_accept(i){
  do_request('joinreq_accept ' + i, do_init);
}

function joinreq_delete(i){
  do_request('joinreq_delete ' + i, do_init);
}

