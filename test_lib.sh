check(){
  local exp_err=$1; shift;
  local msg=$1; shift;

  if [ "$exp_err" = 1 ]; then
    echo "== Expected Error == $msg =="
  else
    echo "== Expected OK == $msg =="
  fi

  ./eventdb "$@" 2>&1
  local ret=$?

  if [ "$exp_err" = 1 -a "$ret"  = 0 ] ||
     [ "$exp_err" = 0 -a "$ret" != 0 ]; then
    echo "WRONG RESPONSE!"
    exit
  fi
}
