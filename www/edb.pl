#!/usr/bin/perl -w
use strict;
use CGI   ':standard';
use HTTP::Tiny;
use JSON;

my $evdb_prog="EVDB_SRV";
my $providers="livejournal,facebook,vkontakte,yandex,google";
my $myhost = 'MYHOST';
my $url="http://MYHOST/EVDB_CGI";

my $token   = param('token') || '';
my $session = param('session') || cookie('SESSION') || '';
my $action  = param('action') || '';
my $ret     = cookie('RETPAGE') || '';

my $out;
my $cookie;

if ($action eq ''){
  $out=qx(printf "%s" "$token" | $evdb_prog login 2>/dev/null) || '';
  # new session
  my $data  = decode_json($out);
  $session = $data->{session} || '';
  $cookie = cookie(-name=>'SESSION', -value=>$session,
                   -expires=>'+1y', -host=>$myhost) if $session;
  if ($ret){
    print redirect (-uri=>$ret, -cookie=>$cookie);
    exit 0;
  }
}

elsif ($action eq 'logout'){
  $out=qx(printf "%s" "$session" | $evdb_prog $action 2>/dev/null) || '';
  $cookie = cookie(-name=>'SESSION',
                   -expires=>'-1s', -host=>$myhost);
}

else {
  $out=qx(printf "%s" "$session" | $evdb_prog $action 2>/dev/null) || '';
}

if ($cookie){
  print header (-type=>'application/json', -charset=>'utf-8', -cookie=>$cookie);
}
else{
  print header (-type=>'application/json', -charset=>'utf-8');
}

print $out;

